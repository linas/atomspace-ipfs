

ADD_LIBRARY (persist-ipfs SHARED
	IPFSAtomDelete
	IPFSAtomLoad
	IPFSAtomStorage
	IPFSAtomStore
	IPFSBulk
	IPFSIncoming
	IPFSValues
	IPFSPersistSCM
)

TARGET_LINK_LIBRARIES(persist-ipfs
	smob
	ipfs-http-client
)

ADD_GUILE_EXTENSION(SCM_CONFIG persist-ipfs "opencog-ext-path-persist-ipfs")

INSTALL (TARGETS persist-ipfs EXPORT AtomSpaceTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

# yo.

ADD_EXECUTABLE(sniff
	sniff
)

TARGET_LINK_LIBRARIES(sniff
	ipfs-api
	curl
)
