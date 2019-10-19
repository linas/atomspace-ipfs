
; Proto-example
(use-modules (opencog))
(use-modules (opencog persist-ipfs))

; To view documentation, type
;    ,a ipfs
; at the guile prompt. For a given function, try, for example:
;    ,d ipfs-open

; hostname of IPFS server, and a key name ...
; (ipfs-open "ipfs://localhost/demo-atomspace")
(ipfs-open "ipfs:///demo-atomspace")
