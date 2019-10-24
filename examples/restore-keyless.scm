;
; restore-keyless.scm
;
; Simple example of restoring individual atoms, and the bulk restore of
; entire AtomSpaces, assuming that the CID's of the Atoms and AtomSpace
; is known by some other means.
;
(use-modules (opencog))
(use-modules (opencog persist))
(use-modules (opencog persist-ipfs))

; Open connection to the IPFS server.
; Note that we don't need any keys, if we are not publishing to IPNS.
; This does, however, require explicit knowledge of the CID's of
; individual Atoms, or of the AtomSpaces, which are obtained by some
; other mechanism.
;
; (ipfs-open "ipfs://localhost/")
; (ipfs-open "ipfs://localhost:5001/")
(ipfs-open "ipfs:///")

; Restore a single ConceptNode.  This should be in IPFS, if you've
; run the earlier examples. This should return
;     (ConceptNode "example concept")
(ipfs-fetch-atom "bafyreigx524ryjvcjvph4eyihduqspxctgjx5gvyxhpgjbwmrifgpctmoa")

; Restore an EvaluationLink. This too should be in IPFS already.
; This should return
; (EvaluationLink
;    (PredicateNode "Some relationship")
;    (ListLink
;       (ConceptNode "foo")
;       (ConceptNode "bar")
;    )
; )
(ipfs-fetch-atom "bafyreigll67ssepbqfhtooqobirodyyhsq3ptmxseuwxwnxs5aci75hpoq")

; Review the stats
(ipfs-stats)

; Or one can fetch the entire contents of the AtomSpace. The CID
; below should already be in the AtomSpace, as it was created by
; the earlier examples (specifically, the basic.scm example).
(ipfs-load-atomspace "QmVz6C1F1xYYYZM3L8a3BpGd41pY35SbkAL4P4pGGbgK4w")

; Verify the AtomSpace contents
(cog-prt-atomspace)

; Other formats are supported.  The above can also be written as
(ipfs-load-atomspace "/ipfs/QmVz6C1F1xYYYZM3L8a3BpGd41pY35SbkAL4P4pGGbgK4w")

; Also, IPNS lookups of atomspaces are supported. For example:
(ipfs-load-atomspace "/ipns/QmVkzxhCMDYisZ2QEMA5WYTjrZEPVbETZg5rehsijUxVHx")
; Caution: the above IPNS entry is in active use for develpment, and
; might resolve into any kind of crazy test atomspace. Or it might not
; resolve at all... Note also: at this time, IPNS resolution can take
; a minute or longer, due to a well-known and still unfixed IPFS bug.

; Review the stats
(ipfs-stats)

; Close the connection.
(ipfs-close)

; The End.  That's all for now.
