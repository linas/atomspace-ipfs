
; Proto-example
(use-modules (opencog))
(use-modules (opencog persist))
(use-modules (opencog persist-ipfs))

; To view documentation, type
;    ,a ipfs
; at the guile prompt. For a given function, try, for example:
;    ,d ipfs-open

; hostname of IPFS server, and a key name ...
; (ipfs-open "ipfs://localhost/demo-atomspace")
(ipfs-open "ipfs:///demo-atomspace")

; View the IPNS name of the AtomSpace.
; The file path returned by the above can be resolved into the
; current AtomSpace by saying, at the bash prompt,
;     `ipfs name resolve /ipns/Qm...`
(ipns-atomspace-cid)

; Create an atom
(define c (Concept "example concept"))

; Publish it to IPFS.  This will create a new IPFS object that encodes
; an AtomSpace with this atom in it.
(store-atom c)

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
