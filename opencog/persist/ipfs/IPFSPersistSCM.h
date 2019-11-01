/*
 * opencog/persist/ipfs/IPFSPersistSCM.h
 *
 * Copyright (c) 2008 by OpenCog Foundation
 * Copyright (c) 2008, 2009, 2013, 2015, 2019 Linas Vepstas <linasvepstas@gmail.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_IPFS_PERSIST_SCM_H
#define _OPENCOG_IPFS_PERSIST_SCM_H

#include <string>

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atoms/base/Handle.h>
#include <opencog/persist/ipfs/IPFSAtomStorage.h>

namespace opencog
{
/** \addtogroup grp_persist
 *  @{
 */

class IPFSBackingStore;
class IPFSPersistSCM
{
private:
	static void* init_in_guile(void*);
	static void init_in_module(void*);
	void init(void);

	IPFSAtomStorage *_backing;
	AtomSpace *_as;

public:
	IPFSPersistSCM(AtomSpace*);
	~IPFSPersistSCM();

	void do_open(const std::string&);
	void do_close(void);
	void do_load(void);
	std::string do_atom_cid(const Handle&);
	Handle do_fetch_atom(const std::string&);
	void do_load_atomspace(const std::string&);
	std::string do_ipfs_atomspace(void);
	std::string do_ipns_atomspace(void);
	void do_publish_atomspace(void);
	void do_resolve_atomspace(void);

	void do_stats(void);
	void do_clear_stats(void);
}; // class

/** @}*/
}  // namespace

extern "C" {
void opencog_persist_ipfs_init(void);
};

#endif // _OPENCOG_IPFS_PERSIST_SCM_H
