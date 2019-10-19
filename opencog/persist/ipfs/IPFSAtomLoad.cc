/*
 * IPFSAtomLoad.cc
 * Restore of individual atoms.
 *
 * Copyright (c) 2008,2009,2013,2017,2019 Linas Vepstas <linas@linas.org>
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

#define BUFSZ 250

/* ================================================================ */

Handle IPFSAtomStorage::getNode(Type t, const char * str)
{
	rethrow();
	throw RuntimeException (TRACE_INFO, "Not implemented\n");
	Handle h; // (doGetNode(t, str));
	if (h) get_atom_values(h);
	return h;
}

Handle IPFSAtomStorage::getLink(Type t, const HandleSeq& hs)
{
	rethrow();
	throw RuntimeException (TRACE_INFO, "Not implemented\n");
	Handle hg; // (doGetLink(t, hs));
	if (hg) get_atom_values(hg);
	return hg;
}

/* ============================= END OF FILE ================= */
