# atomspace-ipfs
IPFS backend to the AtomSpace.
https://github.com/opencog/atomspace

The goal is to be able to share AtomSpace contents via the IPFS network.

## The AtomSpace
The [AtomSpace](https://wiki.opencog.org/w/AtomSpace) is a
(hyper-)graph database whose nodes and links are called
["Atoms"](https://wiki.opencog.org/w/Atom). Each (immutable) Atom has
an associated (mutable)
[key-value store](https://wiki.opencog.org/w/Value).
The Atomspace has a variety of advanced features not normally found
in ordinary graph databases, including an advanced query language
and "active" Atoms.

## Beta version 0.1.1
Developed and tested with IPFS version `0.4.22-`.

**Status**: In the current implementation:
 * A design for representing the AtomSpace in IPFS has been chosen.
   It has numerous shortcomings, detailed below.
 * System is feature-complete. Four unit tests (out of seven from
   the Atomspace-SQL test suite) has been ported. Three of them pass.
   The [basic tutorials](examples) work as documented.  So its plenty
   enough to play around with (you are very unlikely to hit the bug
   in the one failing unit test!)
 * Due to IPFS bugs with the performance of IPNS, it is mostly unused
   in this implementation.  This means that users need to arrange other
   channels of communication for find out what the latest AtomSpace
   is (by sharing the CID in some other way, rather than sharing via
   IPNS).
 * Many or most operations are slow. Like really, really slow.
	Like, a dozen-atoms-per-second-slow. Which is unusable on a
   production database. In a few cases, performance could be improved
   by better caching.  In most cases, this is a fundamental limitation
   of the current design. If might be a fundamental limitation of IPFS,
   since IPFS is not optimal for handling very small objects, and
   Atoms are just tiny.

There does not seem to be any way of mapping the AtomSpace into the
current design of IPFS+IPNS without resorting to a single, centralized
directory file listing all of the Atoms in an AtomSpace.  Implementing
a single, centralized directory file seems like a "really bad idea"
for all of the usual reasons:
 * When it gets large, it does not scale.
 * Impossible to optimize fetch of atoms-by-type.
 * Hard to optimize fetch of incoming set.
 * Unresolvable update conflicts (race conditions) when there
   are multiple writers (i.e. with multiple writers, it's unclear
   which of the published AtomSpace versions are authoritative.)
 * Performance bottlenecks when there are multiple writers.

Despite this, a bad, hacky implementation, with the above obvious
failures, is created anyway.  But we need a better design, and that
better design seems to be blocked without core changes to the IPFS
core system.

A suitable decentralized design *would* be possible, if IPNS was
extended with one additional feature (or if some other system was
used, taking the place of IPNS).  Currently, IPNS does this:
```
    PKI public-key ==> resolved CID
```
The ideal enhanced-IPNS lookup would be this:
```
    (PKI public-key, hash) ==> resolved CID
```
Details are described below.

### Known Bugs
There are several bugs that are known, but are problematic to fix:
 * Excess debug printfs still in the code. (convert to logger().debug())
 * Centralized directory, as noted above.
 * Race conditions if multiple users update the same AtomSpace at
   the same time.  These race conditions will result in lost data
   (lost Atom inserts, deletes, or lost changes of TruthValues or
   other Values.)
 * Unsafe access to _atomspace_cid from multiple threads.
 * Potential crashes if user manipulates non-existant Atoms.(?)
 * Atom removal is a particularly heavy-weight operation, due
   to heavy interaction with incoming sets.

### Architecture:
This implementation provides the a standard `BackingStore` API
as defined by the Atomspace.

The git repo layout is the same as that of the atomspace. Build and
install mechanisms are the same.

### Design requirements:
* To get any hope of uniqueness and non-collision of Atoms, this will
  require that each atom will get it's own globally unique hash, viz
  a crypto-secure 256-bit (32-byte) hash. This is considerably larger
  than the current non-crypt-secure 64-bit hash used in the current
  AtomSpace implementation.

* To avoid hash collisions, the Atom Type has to be hashed in, as a
  string, as numeric Atom Type assignments (currently 16-bit short
  int's in the AtomSpace) cannot be made global safely.

* Although Atoms are globally unique and immutable, the associated
  values are mutable, and also vary depending on which AtomSpace they
  belong to.

* How do we associate mutable data to an Atom? Specifically:
  -- the Values on the Atom.
  -- the various AtomSpaces the atom belongs to.
  -- the slowly changing Incoming Set.
  To summarize: Values and Incoming Sets are aspects of the AtomSpace,
  and not of the Atom itself.  Different AtomSpaces will typically
  see different Values and different incoming sets for any given Atom.
  (and any given Atom might not even belong to a given AtomSpace).

### Design choices and issues:

* The first two bullets are satisfied by writing the Atom type and
  it's name (if its a Node) as text into a file. For Links, the
  outgoing set can be placed in the links[] json member. These will
  be automatically hashed by the IPFS subsystem, deliviering a true
  globally unique ID (the CID) for the Atom.

* Each read-only AtomSpace corresponds to a directory, so that each
  Atom appears in the links[] json member of the directory.  Updated
  AtomSpaces are published on IPNS, so that a read-write AtomSpace
  corresponds to a unique key (the key used to generate the IPNS name).
  That is, when an Atom is added to/removed from the Atomspace, the
  links[] list is modified to add/remove that Atom, thus creating a
  new IPFS CID. Then IPNS is updated to point at this new AtomSpace
  CID.  The good news: one knows *exactly* which version of an AtomSpace
  one is working with (this is very unlike the current AtomSpace!)

* Currently, IPNS is slow. A core assumption in the design is that
  someday, this will be fixed, and IPNS will be fast.  Or that, at
  least, the IPNS latency will be immaterial, and that we'll work with
  the most-recently-resolved values.

* Design alternative A:
  -- Every Atom has a corresponding PKI key. So, millions of keys.
     The key name is globally unique: it is just the name of the
     AtomSpace, followed by the scheme string of the Atom.
  -- The private part of the PKI key is held by the key-creator.
     It stays private, unless shared.
  -- The AtomSpace is a single file, listing all of the Atoms in it,
     together with all of the public keys for each Atom. This means
     that the AtomSpace is centralized.
  -- If a user wants to find the current Valuation of an Atom,
     they must:
     ++ obtain the AtomSpace file somehow.
     ++ Look up the Atom in that file, if present.
     ++ Examine the public key of that Atom.
     ++ Perform the IPNS lookup for that key.
     ++ Fetch the file corresponding to the CID that IPNS returned.
     ++ Parse the file, extract the desired Value.
  -- Incoming sets are stored along with the Valuation file.
     (Except that this is pointless, because there is no way of
     knowing the CID of this file, unless one first loads all of
     the AtomSpace, in which case the loading of the incoming set is
     pointless...)
  -- If a user wants to change (update) the Valuation of a Atom,
     they must:
     ++ Obtain the private key for that Atom/Atomspace combination,
        by asking someone for it.
     ++ Update the Valuation file.
     ++ IPNS publish the new file.
     Note that the updates to the IncomingSet are conflict-prone. So
     a CRDT format for IncomingSets is required.

  Issues:
  -- Publishing a single large AtomSpace file is ugly; it prevents
     simultaneous, high-speed updates. It's centralized and not
     scalable. There's conflict resolution issues if there are multiple
     updaters.

* Q: How to load the incoming set of an Atom?
  Currently, the incoming set of an Atom is stored as part of the
  mutable version of that Atom, and can therefore be fetched. The
  IPFS CID of the current mutated Atom is obtained by lookup of the
  AtomSpace (from the single, large directory file that the AtomSpace
  is stored in).

* Q: is Pin needed to prevent a published atomspace from disappearing?
  Doesn't seem to be!?

* Use pubsub to publish value updates.

* The current encoding is gonna do alpha-equivalence all wrong. This is
  an implementation bug.

## IPNS++
It currently appears to be impossible to map the AtomSpace into IPFS
without resorting to a single, centralized file that contains the
AtomSpace contents.  Clearly, this would be a bad design, for all of
the usual reasons associated with centralization.

However, a good high-quality, truly decentralized design would be
possible if IPNS was modified slightly. Currently, IPNS does this:
```
    PKI public-key ==> resolved CID
```
The ideal IPNS lookup would be this:
```
    (PKI public-key, hash) ==> resolved CID
```
If the above were possible, the AtomSpace mapping would become
straightforward: The `hash` would be the hash of an Atom, and the
resolved CID would contain the Values associated with that Atom.
This works, because the hash of an Atom is globally unique: anyone
can know what it is. Anyone having access to the public-key would
then have read-access to that particular AtomSpace.  Anyone having
the private key would have write access. All operations are
distributed, decentralized, assuming that the lookup itself can be
made decentralized.

Of course, its easy to create a centralized hash lookup: a single
large file containing a list of `hash ==> CID` mappings. But that
suffers from all the typical problems of centralization: the
multiple-writers problem, problems with being a bottleneck for
updates, file-size issues. etc.

## Prereqs

 * Clone and build the atomspace.
 * Install IPFS Core
   ```
   curl https://get.siderus.io/key.public.asc | sudo apt-key add -
   echo "deb https://get.siderus.io/ apt/" | sudo tee -a /etc/apt/sources.list.d/siderus.list
   sudo apt update
   sudo apt install ipfs
   ```
   Some useful commands:
   ```
   ipfs init
   ipfs daemon
   ipfs swarm peers
   ipfs commands
   ```

 * Install the IPFS C++ client library

   `https://github.com/vasild/cpp-ipfs-api`
   Uh, no, actually, you need the extended, updated version, here:
   [`https://github.com/linas/cpp-ipfs-api`](https://github.com/linas/cpp-ipfs-api)
   and and so then
   `git cline https://github.com/linas/cpp-ipfs-api` and
   `git checkout master-linas` and
   `mkdir build; cd build; cmake ..; make -j; sudo make install`

   This needs the package "JSON for Modern C++"
   `sudo apt install nlohmann-json3-dev`

   API documentation is here:
   [`https://github.com/ipfs/interface-js-ipfs-core/tree/master/SPEC`](https://github.com/ipfs/interface-js-ipfs-core/tree/master/SPEC).
   and here:
   [`https://vasild.github.io/cpp-ipfs-api/classipfs_1_1Client.html`](https://vasild.github.io/cpp-ipfs-api/classipfs_1_1Client.html).

## Building
After installing the pre-reqs, do this:
```
   mkdir build
   cd build
   cmake ..
   make -j
   sudo make install
```
Then go through the [examples](examples) directory.
