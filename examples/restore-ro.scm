;
; restore-ro.scm
;
; Simple example of read-only access to an AtomSpace.
;
(use-modules (opencog))
(use-modules (opencog persist))
(use-modules (opencog persist-ipfs))

; Open connection to the IPFS server.
; One can specify either an IPNS CID, or an IPFS CID that identifies the
; AtomSpace.
; (ipfs-open "ipfs://localhost/ipns/QmVkzxhCMDYisZ2QEMA5WYTjrZEPVbETZg5rehsijUxVHx")
; (ipfs-open "ipfs://localhost:5001/ipns/QmVkzxhCMDYisZ2QEMA5WYTjrZEPVbETZg5rehsijUxVHx")
(ipfs-open "ipfs:///ipfs/QmUqobJLPNyB7kQcczLo3GrJBDpXJG2JcHqfDCCWCt2NCg")

; (ipfs-resolve-atomspace)

; Given a specific Atom, fetch the current Values on that Atom
(define c (ConceptNode "example concept"))
(fetch-atom c)

; Restore the EvaluationLink that was previously created, by restoring
; the Incoming Set of the ListLink it contains. Recall, that the
; original EvaluationLink was:
;
;    (EvaluationLink
;       (PredicateNode "Some relationship")
;       (ListLink
;          (ConceptNode "foo")
;          (ConceptNode "bar")))
;
(define ll (List (Concept "foo") (Concept "bar")))
(fetch-incoming-set ll)

; Verify that the EvaluationLink was loaded.
(cog-prt-atomspace)

; Review the stats
(ipfs-stats)

; Or one can fetch the entire contents of the AtomSpace.
(load-atomspace)

; Verify the AtomSpace contents
(cog-prt-atomspace)

; Review the stats
(ipfs-stats)

; Close the connection.
(ipfs-close)

; The End.  That's all for now.
