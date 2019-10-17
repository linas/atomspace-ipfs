# atomspace-ipfs
IPFS backend to the AtomSpace.

## Pre-alpha version 0.0.1

The goal is to be able to share AtomSpace contents via the IPFS network.
**Status**: there's isn't any code yet. It does not work.

### Architecture:
This implementation will be just a standard `BackingStore`
implementation to the current Atomspace backend API.


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

## Prereqs

* Clone and build the atomspace.
* Install IPFS
  -- https://github.com/vasild/cpp-ipfs-api
     This needs `sudo apt install nlohmann-json3-dev`
