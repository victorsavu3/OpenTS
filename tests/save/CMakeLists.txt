# The harness drives the file a saved game is kept in: savefile.cpp's writer and reader over
# the field table, the compressed content, and every refusal the reader makes. It links the
# same vendored LZO library the engine does, and compiles crc.cpp for the checksum the
# header and the content carry.

# The vendored LZO headers read the bare WIN32/_WINDOWS macros to pick their own platform
# branch, so those only belong here when the target actually is Windows.
if(WIN32)
    set(SAVETEST_DEFINITIONS WIN32 _WINDOWS _MBCS NOMINMAX)
else()
    set(SAVETEST_DEFINITIONS NOMINMAX)
endif()

opents_add_test(SaveTest
    NAME save
    SOURCES savetest.cpp
    ENGINE
        crc.cpp
        file.cpp
        file_posix.cpp
        file_win.cpp
        savefile.cpp
    DEFINITIONS ${SAVETEST_DEFINITIONS}
    LIBRARIES lzo
)
