;
; values-restore.scm
;
; This provides examples of restoring Atoms with Values attached to them.
; Be sure to review and run the earlier example `values-save.scm` first,
; otherwise this example won't work.
;
(use-modules (opencog))
(use-modules (opencog persist))
(use-modules (opencog persist-ipfs))

; hostname of IPFS server. No key-name is needed.
(ipfs-open "ipfs:///")

; Fetch the previously published ConceptNode
(ipfs-fetch-atom "bafyreifhaduwgp46ho6odbjcdjgd6v3r2hxztonufgtzqgaebowrnhia5a")

; Verify the Keys and the Values are what were previousl published:
(define c (ConceptNode "example concept"))
(cog-keys c)

; Print them all, too.
(for-each
	(lambda (key)
		(format #t "Key ~A   has value ~A\n" key (cog-value c key)))
	(cog-keys c))

; Fetch the previously-stored EvaluationLink
(define e
	(ipfs-fetch-atom "bafyreiaak6j7psknn5id7d456jaxaqxq7xjczmi7boj4zg6pqhgr5oeuuu"))

(cog-keys e)

; Print them all, too.
(for-each
	(lambda (key)
		(format #t "Key ~A   has value ~A\n" key (cog-value e key)))
	(cog-keys e))

; Close the connection.
(ipfs-close)

; The End.  That's all for now.
