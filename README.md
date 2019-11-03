# atomspace-ipfs
[IPFS](https://ipfs.io/) driver backend to the
[AtomSpace](https://github.com/opencog/atomspace) (hyper-)graph database.

The code here is a backend driver to the AtomSpace graph database,
enabling AtomSpace contents to be shared via the InterPlanetary
File System (IPFS) network. The goal is to allow efficient decentralized,
distributed operation over the global internet, allowing many
AtomSpace processes to access and perform updates to large datasets.

### The AtomSpace
The [AtomSpace](https://wiki.opencog.org/w/AtomSpace) is a
(hyper-)graph database whose nodes and links are called
["Atoms"](https://wiki.opencog.org/w/Atom). Each (immutable) Atom has
an associated (mutable)
[key-value store](https://wiki.opencog.org/w/Value).
The Atomspace has a variety of advanced features not normally found
in ordinary graph databases, including an advanced query language
and "active" Atoms.

### IPFS
IPFS, the InterPlanetary File System, is an internet-wide
globally-accesible file system, built on top of a distributed hash
table for addressing files by content, wherever they may be located
on the network.  It provides decentralized file storage.

## Important Notice !!
___There are fundamental design issues with this implementation that
appear to be unresolvable with the current API's offered by IPFS. All
potential users and developers are strongly urged to make use of the
[OpenDHT-based AtomSpace backend](https://github.com/opencog/atomspace-dht)
instead!___

___It appears that the OpenDHT API is an excellent fit for the
AtomSpace, providing exactly those kinds of features that the AtomSpace
needs!! Woo Hoo!!___

___Because of this situation, it seems unlikely that the development
of the IPFS driver will continue beyond the current version. Again,
please use the
[OpenDHT-based backend driver](https://github.com/opencog/atomspace-dht)
instead!___

## Beta version 0.2.0
The driver here was developed and tested with IPFS version `0.4.22-`.

### Status
In the current implementation:
 * A design for representing the AtomSpace in IPFS has been chosen.
   It has numerous shortcomings, detailed below.
 * System is feature-complete. All seven unit tests from the original
   Atomspace-SQL test suite has been ported. Six of the seven pass --
   the seventh one is a multi-user test, and the multi-user design
   used here is (deeply) flawed. See further comments below.
   The [basic tutorials](examples) work as documented.  So basically,
   everything works, as long as only one user at a time is modifying
   the AtomSpace contents. Multiple users *could* edit the same
   AtomSpace, *if* they were careful to exchange with each-other what
   their latest CID was. Otherwise, each user ends up forking the
   AtomSpace, and the forks never get merged back together again.
   This is a design flaw: IPFS does not provide any way of doing
   decentralized set membership. This forces the entire AtomSpace
   to be mapped into just one file, making it very highly "centralized".
   Since it's just a file, it can be forked. See comments below.
 * Due to IPFS bugs with the performance of IPNS, IPNS is mostly unused
   in this implementation.  This means that users need to arrange other
   channels of communication to find out what the latest AtomSpace
   is (by sharing the AtomSpace CID in some other way, rather than
   sharing via IPNS).
 * Many or most operations are slow. Like really, really slow.
	Like, a dozen-atoms-per-second-slow. Which is unusable on a
   production database. In a few cases, performance could be improved
   by better caching.  In most cases, this is a fundamental limitation
   of the current design. If might be a fundamental limitation of IPFS,
   since IPFS is not optimal for handling very small objects, and
   (most) Atoms are just tiny.

### Centralization
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
   which of the published AtomSpace versions are authoritative.
   As a result, each writer effectively creates a forked version
   of the AtomSpace, and there's no particular way to merge the forks
   back together again. See `MultiUserUTest` for a failing example
   of the resulting badness.)
 * Performance bottlenecks when there are multiple writers.

Despite these design flaws, I went ahead and wrote the code anyway.
It helped clarify the issues.  A better design is needed, but that
better design seems to be blocked without core changes to the IPFS
core system. Decentralized updates are sorely needed.

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
 * Centralized directory, as noted above.
 * Race conditions if multiple users update the same AtomSpace at
   the same time.  These race conditions will result in lost data
   (lost Atom inserts, deletes, or lost changes of TruthValues or
   other Values.)
 * Atom removal is a heavy-weight operation, due to heavy
   interaction with incoming sets.

### Architecture:
This implementation provides a full, complete implementation of the
standard `BackingStore` API from the Atomspace. Its a backend driver.

The git repo layout is the same as that of the AtomSpace repo. Build
and install mechanisms are the same.

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
  outgoing set can be placed in the IPLD `links[]` json element. These
  will be automatically hashed by the IPFS subsystem, delivering a true
  globally unique ID (the CID) for the Atom, exactly as desired.

* We distinguish between the GUID of the Atom, and the CID of the
  Valuation. The GUID of the Atom is it's IPFS CID, when considering
  only the Atom itself, and not it's values or incoming set. Thus,
  it really is globally unique and non-varying.  By contrast, the
  file containing the Valuations will change whenever the Values
  change, and so the CID attached to an Atom will be changing.
  The tricky part of the design is to associate this immutable
  GUID, with the current CID for that Atom.

* Conceptually, the AtomSpace is nothing more than a set of (GUID, CID)
  pairs, such that there can only ever be just one CID per GUID.
  That is, the AtomSpace is a time-varying map from GUID to CID.
  This automaticaly enforces the other requirements:
   - If a GUID is not a part of the AtomSpace, then that Atom is not
     in the AtomSpace.
   - The Atom can have other Values in other AtomSpaces, but it can
     have only one Valuation in this AtomSpace.

* Each read-only AtomSpace corresponds to a directory, so that each
  Atom appears in the `links[]` json member of the directory.  Updated
  AtomSpaces are published on IPNS, so that a read-write AtomSpace
  corresponds to a unique key (the key used to generate the IPNS name).
  That is, when an Atom is added to/removed from the Atomspace, the
  `links[]` list is modified to add/remove that Atom, thus creating a
  new IPFS CID. Then IPNS is updated to point at this new AtomSpace
  CID.  The good news: one knows *exactly* which version of an AtomSpace
  one is working with (this is very unlike the current AtomSpace!)

* Currently, IPNS is slow. A core assumption in the design is that
  someday, this will be fixed, and IPNS will be fast.  Or that, at
  least, the IPNS latency will be immaterial, and that we'll work with
  the most-recently-resolved values.

* Design alternative A:
    - Every Atom has a corresponding PKI key. So, millions of keys.
      The key name is globally unique: it is just the name of the
      AtomSpace, followed by the scheme string of the Atom.
    - The private part of the PKI key is held by the key-creator.
      It stays private, unless shared.
    - The AtomSpace is a single file, listing all of the Atoms in it,
      together with all of the public keys for each Atom. This means
      that the AtomSpace is centralized.
    - If a user wants to find the current Valuation of an Atom,
      they must:
      + Obtain the AtomSpace file somehow (either someone gives the
        user the CID of the current AtomSpace, or the user obtains the
        CID from an IPNS lookup.)
      + Look up the Atom in that file, if present.
      + Examine the public key of that Atom.
      + Perform the IPNS lookup for that key.
      + Fetch the file corresponding to the CID that IPNS returned.
      + Parse the file, extract the desired Value.
    - Incoming sets are stored along with the Valuation file.
    - If a user wants to change (update) the Valuation of a Atom,
      they must:
      + Obtain the private key for that Atom/Atomspace combination,
        by asking someone for it.
      + Update the Valuation file.
      + IPNS publish the new file.

      Note that the updates to the IncomingSet are conflict-prone. So
      a CRDT format for IncomingSets is required.

    Issues:
    - Publishing a single large AtomSpace file is ugly; it prevents
      simultaneous, high-speed updates. It's centralized and not
      scalable. There's conflict resolution issues if there are multiple
      updaters.

    Status:
    - The above was NOT followed, in that IPNS was avoided. Instead,
      there is a master AtomSpace file containing only IPFS CID's for
      Valuations.

* Design Alternative B: Perhaps it is possible to store mutable values
  using the DHT API directly?  This would also allow alpha-conversion
  issues to be handled (as we'd alpha-convert to a unique combinator
  form, and hash only that.)

* Q: How to load the incoming set of an Atom?
  Currently, the incoming set of an Atom is stored as part of the
  mutable version of that Atom, and can therefore be fetched. The
  IPFS CID of the current mutated Atom is obtained by lookup of the
  AtomSpace (from the single, large directory file that the AtomSpace
  is stored in).

* Q: is Pin needed to prevent a published atomspace from disappearing?
  Doesn't seem to be!? (Yet. As long as my IPFS daemon stays up...)

* Idea: Use pubsub to publish value updates.

* The current encoding is gonna do alpha-equivalence all wrong. As long
  as there is a single writer, then the AtomSpace can hide this via the
  usual alpha-renaming techniques. But in a multi-user setup, this will
  surely lead to distinct-but-alpha-equivalent Atoms. The core problem
  is that we cannot tell IPFS to skip certain parts of the file, when
  computing the content hash. Maybe this is possible with direct DHT
  access?

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

## Build Prereqs

 * Clone and build the [AtomSpace](https://github.com/opencog/atomspace).
 * Install IPFS Core. On Debian/Ubuntu:
   ```
   curl https://get.siderus.io/key.public.asc | sudo apt-key add -
   echo "deb https://get.siderus.io/ apt/" | sudo tee -a /etc/apt/sources.list.d/siderus.list
   sudo apt update
   sudo apt install ipfs
   ```
 * Get familiar with IPFS. Some useful commands:
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
   `git clone https://github.com/linas/cpp-ipfs-api` and
   `git checkout master-linas` and
   `mkdir build; cd build; cmake ..; make -j; sudo make install`

   This needs the package
   ["JSON for Modern C++"](https://github.com/nlohmann/json):
   `sudo apt install nlohmann-json3-dev`

   API documentation is here:
   [`https://github.com/ipfs/interface-js-ipfs-core/tree/master/SPEC`](https://github.com/ipfs/interface-js-ipfs-core/tree/master/SPEC)
   and here:
   [`https://vasild.github.io/cpp-ipfs-http-client/classipfs_1_1Client.html`](https://vasild.github.io/cpp-ipfs-http-client/classipfs_1_1Client.html).

## Building
Building is just like that for any other OpenCog component.
After installing the pre-reqs, do this:
```
   mkdir build
   cd build
   cmake ..
   make -j
   sudo make install
```
Then go through the [examples](examples) directory.
