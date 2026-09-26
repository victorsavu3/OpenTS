# Exercises the millisecond timing contract (win_compat.h on this platform, the real Win32
# calls on Windows) directly, with no engine and no game data.

opents_add_test(TimingHelper
    NAME timinghelper
    SOURCES timinghelpertest.cpp
)
