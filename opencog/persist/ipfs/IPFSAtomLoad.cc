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

/* ================================================================ */

/// Fetch the indicated atom from the IPFS CID.
/// This will return the raw JSON representation.
ipfs::Json IPFSAtomStorage::fetch_atom_dag(const std::string& cid)
{
	rethrow();

	ipfs::Json dag;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagGet(cid, &dag);
	conn_pool.push(conn);
	_num_get_atoms++;

	// std::cout << "Fetched the DAG:" << dag.dump(2) << std::endl;
	return dag;
}

/* ================================================================ */

/// Fetch the indicated atom from the IPFS CID.
/// This will also grab and decode values, if present.
Handle IPFSAtomStorage::fetch_atom(const std::string& cid)
{
	ipfs::Json dag(fetch_atom_dag(cid));
	Handle h(decodeJSONAtom(dag));
	get_atom_values(h, dag);

	// Update the local json cache.
	// XXX FIXME well, we *should* do this here, except that we
	// don't have the AtomSpace version of the handle yet.
	// We want to use that. So maybe later...

	return h;
}

/// Convert a JSON message into a C++ Atom
/// For example:
///   {
///      "name": "example concept",
///      "type": "ConceptNode"
///   }
/// is obviously a ConceptNode

Handle IPFSAtomStorage::decodeJSONAtom(const ipfs::Json& atom)
{
	Type t = nameserver().getType(atom["type"]);
	if (nameserver().isNode(t))
	{
		_num_got_nodes ++;
		return createNode(t, atom["name"]);
	}

	if (not nameserver().isLink(t))
		throw RuntimeException(TRACE_INFO, "Bad Atom JSON! %s\n", atom.dump(2));

	// The json representation for outgoing always holds guid
	// for the atom (i.e. the atom without values on it) and
	// never the CID (the atom with values on it).
	HandleSeq oset;
	for (const std::string& guid: atom["outgoing"])
	{
		// This is multi-threaded; access under a lock
		std::unique_lock<std::mutex> lck(_inv_mutex);
		auto hiter = _guid_inv_map.find(guid);
		lck.unlock();
		if (_guid_inv_map.end() == hiter)
		{
			Handle hout(fetch_atom(guid));
			oset.push_back(hout);
			std::lock_guard<std::mutex> lck(_inv_mutex);
			_guid_inv_map.insert({guid, hout});
		}
		else
			oset.push_back(hiter->second);
	}

	_num_got_links ++;
	return createLink(oset, t);
}

/// Convert a scheme expression into a C++ Atom.
/// For example: `(Concept "foobar")`  or
/// `(Evaluation (Predicate "blort") (List (Concept "foo") (Concept "bar")))`
/// will return the corresponding atoms.
///
Handle IPFSAtomStorage::decodeStrAtom(const std::string& satom)
{
	std::string scm = satom;
	if ('(' != scm[0])
		throw RuntimeException(TRACE_INFO, "Bad Atom string! %s\n", scm.c_str());
	size_t pos = scm.find(' ');
	if (pos == std::string::npos)
		throw RuntimeException(TRACE_INFO, "Bad Atom string! %s\n", scm.c_str());
	scm[pos] = 0;

	Type t = nameserver().getType(&scm[1]);
	if (nameserver().isNode(t))
	{
		size_t name_start = scm.find('"', pos+1);
		if (name_start == std::string::npos)
			throw RuntimeException(TRACE_INFO, "Bad Atom string! %s\n", scm.c_str());
		size_t name_end = scm.find('"', name_start+1);
		if (name_end == std::string::npos)
			throw RuntimeException(TRACE_INFO, "Bad Atom string! %s\n", scm.c_str());
		scm[name_end] = 0;
		_num_got_nodes ++;
		return createNode(t, &scm[name_start+1]);
	}

	if (not nameserver().isLink(t))
		throw RuntimeException(TRACE_INFO, "Bad Atom string! %s\n", scm.c_str());

	// If we are here, its a Link.
	HandleSeq oset;
	size_t oset_start = scm.find('(', pos+1);
	while (oset_start != std::string::npos)
	{
		oset.push_back(decodeStrAtom(&scm[oset_start]));

		// Find the next balanced paren, and restart there.
		// This is not very efficient, but it works.
		size_t pos = oset_start;
		int pcnt = 1;
		while (0 < pcnt and pos != std::string::npos)
		{
			char c = scm[++pos];
			if ('(' == c) pcnt ++;
			else if (')' == c) pcnt--;
		}
		if (pos == std::string::npos) break;
		oset_start = scm.find('(', pos+1);
	}

	_num_got_links ++;
	return createLink(oset, t);
}

/// Convert value (or Atom) into a string.
std::string IPFSAtomStorage::encodeValueToStr(const ValuePtr& v)
{
	if (nameserver().isA(v->get_type(), FLOAT_VALUE))
	{
		// The FloatValue to_string() print prints out a high-precision
		// form of the value, as compared to SimpleTruthValue, which
		// only prints 6 digits and breaks the unit tests.
		FloatValuePtr fv(FloatValueCast(v));
		return fv->FloatValue::to_string();
	}
	return v->to_short_string();
}

/* ================================================================ */

Handle IPFSAtomStorage::do_fetch_atom(Handle &h)
{
	ipfs::Json dag = get_atom_json(h);

	if (0 == dag.size())
	{
		// If we are here, its because there was no such Atom
		// recorded in IPFS. That's a normal situation, just
		// ignore the error.
		return Handle();
	}

	// std::cout << "The dag is:" << dag.dump(2) << std::endl;
	get_atom_values(h, dag);
	return h;
}

Handle IPFSAtomStorage::getNode(Type t, const char * str)
{
	rethrow();
	Handle h(createNode(t, str));
	return do_fetch_atom(h);
}

Handle IPFSAtomStorage::getLink(Type t, const HandleSeq& hs)
{
	rethrow();
	Handle h(createLink(hs, t));
	return do_fetch_atom(h);
}

/* ============================= END OF FILE ================= */
