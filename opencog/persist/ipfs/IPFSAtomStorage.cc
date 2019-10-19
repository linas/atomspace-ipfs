/*
 * IPFSAtomStorage.cc
 * Persistent Atom storage, IPFS-backed.
 *
 * Atoms and Values are saved to, and restored from, an IPFS DB using
 * one of the available database drivers. Currently, the postgres
 * native libpq-dev API and the ODBC API are supported. Note that
 * libpq-dev is about three times faster than ODBC.
 *
 * Atoms are identified by means of unique ID's (UUID's), which are
 * correlated with specific in-RAM atoms via the TLB.
 *
 * Copyright (c) 2008,2009,2013,2015,2017 Linas Vepstas <linas@linas.org>
 *
 * LICENSE:
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <stdlib.h>
#include <unistd.h>

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atomspaceutils/TLB.h>

#include "IPFSAtomStorage.h"

using namespace opencog;

/* ================================================================ */
// Constructors

void IPFSAtomStorage::init(const char * uri)
{
	_uri = uri;

	if (1)
		throw IOException(TRACE_INFO, "Unknown URI '%s'\n", uri);

	max_height = 0;
	bulk_load = false;
	bulk_store = false;
	clear_stats();

	if (!connected()) return;

	// Special-case for TruthValues
	tvpred = doGetNode(PREDICATE_NODE, "*-TruthValueKey-*");
	if (nullptr == tvpred)
	{
		tvpred = createNode(PREDICATE_NODE, "*-TruthValueKey-*");
		do_store_single_atom(tvpred, 0);
	}

	// Special case for the pre-defined atomspaces.
	table_id_cache.insert(1);
}

// Number of write-back queues
#define NUM_WB_QUEUES 6

IPFSAtomStorage::IPFSAtomStorage(std::string uri) :
	_tlbuf(&_uuid_manager),
	_uuid_manager("uuid_pool"),
	_vuid_manager("vuid_pool"),
	_write_queue(this, &IPFSAtomStorage::vdo_store_atom, NUM_WB_QUEUES),
	_async_write_queue_exception(nullptr)
{
	init(uri.c_str());
}

IPFSAtomStorage::~IPFSAtomStorage()
{
#if 0
	flushStoreQueue();

	while (not conn_pool.is_empty())
	{
		LLConnection* db_conn = conn_pool.pop();
		delete db_conn;
	}
#endif
}

/**
 * connected -- return true if a successful connection to the
 * database exists; else return false.  Note that this may block,
 * if all database connections are in use...
 */
bool IPFSAtomStorage::connected(void)
{
	return false;
}

/// Rethrow asynchronous exceptions caught during atom storage.
///
/// Atoms are stored asynchronously, from a write queue, from some
/// other thread. If that thread has an exception, e.g. due to some
/// IPFS error, and the exception is uncaught, then the process will
/// die. So we have to catch that exception.  Once caught, what do
/// we do with it? Well, we culd ignore it, but then the user would
/// not know that the IPFS backend was damaged. So, instead, we throw
/// it at the first user, any user that is doing soem other IPFS stuff.
void IPFSAtomStorage::rethrow(void)
{
#if 0
	if (_async_write_queue_exception)
	{
		std::exception_ptr exptr = _async_write_queue_exception;
		_async_write_queue_exception = nullptr;
		std::rethrow_exception(exptr);
	}
#endif
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
/// The second issue is more serious: there's no fence or barrier in
/// Postgres (that I can find or think of), and so although we've sent
/// everything to PG, there's no guarantee that PG will process these
/// requests in order. How likely this could be, I don't know.
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
}

/* ================================================================ */

/**
 * kill_data -- destroy data in the database!! Dangerous !!
 * This routine is meant to be used only for running test cases.
 * It is extremely dangerous, as it can lead to total data loss.
 */
