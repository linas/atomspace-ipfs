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
in ordinary graph databases.

## Alpha version 0.0.3

**Status**: Design alternatives are being explored. In the current
implementation:
 * It is possible to save Atoms to IPFS, but not values.
 * AtomSpaces can be saved and restored in bulk. See the
   [examples](examples).

The design for Values is unknown and likely to be challenging.

### Architecture:
This implementation will be just a standard `BackingStore`
implementation to the current Atomspace backend API.

The file directory layout is the same as that of the atomspace.

### Design requirements:
* To get any hope of uniqueness and non-collision of Atoms, this will
  require that each atom will get it's own globally unique hash, viz
  a crypto-secure 256-bit (32-byte) hash. This is considerabley larger
  than the current non-crypt-secure 64-bit hash used in the current
  AtomSpace implementation.

* To avoid hash collisions, the Atom Type has to be hashed in, as a
  string, as numeric Atom Type assignments (currently 16-bit short
  int's in the AtomSpace) cannot be made global safely.

* The hashes have to include an AtomSpace ID as well. Different
  Atomspaces might contain atoms that have the same type, name, etc.
  but should not be confused with one-another. (The alternative
  representation would be to take all Atoms as globally unique, but then
  have context-dependent truth values. This does not seem to be a wise
  design choice.

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

* Long-term: use IPLD to create some kind of Atomspace-specific thing.
  https://ipld.io/ For now, we are limited to IPFS, which uses a
  distinctive file format.  Based on the current IPFS implementation,
  it seems like the only possibility is to fork the IPFS code, and
  alter it to avoid including Values as part of the CID hash.

* So we can store values as data in the object. XXX but then oset
  problems... where does the value go for Links? where is it stored?
  How is it updated?

* How to search incoming set?

* Use pubsub to publish value updates.

* The current encoding is gonna do alpha-equivalnce all wrong.

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
