
AtomSpace-IPFS Examples
-----------------------
The following examples all assume that you have the ipfs daemon
running on your local machine.  Do this by saying
```
$ ipfs daemon
```
at the `bash` prompt. Verify that it is working by connecting to it at
`http://localhost:5001/webui` which should bring up the IPFS explorer.
The explorer will be useful for verifying AtomSpace contents.

* basic.scm       - Creating, saving and viewing Atoms.
* bulk-save.scm   - Bulk save of an entire AtomSpaces.
* restore-keyless.scm - Fetch of individual Atoms and entire AtomSpaces.
* restore-ro.scm      - Read-only fetch of individual Atoms and AtomSpaces.
* values-save.scm     - Save of Values
* values-restore.scm  - Restore of Values, to be run after the above.
