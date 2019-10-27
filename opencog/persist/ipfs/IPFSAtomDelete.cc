/*
 * IPFSAtomDelete.cc
 * Deletion of individual atoms.
 *
 * Copyright (c) 2017,2019 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <stdlib.h>

#include <opencog/atoms/base/Atom.h>
#include <opencog/atomspace/AtomSpace.h>

#include "IPFSAtomStorage.h"

using namespace opencog;

/// Remove an atom, and all of it's associated values from the database.
/// If the atom has a non-empty incoming set, then it is NOT removed
/// unless the recursive flag is set. If the recursive flag is set, then
/// the atom, and everything in its incoming set is removed.
///
void IPFSAtomStorage::removeAtom(const Handle& h, bool recursive)
{
	// Synchronize. The atom that we are deleting might be sitting
	// in the store queue.
	// XXX FIXME -- we actually want to pause the queue, until this is
	// done, in order to make sure that the operations here are fully
	// serialized with atom stores. Perhaps the easiest way is to grab
	// some lock...Hmmm ... which lock? a recursive lock on
	// _atomspace_cid_mutex seems like it should do the trick...
	flushStoreQueue();

	ipfs::Json jatom;
#if DONT_CACHE_INSTEAD_GET_IT_FROM_IPFS
	// First, look it up.
	std::string name = h->to_short_string();
	std::string path = _atomspace_cid + "/" + name;
	ipfs::Client* conn = conn_pool.pop();

	// Delete can fail if Atom is not in the AtomSpace.
	// In this case, there's nothing to be done.
	try
	{
		conn->DagGet(path, &jatom);
	}
	catch (const std::exception &ex)
	{
		conn_pool.push(conn);
		return;
	}
	conn_pool.push(conn);

	// The jatom should be identical to what is in _json_map,
	// if our code is actually correct. We could test this here,
	// if we wanted to...
#else // DONT_CACHE_INSTEAD_GET_IT_FROM_IPFS
	{
		std::lock_guard<std::mutex> lck(_json_mutex);
		const auto& ptr = _json_map.find(h);
		// If might be not found, because it had never been
		// stored before. This is not an error.
		if (_json_map.end() == ptr) return;

		jatom = ptr->second;
	}
#endif // DONT_CACHE_INSTEAD_GET_IT_FROM_IPFS

	auto pinc = jatom.find("incoming");
	if (jatom.end() != pinc)
	{
		auto iset = *pinc; // iset = jatom["incoming"];

		// Fail if a non-trivial incoming set.
		if (not recursive and 0 < iset.size()) return;

		// We're recursive; so recurse.
		for (auto acid: iset)
		{
			Handle hin(fetch_atom(acid));
			removeAtom(hin, true);
		}
	}

	// Remove this atom from the incoming sets of those that
	// it contains.
	if (h->is_link())
	{
		for (const Handle& hoth: h->getOutgoingSet())
		{
			std::string acid;
			{
				std::lock_guard<std::mutex> lck(_guid_mutex);
				acid = _guid_map[hoth];
			}
			//if (0 == acid.size())
			//	throw RuntimeException(TRACE_INFO, "Error: missing CID for
			remove_incoming_of(hoth, acid);
		}
	}

	// Drop the atom from out caches
	{
		std::lock_guard<std::mutex> lck(_json_mutex);
		_json_map.erase(h);
	}
	{
		std::lock_guard<std::mutex> lck(_guid_mutex);
		_guid_map.erase(h);
	}

	// Now actually remove.
	std::string new_as_id;
	ipfs::Client* conn = conn_pool.pop();
	{
		std::string name = h->to_short_string();
		try
		{
			// Update the cid under a lock, as atomspace modifications
			// can occur from multiple threads.  It's not actually the
			// cid that matters, its the patch itself.
			std::lock_guard<std::mutex> lck(_atomspace_cid_mutex);
			conn->ObjectPatchRmLink(_atomspace_cid, name, &new_as_id);
			_atomspace_cid = new_as_id;
			std::cout << "Atomspace after removal of " << name
			          << " is " << _atomspace_cid << std::endl;
		}
		catch (const std::exception& ex)
		{
			std::cout << "Error: Atomspace " << _atomspace_cid
			          << " does not contain " << name << std::endl;

			conn_pool.push(conn);
			throw RuntimeException(TRACE_INFO,
				"Error: Atomsapce did not contain atom; how did that happen?\n");
		}
	}
	conn_pool.push(conn);

	// Bug with stats: should not increment on recursion.
	_num_atom_deletes++;
	_num_atom_removes++;
}

/* ============================= END OF FILE ================= */
