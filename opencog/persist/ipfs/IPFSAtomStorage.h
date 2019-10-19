/*
 * FILE:
 * opencog/persist/ipfs/IPFSAtomStorage.h

 * FUNCTION:
 * IPFS-backed persistent storage.
 *
 * HISTORY:
 * Copyright (c) 2008,2009,2013,2017,2019 Linas Vepstas <linasvepstas@gmail.com>
 *
 * LICENSE:
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_IPFS_ATOM_STORAGE_H
#define _OPENCOG_IPFS_ATOM_STORAGE_H

#include <atomic>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include <ipfs/client.h>

#include <opencog/util/async_buffer.h>

#include <opencog/atoms/base/Atom.h>
#include <opencog/atoms/base/Link.h>
#include <opencog/atoms/base/Node.h>
#include <opencog/atoms/atom_types/types.h>
#include <opencog/atoms/value/FloatValue.h>
#include <opencog/atoms/value/LinkValue.h>
#include <opencog/atoms/value/StringValue.h>
#include <opencog/atoms/base/Valuation.h>

#include <opencog/atomspace/AtomTable.h>
#include <opencog/atomspace/BackingStore.h>

namespace opencog
{
/** \addtogroup grp_persist
 *  @{
 */

// Number of threads do use ofr IPFS I/O.
#define NUM_OMP_THREADS 8

class IPFSAtomStorage : public BackingStore
{
	private:
		// The IPFS CID of the current atomspace.
		std::string _atomspace_cid;

		// Pool of shared connections
		concurrent_stack<ipfs::Client*> conn_pool;
		int _initial_conn_pool_size;

		void init(const char *);
		std::string _uri;

		// ---------------------------------------------
		// Fetching of atoms.
		Handle doGetNode(Type, const char *);
		Handle doGetLink(Type, const HandleSeq&);

		int getMaxObservedHeight(void);
		int max_height;

		void getIncoming(AtomTable&, const char *);
		// --------------------------
		// Storing of atoms
		std::mutex _store_mutex;

		int do_store_atom(const Handle&);
		void vdo_store_atom(const Handle&);
		void do_store_single_atom(const Handle&, int);

		bool not_yet_stored(const Handle&);
		std::string oset_to_string(const HandleSeq&);

		bool bulk_load;
		bool bulk_store;
		time_t bulk_start;

		// --------------------------
		// Atom removal
		// void removeAtom(Response&, UUID, bool recursive);
		// void deleteSingleAtom(Response&, UUID);

		// --------------------------
		// Values
#define NUMVMUT 16
		std::mutex _value_mutex[NUMVMUT];
		void store_atom_values(const Handle &);
		void get_atom_values(Handle &);

		typedef unsigned long VUID;

		// ValuePtr doUnpackValue(Response&);
		ValuePtr doGetValue(const char *);

		VUID storeValue(const ValuePtr&);
		ValuePtr getValue(VUID);
		void deleteValue(VUID);

		// --------------------------
		// Valuations
		std::mutex _valuation_mutex;
		void storeValuation(const ValuationPtr&);
		void storeValuation(const Handle&, const Handle&, const ValuePtr&);
		ValuePtr getValuation(const Handle&, const Handle&);
		void deleteValuation(const Handle&, const Handle&);
		// void deleteValuation(Response&, UUID, UUID);
		// void deleteAllValuations(Response&, UUID);

		std::string float_to_string(const FloatValuePtr&);
		std::string string_to_string(const StringValuePtr&);
		std::string link_to_string(const LinkValuePtr&);

		Handle tvpred; // the key to a very special valuation.

		// --------------------------
		// Performance statistics
		std::atomic<size_t> _num_get_nodes;
		std::atomic<size_t> _num_got_nodes;
		std::atomic<size_t> _num_rec_nodes;
		std::atomic<size_t> _num_get_links;
		std::atomic<size_t> _num_got_links;
		std::atomic<size_t> _num_rec_links;
		std::atomic<size_t> _num_get_insets;
		std::atomic<size_t> _num_get_inlinks;
		std::atomic<size_t> _num_node_inserts;
		std::atomic<size_t> _num_link_inserts;
		std::atomic<size_t> _num_atom_removes;
		std::atomic<size_t> _num_atom_deletes;
		std::atomic<size_t> _load_count;
		std::atomic<size_t> _store_count;
		std::atomic<size_t> _valuation_stores;
		std::atomic<size_t> _value_stores;
		time_t _stats_time;

		// -------------------------------
		// Type management
		// The typemap translates between opencog type numbers and
		// the database type numbers.  Initially, they match up, but
		// might get askew if new types are added or deleted.

		// TYPEMAP_SZ is defined as the maximum number of possible
		// OpenCog Types (65536 as Type is currently a short int)
		static_assert(2 == sizeof(Type),
		     "*** Typemap needs to be redesigned to handle larger types! ***");
		#define TYPEMAP_SZ (1 << (8 * sizeof(Type)))
		int storing_typemap[TYPEMAP_SZ];
		Type loading_typemap[TYPEMAP_SZ];
		char * db_typename[TYPEMAP_SZ];

		bool type_map_was_loaded;
		void load_typemap(void);
		void setup_typemap(void);
		void set_typemap(int, const char *);
		std::mutex _typemap_mutex;

		// Provider of asynchronous store of atoms.
		// async_caller<IPFSAtomStorage, Handle> _write_queue;
		async_buffer<IPFSAtomStorage, Handle> _write_queue;
		std::exception_ptr _async_write_queue_exception;
		void rethrow(void);

	public:
		IPFSAtomStorage(std::string uri);
		IPFSAtomStorage(const IPFSAtomStorage&) = delete; // disable copying
		IPFSAtomStorage& operator=(const IPFSAtomStorage&) = delete; // disable assignment
		virtual ~IPFSAtomStorage();
		bool connected(void); // connection to DB is alive

		void kill_data(void); // destroy DB contents

		void registerWith(AtomSpace*);
		void unregisterWith(AtomSpace*);
		void extract_callback(const AtomPtr&);
		int _extract_sig;

		// AtomStorage interface
		Handle getNode(Type, const char *);
		Handle getLink(Type, const HandleSeq&);
		void getIncomingSet(AtomTable&, const Handle&);
		void getIncomingByType(AtomTable&, const Handle&, Type t);
		void getValuations(AtomTable&, const Handle&, bool get_all);
		void storeAtom(const Handle&, bool synchronous = false);
		void removeAtom(const Handle&, bool recursive);
		void loadType(AtomTable&, Type);
		void barrier();
		void flushStoreQueue();

		// Large-scale loads and saves
		void loadAtomSpace(AtomSpace*);
		void storeAtomSpace(AtomSpace*);
		void load(AtomTable &); // Load entire contents of DB
		void store(const AtomTable &); // Store entire contents of AtomTable

		// Debugging and performance monitoring
		void print_stats(void);
		void clear_stats(void); // reset stats counters.
		void set_hilo_watermarks(int, int);
		void set_stall_writers(bool);
};


/** @}*/
} // namespace opencog

#endif // _OPENCOG_IPFS_ATOM_STORAGE_H
