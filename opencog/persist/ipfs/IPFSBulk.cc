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

/// load_atomspace -- load AtomSpace from path.
/// The path could be a CID, or it could be /ipfs/CID or it could
/// be /ipns/CID. In the later case, the IPNS lookup is performed.
void IPFSAtomStorage::load_atomspace(AtomSpace* as, const std::string& path)
{
	rethrow();

	if ('/' != path[0])
	{
		load_as_from_cid(as, path);
		return;
	}

	if (std::string::npos != path.find("/ipfs/"))
	{
		load_as_from_cid(as, &path[sizeof("/ipfs/") - 1]);
		return;
	}

	if (std::string::npos != path.find("/ipns/"))
	{
		// Caution: as of this writing, name resolution takes
		// exactly 60 seconds.
		std::string ipfs_path;
		ipfs::Client* conn = conn_pool.pop();
		conn->NameResolve(path, &ipfs_path);
		conn_pool.push(conn);

		// We are expecting the name to resolve into a string
		// of the form "/ipfs/Qm..."
		load_as_from_cid(as, &ipfs_path[sizeof("/ipfs/") - 1]);
		return;
	}

	throw RuntimeException(TRACE_INFO, "Unsupported URI %s\n", path.c_str());
}

/* ================================================================ */

/// load_as_from_cid -- load all atoms listed at the indicated CID.
/// The CID is presumed to be an IPFS CID (and not an IPNS CID or
/// something else).
void IPFSAtomStorage::load_as_from_cid(AtomSpace* as, const std::string& cid)
{
	rethrow();

	size_t start_count = _load_count;
	printf("Loading all atoms from %s\n", cid.c_str());
	bulk_load = true;
	bulk_start = time(0);

	ipfs::Json dag;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagGet(cid, &dag);
	conn_pool.push(conn);
	// std::cout << "The atomspace dag is:" << dag.dump(2) << std::endl;

	auto atom_list = dag["links"];
	for (auto acid: atom_list)
	{
		// std::cout << "Atom CID is: " << acid["Cid"] << std::endl;

		// Rather than fetching objects, we just part the raw string.
		// That seems faster, simpler, easier.
		as->add_atom(decodeStrAtom(acid["Name"]));
		_load_count++;
	}

	time_t secs = time(0) - bulk_start;
	double rate = ((double) _load_count) / secs;
	printf("Finished loading %zu atoms in total in %d seconds (%d per second)\n",
		(_load_count - start_count), (int) secs, (int) rate);
	bulk_load = false;

	// synchrnonize!
	as->barrier();
}

void IPFSAtomStorage::loadType(AtomTable &table, Type atom_type)
{
	rethrow();

#if 0
	size_t start_count = _load_count;

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
	as->barrier();
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
