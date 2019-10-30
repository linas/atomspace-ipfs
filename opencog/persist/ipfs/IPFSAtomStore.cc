/*
 * IPFSAtomStore.cc
 * Save of individual atoms.
 *
 * Copyright (c) 2008,2009,2013,2017,2019 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <stdlib.h>
#include <unistd.h>
#include <algorithm>

#include <opencog/atoms/base/Atom.h>
#include <opencog/atomspace/AtomSpace.h>

#include "IPFSAtomStorage.h"

using namespace opencog;

/* ================================================================ */
/**
 * Recursively store the indicated atom and all of the values attached
 * to it.  Also store it's outgoing set, and all of the values attached
 * to those atoms.  The recursive store is unconditional.
 *
 * By default, the actual store is done asynchronously (in a different
 * thread); this routine merely queues up the atom. If the synchronous
 * flag is set, then the store is performed in this thread, and it is
 * completed (sent to the IPFS daemon) before this method returns.
 */
void IPFSAtomStorage::storeAtom(const Handle& h, bool synchronous)
{
	rethrow();

	// If a synchronous store, avoid the queues entirely.
	if (synchronous)
	{
		if (guid_not_yet_stored(h)) do_store_atom(h);
		store_atom_values(h);
		return;
	}

	// _write_queue.enqueue(h);
	_write_queue.insert(h);
}

/**
 * Synchronously store a single globally-unique atom.
 * A "globally unique Atom" is the one without any attached values or
 * other mutable state. Thus, it posses a single globally unique CID.
 *
 * The store is synchronous, as it is done in the calling thread.
 * The intent is that the writeback queue is the one calling this
 * method.
 */
void IPFSAtomStorage::do_store_atom(const Handle& h)
{
	if (not guid_not_yet_stored(h)) return;

	if (h->is_node())
	{
		do_store_single_atom(h);
		return;
	}

	// Recurse.
	for (const Handle& ho: h->getOutgoingSet())
		do_store_atom(ho);

	do_store_single_atom(h);

	// Make note of the incoming set.
	for (const Handle& ho: h->getOutgoingSet())
		store_incoming_of(ho,h);
}

/// This method runs in the write-pool dispatcher thread.
/// That is, for each atom that was queued into the write queue,
/// when it gets dequeued, this method is called to store it.
/// Take careful note of the design here: the only things that
///
void IPFSAtomStorage::vdo_store_atom(const Handle& h)
{
	try
	{
		do_store_atom(h);
		store_atom_values(h);
	}
	catch (...)
	{
		_async_write_queue_exception = std::current_exception();
	}
}

bool IPFSAtomStorage::guid_not_yet_stored(const Handle& h)
{
	std::lock_guard<std::mutex> lck(_guid_mutex);
	return _guid_map.end() == _guid_map.find(h);
}

/* ================================================================ */

/// Convert a single C++ Atom into a json expression.
/// But only the minimalist Atom itself, and not any Values,
/// nor any of it's incoming set. So the resulting Json is
/// globally-unique.
ipfs::Json IPFSAtomStorage::encodeAtomToJSON(const Handle& h)
{
	// The minimalist Atom definition, as json.
	ipfs::Json jatom;
	jatom["type"] = nameserver().getTypeName(h->get_type());
	if (h->is_node())
	{
		jatom["name"] = h->get_name();
	}
	else
	if (h->is_link())
	{
		ipfs::Json oset;
		int i=0;
		for (const Handle& hout: h->getOutgoingSet())
		{
			oset[i] = get_atom_guid(hout);
			i++;
		}
		jatom["outgoing"] = oset;
	}
	else
	{
		throw RuntimeException(TRACE_INFO, "Neither Node nor Link!\n");
	}
	return jatom;
}

/* ================================================================ */

/**
 * Store just this one single atom.
 * Atoms in the outgoing set are NOT stored!
 * Values attached to the Atom are not stored!
 * The store is performed synchronously (in the calling thread).
 */
void IPFSAtomStorage::do_store_single_atom(const Handle& h)
{
	// Convert C++ Atom to json. But only the core, unique
	// Atom, and NOT the values! Nor the incoming set...
	ipfs::Json jatom = encodeAtomToJSON(h);

	// XXX FIXME If ipfs throws, then this leaks from the pool
	// We can't just catch here, we need to re-throw too.
	ipfs::Json result;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagPut(jatom, &result);
	conn_pool.push(conn);

	std::string guid = result["Cid"]["/"];

	// Record the guid once and forevermore.
	{
		std::lock_guard<std::mutex> lck(_guid_mutex);
		_guid_map[h] = guid;
	}
	{
		std::lock_guard<std::mutex> lck(_inv_mutex);
		_guid_inv_map[guid] = h;
	}

	// OK, the atom itself is in IPFS; add it to the atomspace, too.
	update_atom_in_atomspace(h, guid);

	// Cache the json, but only if we don't already have a version
	// of it. I guess that there is a very slight chance that some
	// other thread is racing, and maybe its twiddling the json
	// also. We don't want to clobber that with this earlier version.
	{
		std::lock_guard<std::mutex> lck(_json_mutex);
		if (_json_map.end() == _json_map.find(h))
			_json_map[h] = jatom;
	}

	// std::cout << "addAtom: " << name << " id: " << id << std::endl;

	_store_count ++;

	if (bulk_store and _store_count%100 == 0)
	{
		time_t secs = time(0) - bulk_start;
		double rate = ((double) _store_count) / secs;
		unsigned long kays = ((unsigned long) _store_count) / 1000;
		printf("\tStored %luK atoms in %d seconds (%d per second)\n",
			kays, (int) secs, (int) rate);
	}
}

/* ============================= END OF FILE ================= */
