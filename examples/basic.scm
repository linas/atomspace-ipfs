;
; basic.scm
;
; This provides the most basic example of using IPFS to store Atoms.
; This includes connecting to IPFS, and viewing the resulting Content
; ID's (CID's) of the Atoms placed into it.
;
; Before diving into this example, be sure that the IPFS daemon is
; running. Start it by entering `ipfs daemon` at the bash prompt.
; Verify that it is running by opening http://localhost:5001/webui
;
(use-modules (opencog))
(use-modules (opencog persist))
(use-modules (opencog persist-ipfs))

; To view documentation, type
;    ,a ipfs
; at the guile prompt. For a given function, try, for example:
;    ,d ipfs-open

; hostname of IPFS server, and a key name ...
; (ipfs-open "ipfs://localhost/demo-atomspace")
; (ipfs-open "ipfs://localhost:5001/demo-atomspace")
(ipfs-open "ipfs:///demo-atomspace")

; Get the IPFS CID of the current AtomSpace.
; Ths should return something like `/ipfs/Qm...` which can then
; be explored using an IPFS data explorer.  Note that this CID
; will change every time that an Atom is added to or removed from the
; AtomSpace.  The current AtomSpace contents can be viewed with the
; IPFS explorer at `http://localhost:5001/webui` (by copying the CID
; into the "explore" entry in the web panel.)
;
(ipfs-atomspace-cid)

; Get the IPNS name of the AtomSpace.
; Note that this IPNS name will always be the same for this AtomSpace;
; it is unchanging, a permanent identifying name for the AtomSpace.
;
; This name can be resolved into the current AtomSpace IPFS CID by
; saying, at the bash prompt,
;     `ipfs name resolve /ipns/Qm...`
; The resolution should be equal to what `(ipfs-atomspace-cid)`
; currently returns. Note that the resolution will change over time,
; as Atoms are added to or removed from the AtomSpace.
;
; Note that currently, IPNS resolution is quite slow; this is a known
; IPNS bug, see https://github.com/ipfs/go-ipfs/issues/3860 for details.
; Currently, it usualy takes 60 seconds (there is a 60 second delay).
(ipns-atomspace-cid)

; Create an atom
(define c (Concept "example concept"))

; Publish it to IPFS.  This will create a new IPFS object that encodes
; an AtomSpace with this atom in it.
(store-atom c)

; Examine the CID of the Atom.  This should return exactly
;     /ipfs/QmTBUxX48jRZPwAU3dEgPQm4bShxW2ED3gXTHM78gvqugB
; It can now be viewed with a network explorer, for example,
; at https://explore.ipld.io/#/explore/
(ipfs-atom-cid c)

; Publish it to IPNS. The new AtomSpace doesn't become visible until
; it is published via IPNS.
(barrier)

; Try again, with something more complex
(define e
	(Evaluation
		(Predicate "Some relationship")
		(List (Concept "foo") (Concept "bar"))))

(store-atom e)
(barrier)

; Likewise, view the CID for the EvaluationLink:
(ipfs-atom-cid e)

; The expected CID's for the above are:
;
; QmdhTzU8QJ2ffhLBzfBeT12scY39N4oZ1roGrLGkSKZnMj (Concept "foo")
; QmP6zBPWfboPQvXU58Tuxq5yBtpybLtJEsdrS4Skr7xZYH (Concept "bar")
; QmXrooYTtNdx5EJ898WaX6XWMZqPuxGy5c9Uxc6BMRUQag (List (Concept "foo") (Concept "bar"))
; QmedVhDVkA1WkMpTkqkTiQmBubnVEkJyiJueTPpePErKkK (Predicate "Some relationship")
; QmdQwLMqC6fyAGa6xztV6y53NqyZwiGW4fvngPtukjBraZ (EvaluationLink ...)
;
; Everyone should always get exactly these CID's. If not, then we've
; changed the code and/or there's a bug somewhere.

; Review the stats
(ipfs-stats)

; Close the connection.
(ipfs-close)

; The End.  That's all for now.
