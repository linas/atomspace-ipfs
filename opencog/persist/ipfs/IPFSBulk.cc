/*
 * IPFSBulk.cc
 * Bulk save & restore of Atoms.
 *
 * Copyright (c) 2008,2009,2013,2017,2019 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <opencog/util/oc_assert.h>
#include <opencog/util/oc_omp.h>

#include <opencog/atoms/base/Atom.h>
#include <opencog/atoms/atom_types/NameServer.h>
#include <opencog/atoms/base/Link.h>
#include <opencog/atoms/base/Node.h>
#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atomspaceutils/TLB.h>

#include "IPFSAtomStorage.h"

using namespace opencog;

#define BUFSZ 120

/* ================================================================ */
/**
 * Retreive the incoming set of the indicated atom.
 */
void IPFSAtomStorage::getIncoming(AtomTable& table, const char *buff)
{
	throw SyntaxException(TRACE_INFO, "Not Implemented!\n");
#if 0
	// Parallelize always.
	opencog::setting_omp(NUM_OMP_THREADS, NUM_OMP_THREADS);

	// A parallel fetch is much much faster, esp for big osets.
	// std::for_each(std::execution::par_unseq, ... requires C++17
	OMP_ALGO::for_each(pset.begin(), pset.end(),
		[&] (const PseudoPtr& p)
	{
		Handle hi(get_recursive_if_not_exists(p));
		hi = table.add(hi, false);
		get_atom_values(hi);
		std::lock_guard<std::mutex> lck(iset_mutex);
		iset.emplace_back(hi);
	});

	// Performance stats
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

	char buff[BUFSZ];
	getIncoming(table, buff);
}

/**
 * Retreive the incoming set of the indicated atom, but only those atoms
 * of type t.
 */
void IPFSAtomStorage::getIncomingByType(AtomTable& table, const Handle& h, Type t)
{
	rethrow();

	throw SyntaxException(TRACE_INFO, "Not Implemented!\n");

	char buff[BUFSZ];
	getIncoming(table, buff);
}

/* ================================================================ */

void IPFSAtomStorage::load_atomspace(AtomSpace* as, const std::string& cid)
{
	rethrow();
	throw SyntaxException(TRACE_INFO, "Not Implemented!\n");

#if 0
	size_t start_count = _load_count;
	printf("Loading all atoms; maxuuid=%lu max height=%d\n",
		max_nrec, max_height);
	bulk_load = true;
	bulk_start = time(0);


#define NCHUNKS 300
#define MINSTEP 10123
	std::vector<unsigned long> steps;
	unsigned long stepsize = MINSTEP + max_nrec/NCHUNKS;
	for (unsigned long rec = 0; rec <= max_nrec; rec += stepsize)
		steps.push_back(rec);

	printf("Loading all atoms: "
		"Max Height is %d stepsize=%lu chunks=%zu\n",
		 max_height, stepsize, steps.size());

	// Parallelize always.
	opencog::setting_omp(NUM_OMP_THREADS, NUM_OMP_THREADS);

	for (int hei=0; hei<=max_height; hei++)
	{
		unsigned long cur = _load_count;

		OMP_ALGO::for_each(steps.begin(), steps.end(),
			[&](unsigned long rec)
		{
			rp.rs->foreach_row(&Response::load_all_atoms_cb, &rp);
		});
		printf("Loaded %lu atoms at height %d\n", _load_count - cur, hei);
	}

	time_t secs = time(0) - bulk_start;
	double rate = ((double) _load_count) / secs;
	printf("Finished loading %zu atoms in total in %d seconds (%d per second)\n",
		(_load_count - start_count), (int) secs, (int) rate);
	bulk_load = false;

	// synchrnonize!
	table.barrier();
#endif
}

void IPFSAtomStorage::loadType(AtomTable &table, Type atom_type)
{
	rethrow();
	throw SyntaxException(TRACE_INFO, "Not Implemented!\n");

#if 0
	size_t start_count = _load_count;

#define NCHUNKS 300
#define MINSTEP 10123
	std::vector<unsigned long> steps;
	unsigned long stepsize = MINSTEP + max_nrec/NCHUNKS;
	for (unsigned long rec = 0; rec <= max_nrec; rec += stepsize)
		steps.push_back(rec);

	logger().debug("IPFSAtomStorage::loadType: "
		"Max Height is %d stepsize=%lu chunks=%lu\n",
		 max_height, stepsize, steps.size());

	// Parallelize always.
	opencog::setting_omp(NUM_OMP_THREADS, NUM_OMP_THREADS);

	for (int hei=0; hei<=max_height; hei++)
	{
		unsigned long cur = _load_count;

		OMP_ALGO::for_each(steps.begin(), steps.end(),
			[&](unsigned long rec)
		{
			Response rp(conn_pool);
			rp.table = &table;
			rp.store = this;
			char buff[BUFSZ];
			snprintf(buff, BUFSZ, "SELECT * FROM Atoms WHERE type = %d "
			         "AND height = %d AND uuid > %lu AND uuid <= %lu;",
			         db_atom_type, hei, rec, rec+stepsize);
			rp.height = hei;
			rp.exec(buff);
			rp.rs->foreach_row(&Response::load_if_not_exists_cb, &rp);
		});
		logger().debug("IPFSAtomStorage::loadType: "
		               "Loaded %lu atoms of type %d at height %d\n",
			_load_count - cur, db_atom_type, hei);
	}

	logger().debug("IPFSAtomStorage::loadType: Finished loading %zu atoms in total\n",
		_load_count- start_count);

	// Synchronize!
	table.barrier();
#endif
}

/// Store all of the atoms in the atom table.
void IPFSAtomStorage::store(const AtomTable &table)
{
	rethrow();

	logger().info("Bulk store of AtomSpace\n");

	_store_count = 0;
	bulk_start = time(0);
	bulk_store = true;

	// Try to knock out the nodes first, then the links.
	table.foreachHandleByType(
		[&](const Handle& h)->void { storeAtom(h); },
		NODE, true);

	table.foreachHandleByType(
		[&](const Handle& h)->void { storeAtom(h); },
		LINK, true);

	flushStoreQueue();
	bulk_store = false;

	time_t secs = time(0) - bulk_start;
	double rate = ((double) _store_count) / secs;
	printf("\tFinished storing %lu atoms total, in %d seconds (%d per second)\n",
		(unsigned long) _store_count, (int) secs, (int) rate);
	printf("\tAtomSpace CID: %s\n", _atomspace_cid.c_str());
}

void IPFSAtomStorage::storeAtomSpace(AtomSpace* atomspace)
{
	store(atomspace->get_atomtable());
}

void IPFSAtomStorage::loadAtomSpace(AtomSpace* atomspace)
{
	// XXX This is wrong, it should instead do an IPNS resolve
	// and load that!
	throw SyntaxException(TRACE_INFO, "Not Implemented!\n");
	load_atomspace(atomspace, _atomspace_cid);
}

/* ============================= END OF FILE ================= */
