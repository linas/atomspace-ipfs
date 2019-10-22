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

## Alpha version 0.0.3

**Status**: Design alternatives are being explored. In the current
implementation:
 * It is possible to save Atoms to IPFS, but not values.
 * AtomSpaces can be saved and restored in bulk. See the
   [examples](examples).

The design for Values is unknown and likely to be challenging.

After much thought: there does not seem to be any way of mapping the
AtomSpace into the current design of IPFS+IPNS without resorting to
a single, centralized file listing all of the Atoms in an AtomSpace.
Implementing a single, centralized file seems like a "really bad idea"
for all of the usual reasons: resolution of update conflicts, and it
being a bottleneck for multiple simultaneous updates.  Therefore,
implementation is on hold until a design is obtained that could avoid
this.

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

* The hashes have to include an AtomSpace ID as well. Different
  AtomSpaces might contain atoms that have the same type, name, etc.
  but should not be confused with one-another. (The alternative
  representation would be to take all Atoms as globally unique, but
  then have context-dependent truth values.)

* How do we associate mutable data to an Atom? Specifically:
  -- the values on the Atom.
  -- the various AtomSpaces the atom belongs to.
  -- the slowly changing incoming set.
  To summarize: Values and incoming sets are aspects of the AtomSpace,
  and not of the Atom itself.  Different AtomSpaces will typically
  see different Values and different incoming sets for any given Atom.
  (and any given Atom might not even belong to a given AtomSpace).

### Design choices and issues:

* The first two bullets are satisfied by writing the Atom type and
  it's name (if its a Node) as text into a file. For Links, the
  outgoing set can be placed in the links[] json member.

* Each read-only AtomSpace corresponds to a directory, so that each
  Atom appears in the links[] json member of the directory.  Updated
  AtomSpaces are published on IPNS, so that a read-write AtomSpace
  corresponds to a unique key (the key used to generate the IPNS name).
  That is, when an Atom is added to/removed from the Atomspace, the
  links[] list is modified to add/remove that Atom, thus creating a
  new IPFS CID. Then IPNS is updated to point at this new AtomSpace
  CID.  The good news: one knows *exactly* which version of an AtomSpace
  one is working with (this is very unlike the current AtomSpace!)

* Change the code to use the new IPFS DAG API instead of the Object API.
  The DAG API should allow the json contents to more closely mirror
  actual Atom structures.

* Currently, IPNS is slow. A core assumption in the design is that
  someday, this will be fixed, and IPNS will be fast.  Or that, at
  least, the IPNS latency will be immaterial, and that we'll work with
  the most-recently-resolved values.

* Each AtomSpace Atom is an IPFS object. Nothing more nor less.
  That means all Atoms are globally unique: all atoms have a single
  persistent hash. (This is what is currently implemented; a design
  alternative is to add the AtomSpace IPNS CID to the Atom, which
  makes the Atom "effectively private", since the CID is not publicly
  guessable.)

* Design alternative A:
  -- Every Atom has a corresponding key. So, millions of keys.
     The key name is globally unique: it is just the scheme string
     of the Atom.
  -- The private part of the key is held by the key-creator.  It
     stays private, unless shared.
  -- Every IPFS Atom object has the public key in the Data field
     of that object. That means that IPFS Atom objects are effectively
     "secret", because the public key is not generally known/shared.
  -- The AtomSpace is a single file, listing all of the Atoms in it,
     together with all of the public keys.
  -- If a user wants to find the current Valuation of an Atom,
     they must:
     ++ obtain the AtomSpace file somehow.
     ++ Look up the Atom in that file, if present.
     ++ Examine the public key of that Atom.
     ++ Perform the IPNS lookup for that key.
     ++ Fetch the file corresponding to the CID that IPNS returned.
     ++ Parse the file, extract the desired Value.
  -- Incoming sets are stored along with the Valuation file.
  -- If a user wants to change (update) the Valuation of a Atom,
     they must:
     ++ Obtain the private key for that Atom/Atomspace combination,
        by asking someone for it.
     ++ Update the Valuation file.
     ++ IPNS publish the new file.
     Note that the updates are conflict-prone. So a CRDT format for
     the Valuation file is required.

  Issues:
  -- Publishing a single large AtomSpace file is ugly; it prevents
     simultaneous, high-speed updates. It's centralized and not
     scalable. There's conflict resolution issues if there are multiple
     updaters.

* Q: How to search incoming set?

* Q: is pin needed to prevent a published atomspace from disappearing?

* Use pubsub to publish value updates.

* The current encoding is gonna do alpha-equivalence all wrong.

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
* Install IPFS
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
   Uh, no, you need the enhancements in
   `https://linas.com/vasild/cpp-ipfs-api`
   and then
   `git checkout master-linas`

   This needs `sudo apt install nlohmann-json3-dev`

   API documentation is here:
   `https://github.com/ipfs/interface-js-ipfs-core/tree/master/SPEC`
   and here:
   `https://vasild.github.io/cpp-ipfs-api/classipfs_1_1Client.html`

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