void IPFSAtomStorage::kill_data(void)
{
	rethrow();

	// Special case for TruthValues - must always have this atom.
	do_store_single_atom(tvpred, 0);

	throw RuntimeException(TRACE_INFO, "Not Implemented!\n");
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

	_num_get_nodes = 0;
	_num_got_nodes = 0;
	_num_rec_nodes = 0;
	_num_get_links = 0;
	_num_got_links = 0;
	_num_rec_links = 0;
	_num_get_insets = 0;
	_num_get_inlinks = 0;
	_num_node_inserts = 0;
	_num_link_inserts = 0;
	_num_atom_removes = 0;
	_num_atom_deletes = 0;
}

void IPFSAtomStorage::print_stats(void)
{
	printf("sql-stats: Currently open URI: %s\n", _uri.c_str());
	time_t now = time(0);
	// ctime returns string with newline at end of it.
	printf("sql-stats: Time since stats reset=%lu secs, at %s",
		now - _stats_time, ctime(&_stats_time));


	size_t load_count = _load_count;
	size_t store_count = _store_count;
	double frac = store_count / ((double) load_count);
	printf("sql-stats: total loads = %zu total stores = %zu ratio=%f\n",
	       load_count, store_count, frac);

	size_t valuation_stores = _valuation_stores;
	size_t value_stores = _value_stores;
	printf("sql-stats: valuation updates = %zu value updates = %zu\n",
	       valuation_stores, value_stores);

	size_t num_atom_removes = _num_atom_removes;
	size_t num_atom_deletes = _num_atom_deletes;
	printf("sql-stats: atom remove requests = %zu total atom deletes = %zu\n",
	       num_atom_removes, num_atom_deletes);
	printf("\n");

	size_t num_get_nodes = _num_get_nodes;
	size_t num_got_nodes = _num_got_nodes;
	size_t num_rec_nodes = _num_rec_nodes;
	size_t num_get_links = _num_get_links;
	size_t num_got_links = _num_got_links;
	size_t num_rec_links = _num_rec_links;
	size_t num_get_insets = _num_get_insets;
	size_t num_get_inlinks = _num_get_inlinks;
	size_t num_node_inserts = _num_node_inserts;
	size_t num_link_inserts = _num_link_inserts;

	frac = 100.0 * num_got_nodes / ((double) num_get_nodes);
	printf("num_get_nodes=%zu num_got_nodes=%zu (%f pct) recursive=%zu\n",
	       num_get_nodes, num_got_nodes, frac, num_rec_nodes);

	frac = 100.0 * num_got_links / ((double) num_get_links);
	printf("num_get_links=%zu num_got_links=%zu (%f pct) recursive=%zu\n",
	       num_get_links, num_got_links, frac, num_rec_links);

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

	// Some basic TLB statistics; could be improved;
	// The TLB remapping theory needs some work...
	// size_t noh = 0;
	// size_t remap = 0;

	UUID mad = getMaxObservedUUID();
#if DONT_COUNT
	This loop can lead to an apparent hang, when max UUID gets
	// above a quarter-billion or so.  So don't do this.
	for (UUID uuid = 1; uuid < mad; uuid++)
	{
		Handle h = _tlbuf.getAtom(uuid);
		if (nullptr == h) { noh++; continue; }

#if 0
		Handle hr = as->get_atom(h);
		if (nullptr == hr) { extra++; continue; }
		if (hr != h) { remap++; }
#endif
	}
#endif

	printf("\n");
	printf("sql-stats: tlbuf holds %lu atoms\n", _tlbuf.size());
#if 0
	frac = 100.0 * extra / ((double) _tlbuf.size());
	printf("sql-stats: tlbuf holds %lu atoms not in atomspace (%f pct)\n",
	        extra, frac);

	frac = 100.0 * remap / ((double) _tlbuf.size());
	printf("sql-stats: tlbuf holds %lu unremapped atoms (%f pct)\n",
	       remap, frac);
#endif

	size_t used = _tlbuf.size();
	frac = 100.0 * used / ((double) mad);
	printf("sql-stats: %zu of %lu reserved uuids used (%f pct)\n",
	       used, mad, frac);
}

/* ============================= END OF FILE ================= */
