/*
 * IPFSIncoming.cc
 * Save and restore of atom incoming set.
 *
 * Copyright (c) 2008,2009,2013,2017,2019 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <stdlib.h>

#include <opencog/atoms/base/Atom.h>

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

/* ================================================================== */

/// Remove `holder` from the incoming set of atom.
void IPFSAtomStorage::remove_incoming_of(const Handle& atom,
                                         const std::string& holder)
{
	std::cout << "Remove from " << atom->to_short_string()
	          << " inCID " << holder << std::endl;

	// XXX FIXME. This is wildly, insanely inefficient. First,
	// We get a list of all the Atoms in the current AtomSpace.
	// We need this, because we need to find the CID of the
	// Atom passed in.
	std::string path = _atomspace_cid;
	ipfs::Json lsres;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagGet(path, &lsres);
	conn_pool.push(conn);
	// std::cout << "Ls Result " << lsres.dump(2) << std::endl;

	// Now look for the CID of Atom
	std::string cid_of_atom;
	std::string name = atom->to_short_string();
	for (const auto& item : lsres["links"])
	{
		// std::cout << "Item " << item.dump(2) << std::endl;
		if (0 == name.compare(item["Name"]))
		{
			cid_of_atom = item["Cid"]["/"];
			break;
		}
	}
	std::cout << "Found current Atom: " << cid_of_atom << std::endl;

	if (0 == cid_of_atom.size())
		throw RuntimeException(TRACE_INFO,
			"Error: remove_incoming_of(): cannot find %s\n", name.c_str());

	// Now that we have the cid, get the Atom, so that we can
	// remove holder from it's incoming set.
	ipfs::Json jatom;
	conn = conn_pool.pop();
	conn->DagGet(cid_of_atom, &jatom);
	conn_pool.push(conn);
	std::cout << "The Atom:" << jatom.dump(2) << std::endl;

	// Remove the holder from the incoming set ...
	std::set<std::string> inco = jatom["incoming"];
	inco.erase(holder);
	if (0 < inco.size())
		jatom["incoming"] = inco;
	else
		jatom.erase("incoming");
	std::cout << "Atom after erasure:" << jatom.dump(2) << std::endl;

	// Store the edited Atom back into IPFS...
	ipfs::Json result;
	conn = conn_pool.pop();
	conn->DagPut(jatom, &result);
	conn_pool.push(conn);

	// Finally, update the Atomspace with this revised Atom.
	std::string atoid = result["Cid"]["/"];
	update_atom_in_atomspace(atom, atoid, jatom);

	// Phew. Done.
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
