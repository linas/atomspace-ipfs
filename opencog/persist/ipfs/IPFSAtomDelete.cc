/*
 * IPFSAtomDelete.cc
 * Deletion of individual atoms.
 *
 * Copyright (c) 2017,2019 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <stdlib.h>
#include <unistd.h>

#include <opencog/atoms/base/Atom.h>
#include <opencog/atoms/atom_types/NameServer.h>
#include <opencog/atoms/base/Link.h>
#include <opencog/atoms/base/Node.h>
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
	flushStoreQueue();

	// First, look it up.
	std::string name = h->to_short_string();
	std::string path = _atomspace_cid + "/" + name;
	ipfs::Json dag;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagGet(path, &dag);
	conn_pool.push(conn);

	auto iset = dag["incoming"];

	// Fail if a non-trivial incoming set.
	if (not recursive and 0 < iset.size()) return;

	// We're recursive; so recurse.
	for (auto acid: iset)
	{
		removeAtom(fetch_atom(acid), true);
	}

	// Now actually remove.
	std::string new_as_id;
	conn = conn_pool.pop();
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
	conn_pool.push(conn);

	// Bug with stats: should not increment on recursion.
	_num_atom_deletes++;
	_num_atom_removes++;
}

/* ============================= END OF FILE ================= */
