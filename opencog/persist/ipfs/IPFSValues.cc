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
#include <opencog/atoms/truthvalue/TruthValue.h>

#include "IPFSAtomStorage.h"

using namespace opencog;

/* ================================================================== */

/// Delete the valuation, if it exists. This is required, in order
/// to prevent garbage from accumulating in the Values table.
/// It also simplifies, ever-so-slightly, the update of valuations.
void IPFSAtomStorage::deleteValuation(const Handle& key, const Handle& atom)
{
	throw SyntaxException(TRACE_INFO, "Not Implemented!");
}

/**
 * Store a valuation. Return an integer ID for that valuation.
 * Thread-safe.
 */
void IPFSAtomStorage::storeValuation(const ValuationPtr& valn)
{
	storeValuation(valn->key(), valn->atom(), valn->value());
}

void IPFSAtomStorage::storeValuation(const Handle& key,
                                    const Handle& atom,
                                    const ValuePtr& pap)
{
	throw SyntaxException(TRACE_INFO, "Not Implemented!");

	_valuation_stores++;
}

// Almost a cut-n-paste of the above, but different.
IPFSAtomStorage::VUID IPFSAtomStorage::storeValue(const ValuePtr& pap)
{
	throw SyntaxException(TRACE_INFO, "Not Implemented!");

	_value_stores++;
	return 0;
}

/// Return a value, given by the VUID identifier, taken from the
/// Values table. If the value type is a link, then the full recursive
/// fetch is performed.
ValuePtr IPFSAtomStorage::getValue(VUID vuid)
{
	throw SyntaxException(TRACE_INFO, "Not Implemented!");
}

/// Return a value, given by the key-atom pair.
/// If the value type is a link, then the full recursive
/// fetch is performed.
ValuePtr IPFSAtomStorage::getValuation(const Handle& key,
                                      const Handle& atom)
{
	throw SyntaxException(TRACE_INFO, "Not Implemented!");
}

void IPFSAtomStorage::deleteValue(VUID vuid)
{
	throw SyntaxException(TRACE_INFO, "Not Implemented!");
}

/// Store ALL of the values associated with the atom.
void IPFSAtomStorage::store_atom_values(const Handle& atom)
{
	HandleSet keys = atom->getKeys();
	for (const Handle& key: keys)
	{
		ValuePtr pap = atom->getValue(key);
		storeValuation(key, atom, pap);
	}

	// Special-case for TruthValues. Can we get rid of this someday?
	// Delete default TV's, else storage will get clogged with them.
	TruthValuePtr tv(atom->getTruthValue());
	if (tv->isDefaultTV()) deleteValuation(tvpred, atom);
}

/// Get ALL of the values associated with an atom.
void IPFSAtomStorage::get_atom_values(Handle& atom)
{
	if (nullptr == atom) return;

	throw SyntaxException(TRACE_INFO, "Not Implemented!");
}

/* ================================================================ */

void IPFSAtomStorage::getValuations(AtomTable& table,
                                   const Handle& key, bool get_all_values)
{
	rethrow();
	throw SyntaxException(TRACE_INFO, "Not Implemented!");
}

/* ============================= END OF FILE ================= */
