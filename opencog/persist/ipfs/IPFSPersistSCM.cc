/*
 * opencog/persist/ipfs/IPFSPersistSCM.cc
 *
 * Copyright (c) 2008 by OpenCog Foundation
 * Copyright (c) 2008, 2009, 2013, 2015, 2019 Linas Vepstas <linasvepstas@gmail.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <libguile.h>

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atomspace/BackingStore.h>
#include <opencog/guile/SchemePrimitive.h>

#include "IPFSAtomStorage.h"
#include "IPFSPersistSCM.h"

using namespace opencog;


// =================================================================

IPFSPersistSCM::IPFSPersistSCM(AtomSpace *as)
{
    _as = as;
    _backing = nullptr;

    static bool is_init = false;
    if (is_init) return;
    is_init = true;
    scm_with_guile(init_in_guile, this);
}

void* IPFSPersistSCM::init_in_guile(void* self)
{
    scm_c_define_module("opencog persist-ipfs", init_in_module, self);
    scm_c_use_module("opencog persist-ipfs");
    return NULL;
}

void IPFSPersistSCM::init_in_module(void* data)
{
   IPFSPersistSCM* self = (IPFSPersistSCM*) data;
   self->init();
}

void IPFSPersistSCM::init(void)
{
    define_scheme_primitive("ipfs-open", &IPFSPersistSCM::do_open, this, "persist-ipfs");
    define_scheme_primitive("ipfs-close", &IPFSPersistSCM::do_close, this, "persist-ipfs");
    define_scheme_primitive("ipfs-stats", &IPFSPersistSCM::do_stats, this, "persist-ipfs");
    define_scheme_primitive("ipfs-clear-stats", &IPFSPersistSCM::do_clear_stats, this, "persist-ipfs");

    define_scheme_primitive("ipfs-atom-cid", &IPFSPersistSCM::do_atom_cid, this, "persist-ipfs");
    define_scheme_primitive("ipfs-fetch-atom", &IPFSPersistSCM::do_fetch_atom, this, "persist-ipfs");
    define_scheme_primitive("ipfs-load-atomspace", &IPFSPersistSCM::do_load_atomspace, this, "persist-ipfs");
    define_scheme_primitive("ipfs-atomspace-cid", &IPFSPersistSCM::do_ipfs_atomspace, this, "persist-ipfs");
    define_scheme_primitive("ipns-atomspace-cid", &IPFSPersistSCM::do_ipns_atomspace, this, "persist-ipfs");
    define_scheme_primitive("ipfs-publish-atomspace", &IPFSPersistSCM::do_publish_atomspace, this, "persist-ipfs");
    define_scheme_primitive("ipfs-resolve-atomspace", &IPFSPersistSCM::do_resolve_atomspace, this, "persist-ipfs");
}

IPFSPersistSCM::~IPFSPersistSCM()
{
    if (_backing) delete _backing;
}

void IPFSPersistSCM::do_open(const std::string& uri)
{
    if (_backing)
        throw RuntimeException(TRACE_INFO,
             "ipfs-open: Error: Already connected to a database!");

    // Unconditionally use the current atomspace, until the next close.
    AtomSpace *as = SchemeSmob::ss_get_env_as("ipfs-open");
    if (nullptr != as) _as = as;

    if (nullptr == _as)
        throw RuntimeException(TRACE_INFO,
             "ipfs-open: Error: Can't find the atomspace!");

    // Allow only one connection at a time.
    if (_as->isAttachedToBackingStore())
        throw RuntimeException(TRACE_INFO,
             "ipfs-open: Error: Atomspace connected to another storage backend!");
    // Use the IPFS driver.
    IPFSAtomStorage *store = new IPFSAtomStorage(uri);
    if (!store)
        throw RuntimeException(TRACE_INFO,
            "ipfs-open: Error: Unable to open the database");

    if (!store->connected())
    {
        delete store;
        throw RuntimeException(TRACE_INFO,
            "ipfs-open: Error: Unable to connect to the database");
    }

    _backing = store;
    _backing->registerWith(_as);
}

void IPFSPersistSCM::do_close(void)
{
    if (nullptr == _backing)
        throw RuntimeException(TRACE_INFO,
             "ipfs-close: Error: Database not open");

    IPFSAtomStorage *backing = _backing;
    _backing = nullptr;

    // The destructor might run for a while before its done; it will
    // be emptying the pending store queues, which might take a while.
    // So unhook the atomspace first -- this will prevent new writes
    // from accidentally being queued. (It will also drain the queues)
    // Only then actually call the dtor.
    backing->unregisterWith(_as);
    delete backing;
}

std::string IPFSPersistSCM::do_atom_cid(const Handle& h)
{
    if (nullptr == _backing)
        throw RuntimeException(TRACE_INFO,
            "ipfs-atom-cid: Error: Database not open");

    return "/ipfs/" + _backing->get_atom_guid(h);
}

Handle IPFSPersistSCM::do_fetch_atom(const std::string& cid)
{
    if (nullptr == _backing)
        throw RuntimeException(TRACE_INFO,
            "ipfs-fetch-atom: Error: Database not open");

    return _as->add_atom(_backing->fetch_atom(cid));
}

void IPFSPersistSCM::do_load_atomspace(const std::string& cid)
{
    if (nullptr == _backing)
        throw RuntimeException(TRACE_INFO,
            "ipfs-load-atomspace: Error: Database not open");

    return _backing->load_atomspace(_as, cid);
}

std::string IPFSPersistSCM::do_ipfs_atomspace(void)
{
    if (nullptr == _backing)
        throw RuntimeException(TRACE_INFO,
            "ipfs-atomspace: Error: Database not open");

    return _backing->get_ipfs_cid();
}

std::string IPFSPersistSCM::do_ipns_atomspace(void)
{
    if (nullptr == _backing)
        throw RuntimeException(TRACE_INFO,
            "ipns-atomspace: Error: Database not open");

    return _backing->get_ipns_key();
}

void IPFSPersistSCM::do_publish_atomspace(void)
{
    if (nullptr == _backing)
        throw RuntimeException(TRACE_INFO,
            "ipns-atomspace: Error: Database not open");

    return _backing->publish_atomspace();
}

void IPFSPersistSCM::do_resolve_atomspace(void)
{
    if (nullptr == _backing)
        throw RuntimeException(TRACE_INFO,
            "ipns-atomspace: Error: Database not open");

    return _backing->resolve_atomspace();
}

void IPFSPersistSCM::do_stats(void)
{
    if (nullptr == _backing) {
        printf("ipfs-stats: Database not open\n");
        return;
    }

    printf("ipfs-stats: Atomspace holds %lu atoms\n", _as->get_size());
    _backing->print_stats();
}

void IPFSPersistSCM::do_clear_stats(void)
{
    if (nullptr == _backing) {
        printf("ipfs-stats: Database not open\n");
        return;
    }

    _backing->clear_stats();
}

void opencog_persist_ipfs_init(void)
{
    static IPFSPersistSCM patty(NULL);
}
