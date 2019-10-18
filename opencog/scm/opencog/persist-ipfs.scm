;
; OpenCog SQL Persistance module
;

(define-module (opencog persist-ipfs))


(use-modules (opencog))
(use-modules (opencog ipfs-config))
(load-extension (string-append opencog-ext-path-persist-ipfs "libpersist-ipfs") "opencog_persist_ipfs_init")

(export ipfs-clear-cache ipfs-clear-stats ipfs-close ipfs-load ipfs-open
	ipfs-store ipfs-stats)

(set-procedure-property! ipfs-clear-cache 'documentation
"
 ipfs-clear-cache - clear the TLB of cached data
    This will free up RAM, maybe a lot, dependeing on how many atoms
    are in the cache. The cost of doing this is that some operations
    on atoms will no longer be cached, and will need to go to the
    database to fetch contents, potentially impacting performance.
")

(set-procedure-property! ipfs-clear-stats 'documentation
"
 ipfs-clear-stats - reset the performance statistics counters.
    This will zero out the various counters used to track the
    performance of the SQL backend.  Statistics will continue to
    be accumulated.
")

(set-procedure-property! ipfs-close 'documentation
"
 ipfs-close - close the currently open SQL backend.
    Close open connections to the currently-open backend, afterflushing
    any pending writes in the write queues. After the close, atoms can
    no longer be stored to or fetched from the database.
")

(set-procedure-property! ipfs-load 'documentation
"
 ipfs-load - load all atoms in the database.
    This will cause ALL of the atoms in the open database to be loaded
    into the atomspace. This can be a very time-consuming operation.
    In normal operation, it is rarely necessary to load all atoms;
    atoms can always be fetched and stored one at a time, on demand.
")

(set-procedure-property! ipfs-open 'documentation
"
 ipfs-open URL - Open a connection to a database
    Open a connection to the database encoded in the URL. All
    appropriate database credentials must be supplied in the URL,
    including the username and password, if required.

    The URL must be on one of these formats:
       odbc://USER:PASSWORD/DBNAME
       postgres://USER@HOST/DBNAME
       postgres://USER:PASSWORD@HOST/DBNAME
       postgres:///DBNAME?user=USER
       postgres:///DBNAME?user=USER&host=HOST
       postgres:///DBNAME?user=USER&password=PASS

    Other key-value pairs following the question-mark are interpreted
    by the postgres driver, according to postgres documentation.

  Examples of use with valid URL's:
     (ipfs-open \"odbc://opencog_tester:cheese/opencog_test\")
     (ipfs-open \"postgres://opencog_tester@localhost/opencog_test\")
     (ipfs-open \"postgres://opencog_tester:cheese@localhost/opencog_test\")
     (ipfs-open \"postgres:///opencog_test?user=opencog_tester\")
     (ipfs-open \"postgres:///opencog_test?user=opencog_tester&host=localhost\")
     (ipfs-open \"postgres:///opencog_test?user=opencog_tester&password=cheese\")
")

(set-procedure-property! ipfs-store 'documentation
"
 ipfs-store - Store all atoms in the atomspace to the database.
    This will dump the ENTIRE contents of the atomspace to the databse.
    Depending on the size of the database, this can potentially take a
    lot of time.  During normal operation, a bulk-save is rarely
    required, as individual atoms can always be stored, one at a time.
")

(set-procedure-property! ipfs-stats 'documentation
"
 ipfs-stats - report performance statistics.
    This will cause some database performance statistics to be printed
    to the stdout of the server. These statistics can be quite arcane
    and are useful primarily to the developers of the database backend.
")
