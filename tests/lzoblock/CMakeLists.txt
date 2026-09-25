# The LZO block classes are compiled straight into the harness against the same vendored
# library the engine links. It lives outside code/ so that the recursive glob building OpenTS
# cannot pick this target's entry point up.

# The vendored LZO headers read the bare WIN32/_WINDOWS macros to pick their own platform
# branch, so those only belong here when the target actually is Windows.
if(WIN32)
    set(LZOBLOCK_DEFINITIONS WIN32 _WINDOWS _MBCS NOMINMAX)
else()
    set(LZOBLOCK_DEFINITIONS NOMINMAX)
endif()

opents_add_test(LzoBlock
    NAME lzoblock
    SOURCES lzoblock.cpp
    ENGINE
        buff.cpp
        lzopipe.cpp
        lzostraw.cpp
        pipe.cpp
        straw.cpp
        xpipe.cpp
        xstraw.cpp
    DEFINITIONS ${LZOBLOCK_DEFINITIONS}
    LIBRARIES lzo
    FLOAT
)
