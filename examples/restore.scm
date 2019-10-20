;
; restore.scm
;
; Simple example of restoring individual atoms, and the bulk restore of
; entire AtomSpaces.
;
(use-modules (opencog))
(use-modules (opencog persist))
(use-modules (opencog persist-ipfs))

; Open connection to the IPFS server.
; (ipfs-open "ipfs://localhost/")
; (ipfs-open "ipfs://localhost:5001/")
(ipfs-open "ipfs:///")

; Restore a single ConceptNode.  This should be in IPFS, if you've
; run the earlier examples. This should return
;     (ConceptNode "example concept")
(ipfs-fetch-atom "QmTBUxX48jRZPwAU3dEgPQm4bShxW2ED3gXTHM78gvqugB")

; Restore an EvaluationLink. This too should be in IPFGS already.
; This should return
; (EvaluationLink
;    (PredicateNode "Some relationship")
;    (ListLink
;       (ConceptNode "foo")
;       (ConceptNode "bar")
;    )
; )
(ipfs-fetch-atom "QmdQwLMqC6fyAGa6xztV6y53NqyZwiGW4fvngPtukjBraZ")

; Review the stats
(ipfs-stats)

; Close the connection.
(ipfs-close)

; The End.  That's all for now.
