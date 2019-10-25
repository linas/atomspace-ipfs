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
 * Retreive the entire incoming set of the indicated atom.
 * This fetches only teh Atoms in the incoming set, and not
 * the Values associated with them!  XXX FIXME Double check,
 * I think the intent is to also get the attached values,
 * but I'm not sure, because the spec is ill-defined on this
 * and I'm not sure what postgres is going, here.  All backends
 * should be compatible on this.
 */
void IPFSAtomStorage::getIncomingSet(AtomTable& table, const Handle& h)
{
	rethrow();

	std::string path = _atomspace_cid + "/" + h->to_short_string();

	ipfs::Json dag;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagGet(path, &dag);
	conn_pool.push(conn);
	// std::cout << "The dag is:" << dag.dump(2) << std::endl;

	auto iset = dag["incoming"];
	for (auto acid: iset)
	{
		// std::cout << "The incoming is:" << acid.dump(2) << std::endl;
		table.add(fetch_atom(acid), false);
	}

	_num_get_insets++;
	_num_get_inlinks += iset.size();
}

/**
 * Retreive the incoming set of the indicated atom, but only those atoms
 * of type t.
 */
void IPFSAtomStorage::getIncomingByType(AtomTable& table, const Handle& h, Type t)
{
	rethrow();

	// Code is almost same as above. It's not terribly efficient.
	// But it works, at least.
	std::string path = _atomspace_cid + "/" + h->to_short_string();

	ipfs::Json dag;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagGet(path, &dag);
	conn_pool.push(conn);

	auto iset = dag["incoming"];
	for (auto acid: iset)
	{
		Handle h(fetch_atom(acid));
		if (t == h->get_type())
		{
			table.add(h, false);
			_num_get_inlinks ++;
		}
	}

	_num_get_insets++;
}

/* ============================= END OF FILE ================= */
