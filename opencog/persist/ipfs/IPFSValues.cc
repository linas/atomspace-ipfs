/*
 * IPFSValues.cc
 * Save and restore of atom values.
 *
 * Copyright (c) 2008,2009,2013,2017,2019 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <stdlib.h>
#include <unistd.h>

#include <opencog/atoms/base/Atom.h>
#include <opencog/atoms/atom_types/NameServer.h>
#include <opencog/atoms/value/FloatValue.h>
#include <opencog/atoms/value/LinkValue.h>
#include <opencog/atoms/value/StringValue.h>
#include <opencog/atoms/base/Valuation.h>
#include <opencog/atoms/truthvalue/SimpleTruthValue.h>
#include <opencog/atoms/truthvalue/TruthValue.h>

#include "IPFSAtomStorage.h"

using namespace opencog;

/* ================================================================== */

/// Get ALL of the values on the Atom, and return the corresponding
/// JSON representation for them.
ipfs::Json IPFSAtomStorage::encodeValuesToJSON(const Handle& atom)
{
	// Build some json that encodes the key-value pairs
	ipfs::Json jvals;
	HandleSet keys = atom->getKeys();
	for (const Handle& key: keys)
	{
		// Special-case for TruthValues.  Avoid storing default TV's
		// so ast to not clog things up.
		if (key == tvpred)
		{
			TruthValuePtr tv(atom->getTruthValue());
			if (tv->isDefaultTV()) continue;
		}
		ValuePtr pap = atom->getValue(key);
		jvals[encodeAtomToStr(key)] = encodeValueToStr(pap);
	}
	return jvals;
}

/* ================================================================== */

/// Store ALL of the values associated with the atom.
void IPFSAtomStorage::store_atom_values(const Handle& atom)
{
	// No publication of Values, if there's no AtomSpace key.
	if (0 == _keyname.size()) return;

	// Build a JSON representation of the Atom.
	ipfs::Json jatom = encodeAtomToJSON(atom);
	ipfs::Json jvals = encodeValuesToJSON(atom);
	if (0 < jvals.size())
		jatom["values"] = jvals;

	// Store the thing in IPFS
	ipfs::Json result;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagPut(jatom, &result);
	conn_pool.push(conn);

	std::string atoid = result["Cid"]["/"];
	std::cout << "Valued Atom: " << encodeAtomToStr(atom)
	          << " CID: " << atoid << std::endl;

	// The code below is ifdefed out. In a better world, we would
	// publish just the IPNS name of where to find the atom values,
	// and, to get those values, we'd simply look them up in IPNS.
	// Doing it this way would avoid a need to update the master
	// AtomSpace file. However, we're not quite ready for that, yet.
	// So this is stubbed out, for now.
#if LATER_WHEN_IPNS_WORKS
	// Find the key for this atom.
	// XXX TODO this can be speeded up by caching the keys in C++
	std::string atonam = _keyname + encodeAtomToStr(atom);
	std::string atokey;
	conn = conn_pool.pop();
	conn->KeyFind(atonam, &atokey);
	if (0 == atokey.size())
	{
		// Not found; make a new one, by default.
		conn->KeyNew(atonam, &atokey);
		std::cout << "Generated Atom IPNS: " << atonam
		          << " key: " << atokey << std::endl;
	}

	// Publish... XXX FIXME needs to be in a queue.
	std::cout << "Publishing Atom Values: " << atokey << std::endl;

	// XXX hack alert -- lifetime set to 4 hours, it should be
	// infinity or something.... the TTL is 30 seconds, but should
	// be shorter or user-configurable .. set both with scheme bindings.
	try
	{
		// The `ipns_name` should be identical to the `atokey` above.
		std::string ipns_name;
		conn->NamePublish(atoid, atonam, &ipns_name, "4h", "30s");
		std::cout << "Published Atom Values: " << ipns_name << std::endl;
	}
	catch (const std::exception& ex)
	{
		// Arghh. IPNS keeps throwing this error:
		// "can't replace a newer value with an older value"
		// which is insane, because maybe I *do* want to do exactly
		// that!  So WTF ... another IPNS bug.
		std::cerr << "Failed to publish Atom Values: "
		          << ex.what() << std::endl;
	}
	conn_pool.push(conn);
#else // LATER_WHEN_IPNS_WORKS

   // Update the atomspace, so that it holds the new value.
   update_atom_in_atomspace(atom, atoid, jatom);
#endif // LATER_WHEN_IPNS_WORKS
}
/* ================================================================== */

/// Get ALL of the values associated with an atom.
void IPFSAtomStorage::get_atom_values(Handle& atom, const ipfs::Json& jatom)
{
	// If no values, then nothing to do.
	auto pvals = jatom.find("values");
	if (pvals == jatom.end()) return;

	ipfs::Json jvals = *pvals;
	// std::cout << "Jatom vals: " << jvals.dump(2) << std::endl;

	for (const auto& [jkey, jvalue]: jvals.items())
	{
		// std::cout << "KV Pair: " << jkey << " "<<jvalue<< std::endl;
		atom->setValue(decodeStrAtom(jkey), decodeStrValue(jvalue));
	}
}

/* ================================================================ */

ValuePtr IPFSAtomStorage::decodeStrValue(const std::string& stv)
{
	size_t pos = stv.find("(FloatValue ");
	if (std::string::npos != pos)
	{
		pos += strlen("(FloatValue ");
		std::vector<double> fv;
		while (pos != std::string::npos and stv[pos] != ')')
		{
			size_t epos;
			fv.push_back(stod(stv.substr(pos), &epos));
			pos += epos;
		}
		return createFloatValue(fv);
	}

	pos = stv.find("(SimpleTruthValue ");
	if (std::string::npos != pos)
	{
		size_t epos;
		pos += strlen("(SimpleTruthValue ");
		double strength = stod(stv.substr(pos), &epos);
		pos += epos;
		double confidence = stod(stv.substr(pos), &epos);
		return ValueCast(createSimpleTruthValue(strength, confidence));
	}
	throw SyntaxException(TRACE_INFO, "Unknown Value %s", stv.c_str());
}

/* ================================================================ */

/// Get all atoms having indicated key on them.
/// It appears that there are zero users of thi thing, because the
/// guile API for this was never created.  Should probably get rid
/// of this.
void IPFSAtomStorage::getValuations(AtomTable& table,
                                   const Handle& key, bool get_all_values)
{
	rethrow();
	throw SyntaxException(TRACE_INFO, "Not Implemented!");
}

/* ============================= END OF FILE ================= */
