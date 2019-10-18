# atomspace-ipfs
IPFS backend to the AtomSpace.

The goal is to be able to share AtomSpace contents via the IPFS network.

## The AtomSpace
The [AtomSpace](https://wiki.opencog.org/w/AtomSpace) is a
(hyper-)graph database whose nodes and links are called
["Atoms"](https://wiki.opencog.org/w/Atom). Each (immutable) Atom has
an associated (mutable)
[key-value store](https://wiki.opencog.org/w/Value).
The Atomspace has a variety of advanced features not normally found
in ordinary graph databases.

## Pre-alpha version 0.0.1

**Status**: Nothing actually works. Design alternatives are being
explored.

### Architecture:
This implementation will be just a standard `BackingStore`
implementation to the current Atomspace backend API.

The file directory layout is the same as that of the atomspace.


### Design issues:
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

  Or maybe each atomspace gets it's own directory.

* Should Atoms correspond to files, or objects?  Should Atoms be json?
  Objects have a setData method that does not alter the hash.
  oh wait, but it does. Foooooo xxxxx wtf.
  where's the mutable side of this thing?
  So we can store values as data in the object.
  Objects have links, which we can treat as the outgoing set.
  But what should a Node be stored as?

* Use `ipfs key` to generate an IPNS name for a given atomspace.
  Or maybe `ipfs urlstore` instead? Or `ipfs name` ?

* Use pubsub to publish value updates.

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

   https://github.com/vasild/cpp-ipfs-api

   This needs `sudo apt install nlohmann-json3-dev`
