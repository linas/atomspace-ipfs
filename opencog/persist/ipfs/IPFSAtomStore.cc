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
#include <opencog/atomspaceutils/TLB.h>

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
 * completed (sent to the Postgres server) before this method returns.
 */
void IPFSAtomStorage::storeAtom(const Handle& h, bool synchronous)
{
	rethrow();

	// If a synchronous store, avoid the queues entirely.
	if (synchronous)
	{
		if (not_yet_stored(h)) do_store_atom(h);
		store_atom_values(h);
		return;
	}

	// _write_queue.enqueue(h);
	_write_queue.insert(h);
}

/**
 * Synchronously store a single atom. That is, the actual store is done
 * in the calling thread.  All values attached to the atom are also
 * stored.
 *
 * Returns the height of the atom.
 */
void IPFSAtomStorage::do_store_atom(const Handle& h)
{
	if (not not_yet_stored(h)) return;

	if (h->is_node())
	{
		do_store_single_atom(h);
		return;
	}

	// Recurse.
	for (const Handle& ho: h->getOutgoingSet())
		do_store_atom(ho);

	do_store_single_atom(h);
}

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

bool IPFSAtomStorage::not_yet_stored(const Handle& h)
{
	std::lock_guard<std::mutex> lck(_cid_mutex);
	return _ipfs_cid_map.end() == _ipfs_cid_map.find(h);
}

/* ================================================================ */

/// Convert a single C++ Atom into a Json expression.
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
			oset[i] = _ipfs_cid_map.find(hout)->second;
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
 * The store is performed synchronously (in the calling thread).
 */
void IPFSAtomStorage::do_store_single_atom(const Handle& h)
{
	// Convert C++ Atom to json.
	ipfs::Json atom = encodeAtomToJSON(h);

	// XXX FIXME If ipfs throws, then this leaks from the pool
	// We can't just catch here, we need to re-throw too.
	ipfs::Json result;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagPut(atom, &result);
	conn_pool.push(conn);

	std::string id = result["Cid"]["/"];

	{
		// This is multi-threaded; update the table under a lock.
		std::lock_guard<std::mutex> lck(_cid_mutex);
		_ipfs_cid_map.insert({h, id});
	}

	// OK, the atom itself is in IPFS; add it to the atomspace, too.
	std::string name = encodeValueToStr(h);
	add_atom_key_to_atomspace(name, "");

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
