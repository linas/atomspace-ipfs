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

Handle IPFSAtomStorage::fetch_atom(const std::string& cid)
{
	rethrow();
	Handle h(doFetchAtom(cid));
	if (h) get_atom_values(h);
	return h;
}

Handle IPFSAtomStorage::doFetchAtom(const std::string& cid)
{
	ipfs::Json dag;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagGet(cid, &dag);
	conn_pool.push(conn);

	std::cout << "The DAG is:" << dag.dump(2) << std::endl;

	_num_get_atoms++;
	return decodeJSONAtom(dag);
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
	}
	Handle h;
	return h;
}

/// Convert a scheme expression into a C++ Atom.
/// For example: `(Concept "foobar")`  or
/// `(Evaluation (Predicate "blort") (List (Concept "foo") (Concept "bar")))`
/// will return the corresponding atoms.
///
Handle IPFSAtomStorage::decodeSCMAtom(const std::string& satom)
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
		oset.push_back(decodeSCMAtom(&scm[oset_start]));

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
