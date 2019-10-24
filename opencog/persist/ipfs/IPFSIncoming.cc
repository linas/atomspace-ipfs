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

/// Store `holder` into the incoming set of atom.
void IPFSAtomStorage::store_incoming_of(const Handle& atom,
                                        const Handle& holder)
{
	// No publication of Incoming Set, if there's no AtomSpace key.
	if (0 == _keyname.size()) return;

	// Obtain the JSON representation of the Atom.
	ipfs::Json jatom;
	{
		std::lock_guard<std::mutex> lck(_json_mutex);
		jatom = _json_map.find(atom)->second;
	}

	ipfs::Json jinco;
	auto incli = jatom.find("incoming");
	if (jatom.end() != incli)
	{
		// Is the atom already a part of the incoming set?
		// If so, then there's nothing to do.
		auto havit = incli->find(get_atom_guid(holder));
		if (incli->end() != havit) return;
		jinco = *incli;
	}

	jinco.push_back(get_atom_guid(holder));
	jatom["incoming"] = jinco;

	// Store the thing in IPFS
	ipfs::Json result;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagPut(jatom, &result);
	conn_pool.push(conn);

	std::string atoid = result["Cid"]["/"];
	std::cout << "Incoming Atom: " << encodeAtomToStr(atom)
	          << " CID: " << atoid << std::endl;

	update_atom_in_atomspace(atom, atoid, jatom);
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
