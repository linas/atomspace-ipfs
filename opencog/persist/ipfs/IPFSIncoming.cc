/*
 * IPFSIncomin.cc
 * Save and restore of atom incoming set.
 *
 * Copyright (c) 2008,2009,2013,2017,2019 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <stdlib.h>
#include <unistd.h>

#include <opencog/atoms/base/Atom.h>
#include <opencog/atoms/atom_types/NameServer.h>

#include "IPFSAtomStorage.h"

using namespace opencog;

/* ================================================================== */

/// Get the IncomingSet of the Atom, and return the corresponding
/// JSON representation for it.  This object will have CID's to
/// the Atoms in that incoming set, those CID's will identify the
/// globally-unique atoms, and NOT the Atoms w/ values, etc.
ipfs::Json IPFSAtomStorage::encodeIncomingToJSON(const Handle& atom)
{
	ipfs::Json jinco;

	IncomingSet iset(atom->getIncomingSet());

	for (const LinkPtr& lnk: iset)
	{
		jinco.push_back(get_atom_cid(HandleCast(lnk)));
	}
	return jinco;
}

/* ================================================================== */

/// Store ALL of the incoming set associated with the atom.
void IPFSAtomStorage::store_atom_incoming(const Handle& atom)
{
	// No publication of Incoming Set, if there's no AtomSpace key.
	if (0 == _keyname.size()) return;

	// Build a JSON representation of the Atom.
	ipfs::Json jatom = encodeAtomToJSON(atom);
	jatom["incoming"] = encodeIncomingToJSON(atom);

	// Store the thing in IPFS
	ipfs::Json result;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagPut(jatom, &result);
	conn_pool.push(conn);

	std::string atoid = result["Cid"]["/"];
	std::cout << "Incoming Atom: " << encodeValueToStr(atom)
	          << " CID: " << atoid << std::endl;
}

/* ================================================================ */
/**
 * Retreive the incoming set of the indicated atom.
 */
void IPFSAtomStorage::getIncoming(AtomTable& table, const char *buff)
{
	throw SyntaxException(TRACE_INFO, "Not Implemented!\n");
	// Performance stats
#if 0
	_num_get_insets++;
	_num_get_inlinks += iset.size();
#endif
}

/**
 * Retreive the entire incoming set of the indicated atom.
 */
void IPFSAtomStorage::getIncomingSet(AtomTable& table, const Handle& h)
{
	rethrow();

	throw SyntaxException(TRACE_INFO, "Not Implemented!\n");
}

/**
 * Retreive the incoming set of the indicated atom, but only those atoms
 * of type t.
 */
void IPFSAtomStorage::getIncomingByType(AtomTable& table, const Handle& h, Type t)
{
	rethrow();

	throw SyntaxException(TRACE_INFO, "Not Implemented!\n");
}

/* ============================= END OF FILE ================= */
