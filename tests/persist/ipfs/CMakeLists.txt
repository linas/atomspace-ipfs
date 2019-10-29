
LINK_LIBRARIES(
    persist-ipfs
    persist
    atomspace
)


ADD_CXXTEST(BasicSaveUTest)
ADD_CXXTEST(ValueSaveUTest)
ADD_CXXTEST(PersistUTest)
ADD_CXXTEST(DeleteUTest)

# The below still need to be ported from atomspace/sql
# ADD_CXXTEST(MultiPersistUTest)
# ADD_CXXTEST(MultiUserUTest)

ADD_CXXTEST(FetchUTest)
TARGET_LINK_LIBRARIES(FetchUTest execution)
