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
/// TODO The deletion is performed synchronously, in single-threaded
/// fashion.  It might make sense to create a queue for this, so that
/// if could run in parallel (in the unusual case that someone has
/// millions of atoms to delete and is impateint about it...)
void IPFSAtomStorage::removeAtom(const Handle& h, bool recursive)
{
	// Synchronize. The atom that we are deleting might be sitting
	// in the store queue.
	flushStoreQueue();

	throw SyntaxException(TRACE_INFO, "Not Implemented!\n");
	_num_atom_deletes++;
	_num_atom_removes++;
}

/* ============================= END OF FILE ================= */
