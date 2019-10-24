;
; values.scm
;
; This provides examples of saving and restoring Atoms with Values
; attache to them.  Be sure to review the earlier example `basic.scm`
; first.
;
(use-modules (opencog))
(use-modules (opencog persist))
(use-modules (opencog persist-ipfs))

; hostname of IPFS server, and a key name ...
(ipfs-open "ipfs:///demo-atomspace")

; Create an atom
(define c (Concept "example concept"))

; Attach a SimpleTruthValue to it
(cog-set-tv! c (SimpleTruthValue 0.45 0.33))

; Publish it to IPFS.
(store-atom c)

; Examine the CID of the Atom.  This should return exactly
;     /ipfs/bafyreicjwmc5lbfbhlnfnux6dko6m4erulpwvjf5o5bgghev3fk5crmwnu
; which is NOT the same as the CID of the same Atom, without the TV on it:
;     /ipfs/bafyreigx524ryjvcjvph4eyihduqspxctgjx5gvyxhpgjbwmrifgpctmoa
; Both of these can be viewed with a network explorer. Again:
;     http://localhost:5001/webui
(ipfs-atom-cid c)

; Try again, with something more complex
(define e
	(Evaluation
		(Predicate "Some relationship")
		(List (Concept "foo") (Concept "bar"))))

(store-atom e)
(barrier)

; Likewise, view the CID for the EvaluationLink:
(ipfs-atom-cid e)

; The expected CID for the EvaluationLink is:
;
; /ipfs/bafyreigll67ssepbqfhtooqobirodyyhsq3ptmxseuwxwnxs5aci75hpoq
;
; Everyone should always get exactly these CID's. If not, then we've
; changed the code and/or there's a bug somewhere.

; Review the stats
(ipfs-stats)

; Close the connection.
(ipfs-close)

; The End.  That's all for now.
