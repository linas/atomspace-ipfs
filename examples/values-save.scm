;
; values-save.scm
;
; This provides examples of saving Atoms with Values attached to them.
; A second part of this example can be found in `values-restore.scm`.
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

; A more complex example...
(cog-set-value! c (Predicate "position") (FloatValue 1.1 2.2 3.3))
(cog-set-value! c (Predicate "velocity") (FloatValue 4.4 5.5 6.6))
(store-atom c)

; The CID of the above whould be
;    bafyreifhaduwgp46ho6odbjcdjgd6v3r2hxztonufgtzqgaebowrnhia5a

; Try again, with something more complex
(define e
	(Evaluation
		(Predicate "Some relationship")
		(List (Concept "foo") (Concept "bar"))))

(cog-set-value! e (Predicate "position") (FloatValue 3 2 1))
(cog-set-value! e (Predicate "velocity") (FloatValue 42 41 40))
(store-atom e)

; The expected CID for the EvaluationLink is:
;
; /ipfs/bafyreiaak6j7psknn5id7d456jaxaqxq7xjczmi7boj4zg6pqhgr5oeuuu
;

; Close the connection.
(ipfs-close)

; The End.  For part two of this example, see `values-restore.scm`.
