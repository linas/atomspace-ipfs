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
; Note that we don't need any keys, if we are not publishing
; to IPNS.
; (ipfs-open "ipfs://localhost/")
; (ipfs-open "ipfs://localhost:5001/")
(ipfs-open "ipfs:///")

; Restore a single ConceptNode.  This should be in IPFS, if you've
; run the earlier examples. This should return
;     (ConceptNode "example concept")
(ipfs-fetch-atom "QmTBUxX48jRZPwAU3dEgPQm4bShxW2ED3gXTHM78gvqugB")

; Restore an EvaluationLink. This too should be in IPFS already.
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

; Or one can fetch the entire contents of the AtomSpace. The CID
; below should already be in the AtomSpace, as it was created by
; the earlier examples (specifically, the basic.scm example).
(ipfs-load-atomspace "QmT9tZttJ4gVZQwVFHWTmJYqYGAAiKEcvW9k98T5syYeYU")

; Verify the AtomSpace contents
(cog-prt-atomspace)

; Other formats are supported.  The above can also be written as
(ipfs-load-atomspace "/ipfs/QmT9tZttJ4gVZQwVFHWTmJYqYGAAiKEcvW9k98T5syYeYU")

; Also, IPNS lookups of atomspaces are supported. For example:
(ipfs-load-atomspace "/ipns/QmVkzxhCMDYisZ2QEMA5WYTjrZEPVbETZg5rehsijUxVHx")
; Caution: the above IPNS entry is in active use for develpment, and
; might resolve into any kind of crazy test atomspace. Or it might not
; resolve at all...

; Review the stats
(ipfs-stats)

; Close the connection.
(ipfs-close)

; The End.  That's all for now.
