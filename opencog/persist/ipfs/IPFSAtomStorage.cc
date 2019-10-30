/*
 * IPFSAtomStorage.cc
 * IPFS backend driver for persistent AtomSpace storage.
 *
 * Atoms and Values are saved to and restored from the IPFS IPLD
 * system.  Atoms are identified by means of unique ID's (GUID's),
 * which are cryptographic hashes of the unique Atom names. The
 * Values attached to Atoms are identified with IPLD CID's (content
 * ID's) that vary over time as the Values change.  The AtomSpace is
 * conceptually a map of (GUID, AtomSpace-ID) -> CID, so that, given
 * an Atom and an AtomSpace, one can find the Values attached to it.
 *
 * Copyright (c) 2008,2009,2013,2015,2017 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <stdlib.h>
#include <unistd.h>

#include <chrono>
#include <thread>

#include <opencog/atomspace/AtomSpace.h>

#include "IPFSAtomStorage.h"

using namespace opencog;

// Number of write-back queues
#define NUM_WB_QUEUES 6

/* ================================================================ */
// Constructors

void IPFSAtomStorage::init(const char * uri)
{
	tvpred = createNode(PREDICATE_NODE, "*-TruthValueKey-*");

	_uri = uri;

#define URIX_LEN (sizeof("ipfs://") - 1)  // Should be 7
	if (strncmp(uri, "ipfs://", URIX_LEN))
		throw IOException(TRACE_INFO, "Unknown URI '%s'\n", uri);

	// We expect the URI to be for the form
	//    ipfs:///atomspace-key
	//    ipfs://hostname/atomspace-key
	//    ipfs://hostname:port/atomspace-key
	// where the key will be used to publish the IPNS for the atomspace.
	// Read-only access to AtomSpaces is also supported. These have two
	// forms: with IPFS and IPNS:
	//    ipfs:///ipfs/Qm...
	//    ipfs:///ipns/Qm...

	_port = 5001;
	if ('/' == uri[URIX_LEN])
	{
		_hostname = "localhost";
		_keyname = &uri[URIX_LEN+1];
	}
	else
	{
		const char* start = &uri[URIX_LEN];
		_hostname = start;
		char* p = strchr((char *)start, '/');
		if (nullptr == p)
			throw IOException(TRACE_INFO, "Bad URI format '%s'\n", uri);
		size_t len = p - start;
		_hostname.resize(len);
		_keyname = &uri[len+URIX_LEN+1];

		// Keys are not allowed to have trailing slashes.
		size_t pos = _keyname.find('/');
		if (pos != std::string::npos) _keyname.resize(pos);

		// Search for a port, if present.
		p = strchr((char *)start, ':');
		if (p)
		{
			_port = atoi(p+1);
			size_t len = p - start;
			_hostname.resize(len);
		}
	}

	// If the "key" is actually an IPFS or IPNS CID...
	if (std::string::npos != _keyname.find("ipfs/"))
	{
		_atomspace_cid = &_keyname[sizeof("ipfs/")-1];
		_keyname.clear();
	}
	else
	if (std::string::npos != _keyname.find("ipns/"))
	{
		_key_cid = &_keyname[sizeof("ipns/")];
		_keyname.clear();
		// XXX TODO Maybe we should IPNS resolve the atomspace now!?
		// Well, we can't, or don't want to, because IPNS is too
		// slow. Let the user decide when to resolve.
	}

	// Trim trailing whitespace.  This can happen if we get the
	// key from SchemeEval::eval() which has a bad habit of
	// appending newline chars.
	const std::string WHITESPACE = " \n\r\t\f\v";
	size_t end = _atomspace_cid.find_last_not_of(WHITESPACE);
	if (std::string::npos != end)
		_atomspace_cid.resize(end+1);

	end = _key_cid.find_last_not_of(WHITESPACE);
	if (std::string::npos != end)
		_key_cid.resize(end+1);

	// Create pool of IPFS server connections.
	_initial_conn_pool_size = NUM_OMP_THREADS + NUM_WB_QUEUES;
	for (int i=0; i<_initial_conn_pool_size; i++)
	{
		ipfs::Client* conn = new ipfs::Client(_hostname, _port);
		conn_pool.push(conn);
	}

	bulk_load = false;
	bulk_store = false;
	clear_stats();

	// Create the IPNS key under which we will publish,
	// if it does not yet exist.
	_publish_keep_going = false;
	if (0 < _keyname.size())
	{
		// Brute force search for keys.
		ipfs::Json key_list;
		ipfs::Client clnt(_hostname, _port);
		clnt.KeyList(&key_list);
		for (const auto& item : key_list)
		{
			std::string kame = item["Name"];
			if (0 == kame.compare(_keyname))
			{
				_key_cid = item["Id"];
				break;
			}
		}
		if (0 < _key_cid.size())
		{
			std::cout << "Found existing AtomSpace key: /ipns/"
			          << _key_cid << std::endl;
		}
		else
		{
			// Not found; make a new one, by default.
			clnt.KeyGen(_keyname, "rsa", 2048, &_key_cid);
			std::cout << "Generated AtomSpace key: /ipns/"
			          << _key_cid << std::endl;
		}

		// We run IPNS publication in it's own thread, because it's so
		// horridly slow.  As of this writing, either 60 sec or 90 sec.
		// This is a well-known problem, see
		// https://github.com/ipfs/go-ipfs/issues/3860
		_publish_keep_going = true;
		std::thread publisher(publish_thread, this);
		publisher.detach();
		// std::this_thread::yield();
		// std::this_thread::sleep_for(50ms);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	// Initialize a new AtomSpace, but only if
	// we're not already working with one.
	if (0 == _atomspace_cid.size()) kill_data();
}

IPFSAtomStorage::IPFSAtomStorage(std::string uri) :
	_write_queue(this, &IPFSAtomStorage::vdo_store_atom, NUM_WB_QUEUES),
	_async_write_queue_exception(nullptr)
{
	init(uri.c_str());
}

IPFSAtomStorage::~IPFSAtomStorage()
{
	flushStoreQueue();

	_publish_keep_going = false;
	_publish_cv.notify_one();

	while (not conn_pool.is_empty())
	{
		ipfs::Client* conn = conn_pool.pop();
		delete conn;
	}
}

/**
 * connected -- unconditionally true, right now. I guess.
 * XXX FIXME, return false if IPFS connection cannot be made.
 */
bool IPFSAtomStorage::connected(void)
{
	return true;
}

/**
 * Return the IPFS CID of the current AtomSpace.
 */
std::string IPFSAtomStorage::get_ipfs_cid(void)
{
	return "/ipfs/" + _atomspace_cid;
}

/**
 * Return the IPNS CID of the current AtomSpace.
 */
std::string IPFSAtomStorage::get_ipns_key(void)
{
	return "/ipns/" + _key_cid;
}

/**
 * Use IPNS to find the latest IPFS cid for this AtomSpace.
 */
void IPFSAtomStorage::resolve_atomspace(void)
{
	if (0 == _key_cid.size()) return;

	// Caution: as of this writing, name resolution takes
	// exactly 60 seconds. This is a bug; see
	// https://github.com/ipfs/go-ipfs/issues/3860
	// for details.
	std::string ipfs_path;
	ipfs::Client* conn = conn_pool.pop();
	conn->NameResolve(_key_cid, &ipfs_path);
	conn_pool.push(conn);
	_atomspace_cid = ipfs_path;
}

/**
 * Return the IPFS CID of the given (globally unique) Atom.
 * The globally unique Atom is the one without any attached
 * Values, or any other mutable state. Because it's just the
 * immutable Atom, it's by definition globally unique.
 */
std::string IPFSAtomStorage::get_atom_guid(const Handle& h)
{
	if (guid_not_yet_stored(h)) do_store_atom(h);
	std::lock_guard<std::mutex> lck(_guid_mutex);
	return _guid_map.find(h)->second;
}

/**
 * Return the IPFS json of this Atom, including the current state
 * for Values and for the incoming set.
 */
ipfs::Json IPFSAtomStorage::get_atom_json(const Handle& atom)
{
	// Build the name
	std::string path = _atomspace_cid + "/" + atom->to_short_string();

	// std::cout << "Query path = " << path << std::endl;
	ipfs::Json dag;
	ipfs::Client* conn = conn_pool.pop();
	try
	{
		conn->DagGet(path, &dag);
	}
	catch (const std::exception& ex)
	{
		// If we are here, its because there was no such Atom
		// recorded in IPFS. That's a normal situation, just
		// ignore the error.
	}
	conn_pool.push(conn);
	return dag;
}

/**
 * Use IPNS to publish the latest IPFS cid for this AtomSpace.
 *
 * We run IPNS publication in it's own thread, because it's so
 * horridly slow.  As of this writing, either 60 sec or 90 sec.
 * This is a well-known problem, see
 * https://github.com/ipfs/go-ipfs/issues/3860
 */
void IPFSAtomStorage::publish_atomspace(void)
{
	if (0 == _key_cid.size()) return;
	_publish_cv.notify_one();
}

void IPFSAtomStorage::publish_thread(IPFSAtomStorage* self)
{
	ipfs::Client clnt(self->_hostname, self->_port);
	std::mutex mtx;
	while (self->_publish_keep_going)
	{
		std::unique_lock<std::mutex> lock(mtx);
		self->_publish_cv.wait(lock);

		// Last time out, just quit.
		if (not self->_publish_keep_going) break;

		std::cout << "Publishing AtomSpace CID: "
		          << self->_atomspace_cid << std::endl;

		// XXX hack alert -- lifetime set to 4 hours, it should be
		// infinity or something.... the TTL is 30 seconds, but should
		// be shorter or user-configurable .. set both with scheme bindings.
		try
		{
			std::string name;
			ipfs::Json options = {{"lifetime", "4h"}, {"ttl", "4h"}};
			clnt.NamePublish(self->_atomspace_cid,
			                 self->_keyname, options, &name);
			std::cout << "Published AtomSpace: " << name << std::endl;
		}
		catch (const std::exception& ex)
		{
			// Arghh. IPNS keeps throwing this error:
			// "can't replace a newer value with an older value"
			// which is insane, because maybe I *do* want to do exactly
			// that!  So WTF ... another IPNS bug.
			std::cerr << "Failed to publish AtomSpace IPNS entry: "
			          << ex.what() << std::endl;
		}
	}
}

void IPFSAtomStorage::update_atom_in_atomspace(const Handle& h,
                                               const std::string& cid)
{
	std::string label(encodeAtomToStr(h));

	// XXX FIXME ... this leaks pool entries, if ipfs ever throws.
	// We can't just catch, we need to rethrow, too.
	std::string new_as_id;
	ipfs::Client* conn = conn_pool.pop();
	{
		// Update the cid under a lock, as this method can
		// be called from multiple threads.  It's not actually
		// the cid that matters, its the patch itself.
		std::lock_guard<std::mutex> lck(_atomspace_cid_mutex);
		conn->ObjectPatchAddLink(_atomspace_cid, label, cid, &new_as_id);
		_atomspace_cid = new_as_id;
	}
	conn_pool.push(conn);

	{
		// Store the current cid for this atom; this is the cid
		// of the atom that has values attached to it.
		std::lock_guard<std::mutex> lck(_atom_cid_mutex);
		_atom_cid_map[h] = cid;
	}
}

/// Rethrow asynchronous exceptions caught during atom storage.
///
/// Atoms are stored asynchronously, from a write queue, from some
/// other thread. If that thread has an exception, e.g. due to some
/// IPFS error, and the exception is uncaught, then the process will
/// die. So we have to catch that exception.  Once caught, what do
/// we do with it? Well, we could ignore it, but then the user would
/// not know that the IPFS backend was damaged. So, instead, we throw
/// it at the first user, any user that is doing some other IPFS stuff.
void IPFSAtomStorage::rethrow(void)
{
	if (_async_write_queue_exception)
	{
		std::exception_ptr exptr = _async_write_queue_exception;
		_async_write_queue_exception = nullptr;
		std::rethrow_exception(exptr);
	}
}

/* ================================================================== */
/// Drain the pending store queue. This is a fencing operation; the
/// goal is to make sure that all writes that occurred before the
/// barrier really are performed before before all the writes after
/// the barrier.
///
/// Caution: this is potentially racey in two different ways.
/// First, there is a small window in the async_caller implementation,
/// where, if the timing is just so, the barrier might return before
/// the last element is written.  (Although everything else will have
/// gone out; only the last element is in doubt). Technically, that's
/// a bug, but its sufficiently "minor" so we don't fix it.
///
/// Caution: The IPNS publication is done async, because its so slow,
/// and so this will return before the IPNS publish has completed.
///
void IPFSAtomStorage::flushStoreQueue()
{
	rethrow();
	_write_queue.barrier();
	rethrow();
}

void IPFSAtomStorage::barrier()
{
	flushStoreQueue();
	// publish();
}

/* ================================================================ */

void IPFSAtomStorage::registerWith(AtomSpace* as)
{
	BackingStore::registerWith(as);
}

void IPFSAtomStorage::unregisterWith(AtomSpace* as)
{
	BackingStore::unregisterWith(as);

	flushStoreQueue();
}

/* ================================================================ */

/**
 * kill_data -- Publish an empty atomspace. Dangerous!
 * This will forget the IPFS reference to the atomspace containing
 * all of the atoms, resulting in data loss, unless you've done
 * something to keep ahold of that CID.
 *
 * This routine is meant to be used only for running test cases.
 * It is extremely dangerous, as it can lead to total data loss.
 */
void IPFSAtomStorage::kill_data(void)
{
	rethrow();

	_guid_map.clear();
	_atom_cid_map.clear();
	_guid_inv_map.clear();
	_json_map.clear();

	std::string text = "AtomSpace " + _uri;
	ipfs::Json result;

	ipfs::Client* conn = conn_pool.pop();
	conn->FilesAdd({{"AtomSpace",
		ipfs::http::FileUpload::Type::kFileContents,
		text}}, &result);
	conn_pool.push(conn);

	_atomspace_cid = result[0]["hash"];

	// Special case for TruthValues - must always have this atom.
	do_store_single_atom(tvpred);
}

/* ================================================================ */

void IPFSAtomStorage::set_hilo_watermarks(int hi, int lo)
{
	_write_queue.set_watermarks(hi, lo);
}

void IPFSAtomStorage::set_stall_writers(bool stall)
{
	_write_queue.stall(stall);
}

void IPFSAtomStorage::clear_stats(void)
{
	_stats_time = time(0);
	_load_count = 0;
	_store_count = 0;
	_valuation_stores = 0;
	_value_stores = 0;

	_write_queue.clear_stats();

	_num_get_atoms = 0;
	_num_got_nodes = 0;
	_num_got_links = 0;
	_num_get_insets = 0;
	_num_get_inlinks = 0;
	_num_node_inserts = 0;
	_num_link_inserts = 0;
	_num_atom_removes = 0;
	_num_atom_deletes = 0;
}

void IPFSAtomStorage::print_stats(void)
{
	printf("ipfs-stats: Currently open URI: %s\n", _uri.c_str());
	printf("ipfs-stats: IPNS name: /ipns/%s\n", _key_cid.c_str());
	printf("ipfs-stats: curr CID : /ipfs/%s\n", _atomspace_cid.c_str());
	time_t now = time(0);
	// ctime returns string with newline at end of it.
	printf("ipfs-stats: Time since stats reset=%lu secs, at %s",
		now - _stats_time, ctime(&_stats_time));


	size_t load_count = _load_count;
	size_t store_count = _store_count;
	double frac = store_count / ((double) load_count);
	printf("ipfs-stats: total loads = %zu total stores = %zu ratio=%f\n",
	       load_count, store_count, frac);

	size_t valuation_stores = _valuation_stores;
	size_t value_stores = _value_stores;
	printf("ipfs-stats: valuation updates = %zu value updates = %zu\n",
	       valuation_stores, value_stores);

	size_t num_atom_removes = _num_atom_removes;
	size_t num_atom_deletes = _num_atom_deletes;
	printf("ipfs-stats: atom remove requests = %zu total atom deletes = %zu\n",
	       num_atom_removes, num_atom_deletes);
	printf("\n");

	size_t num_get_atoms = _num_get_atoms;
	size_t num_got_nodes = _num_got_nodes;
	size_t num_got_links = _num_got_links;
	size_t num_get_insets = _num_get_insets;
	size_t num_get_inlinks = _num_get_inlinks;
	size_t num_node_inserts = _num_node_inserts;
	size_t num_link_inserts = _num_link_inserts;

	printf("num_get_atoms=%zu num_got_nodes=%zu num_got_links=%zu\n",
	       num_get_atoms, num_got_nodes, num_got_links);

	frac = num_get_inlinks / ((double) num_get_insets);
	printf("num_get_incoming_sets=%zu set total=%zu avg set size=%f\n",
	       num_get_insets, num_get_inlinks, frac);

	unsigned long tot_node = num_node_inserts;
	unsigned long tot_link = num_link_inserts;
	frac = tot_link / ((double) tot_node);
	printf("total stores for node=%lu link=%lu ratio=%f\n",
	       tot_node, tot_link, frac);

	// Store queue performance
	unsigned long item_count = _write_queue._item_count;
	unsigned long duplicate_count = _write_queue._duplicate_count;
	unsigned long flush_count = _write_queue._flush_count;
	unsigned long drain_count = _write_queue._drain_count;
	unsigned long drain_msec = _write_queue._drain_msec;
	unsigned long drain_slowest_msec = _write_queue._drain_slowest_msec;
	unsigned long drain_concurrent = _write_queue._drain_concurrent;
	int high_water = _write_queue.get_high_watermark();
	int low_water = _write_queue.get_low_watermark();
	bool stalling = _write_queue.stalling();

	double dupe_frac = duplicate_count / ((double) (item_count - duplicate_count));
	double flush_frac = (item_count - duplicate_count) / ((double) flush_count);
	double fill_frac = (item_count - duplicate_count) / ((double) drain_count);

	unsigned long dentries = drain_count + drain_concurrent;
	double drain_ratio = dentries / ((double) drain_count);
	double drain_secs = 0.001 * drain_msec / ((double) dentries);
	double slowest = 0.001 * drain_slowest_msec;

	printf("\n");
	printf("hi-water=%d low-water=%d stalling=%s\n", high_water,
	       low_water, stalling? "true" : "false");
	printf("write items=%lu dup=%lu dupe_frac=%f flushes=%lu flush_ratio=%f\n",
	       item_count, duplicate_count, dupe_frac, flush_count, flush_frac);
	printf("drains=%lu fill_fraction=%f concurrency=%f\n",
	       drain_count, fill_frac, drain_ratio);
	printf("avg drain time=%f seconds; longest drain time=%f\n",
	       drain_secs, slowest);

	printf("currently in_drain=%d num_busy=%lu queue_size=%lu\n",
	       _write_queue._in_drain, _write_queue.get_busy_writers(),
	       _write_queue.get_size());

	printf("current conn_pool free=%u of %d\n", conn_pool.size(),
	       _initial_conn_pool_size);

	printf("\n");
}

/* ============================= END OF FILE ================= */
