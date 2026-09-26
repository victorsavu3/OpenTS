# Limit the generated solution to supported configurations.
set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING "" FORCE)

#
# ---------------------------------------------------------
# Directories that build as their own project
# ---------------------------------------------------------
#
set(SUBPROJECT_DIRS
    "language"
    "vqalib"
)

#
# ---------------------------------------------------------
# Gather all potential OpenTS files
# ---------------------------------------------------------
#
file(GLOB_RECURSE OPENTS_RAW CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.rc"
)

#
# ---------------------------------------------------------
# Remove files that belong to a subproject
# ---------------------------------------------------------
#
set(OPENTS_SRC "")
foreach(f ${OPENTS_RAW})
    set(EXCLUDED FALSE)
    foreach(dir ${SUBPROJECT_DIRS})
        if(f MATCHES "^${CMAKE_CURRENT_SOURCE_DIR}/${dir}/.*")
            set(EXCLUDED TRUE)
        endif()
    endforeach()

    if(NOT EXCLUDED)
        list(APPEND OPENTS_SRC "${f}")
    endif()
endforeach()

#
# ---------------------------------------------------------
# The general glob above picks up both platforms' sources, so each build drops the other's:
# the experimental Linux build drops the files that have no non-Windows implementation and
# adds the SDL3-backed ones in their place; a normal Windows build drops those SDL3-backed
# ones, which need not compile there at all. Despite their name, ipxaddr/ipxconn/ipxgconn/
# ipxmgr.cpp already implement Internet play over UDP (see wsproto.cpp/wspudp.cpp) rather than
# the real IPX/SPX protocol, so they build on every platform. winfix.cpp's TreeView/ListView/
# WinHelp fixups exist only for the retired Win32 dialog layer's own callers, none of which
# build on this platform, so it stays Windows-only.
# ---------------------------------------------------------
#
set(OPENTS_LINUX_ONLY_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/sdlstub.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdlmain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdltranslate.cpp"
)

if(OPENTS_EXPERIMENTAL_LINUX)
    set(OPENTS_WINDOWS_ONLY_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/winstub.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/msgloop.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Sun.rc"
        "${CMAKE_CURRENT_SOURCE_DIR}/except.rc"
        "${CMAKE_CURRENT_SOURCE_DIR}/winfix.cpp"
    )
    list(REMOVE_ITEM OPENTS_SRC ${OPENTS_WINDOWS_ONLY_SRC})
else()
    list(REMOVE_ITEM OPENTS_SRC ${OPENTS_LINUX_ONLY_SRC})
endif()

#
# ---------------------------------------------------------
# Create actual OpenTS executable (Win32)
# ---------------------------------------------------------
#
message(STATUS "${PROJECT_NAME}: Creating '${PROJECT_NAME}' executable...")

add_executable(OpenTS WIN32 ${OPENTS_SRC})
target_compile_features(OpenTS PRIVATE cxx_std_20)

#
# ---------------------------------------------------------
# Output paths
# ---------------------------------------------------------
#
message(STATUS "${PROJECT_NAME}: Setting binary output paths...")

set_target_properties(OpenTS PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"

    # Built under the shipped names, so the executable, the symbol file the debug record
    # names, and the map reach the run directory without a rename.
    OUTPUT_NAME_DEBUG "GameD"
    OUTPUT_NAME_RELEASE "Game"
    PDB_NAME_DEBUG "GameD"
    PDB_NAME_RELEASE "Game"
)

set_target_properties(OpenTS PROPERTIES
    VS_DEBUGGER_COMMAND "$<TARGET_FILE:OpenTS>"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:OpenTS>"
    VS_DEBUGGER_COMMAND_ARGUMENTS "-DATADIR=${OPENTS_GAME_DIR}"
)

#
# ---------------------------------------------------------
# Compile flags
# ---------------------------------------------------------
#
message(STATUS "${PROJECT_NAME}: Applying compiler flags...")

set(OPENTS_COMPILE_OPTIONS
    # The inherited tree assumes MSVC's permissive aliasing; GCC and Clang optimize on
    # strict aliasing by default and can miscompile it without this.
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fno-strict-aliasing>

    # STYLE.md asks every implementation file to put always.h first, for its CRT-name and
    # __cdecl shims; a translation unit that does not still needs them on this platform.
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-include${CMAKE_CURRENT_SOURCE_DIR}/always.h>

    # These option spellings are MSVC's; a non-MSVC compiler takes its own defaults for
    # Debug/Release instead (CMAKE_CXX_FLAGS_DEBUG/_RELEASE).
    # ================= DEBUG =================
    $<$<CONFIG:Debug>:

        # ---------- C++ ----------
        # /arch:SSE2 -- IEEE-754 single precision, no x87 excess precision.
        # /fp:precise -- no reassociation of the engine's accumulations.
        # /utf-8 -- sources and literals are UTF-8 whatever the build machine's code page.
        $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:MSVC>>:
            /Zi /Od /RTC1 /GR /MP /EHsc /Oy- /MTd /Zc:__cplusplus /arch:SSE2 /fp:precise /utf-8
        >

        # ---------- C ----------
        $<$<AND:$<COMPILE_LANGUAGE:C>,$<C_COMPILER_ID:MSVC>>:
            /Zi /Od /RTC1 /MP /Oy- /arch:SSE2 /fp:precise /utf-8
        >
    >

    # ================= RELEASE =================
    $<$<CONFIG:Release>:

        # ---------- C++ ----------
        # /arch:SSE2 -- IEEE-754 single precision, no x87 excess precision.
        # /fp:precise -- no reassociation of the engine's accumulations.
        # /utf-8 -- sources and literals are UTF-8 whatever the build machine's code page.
        $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:MSVC>>:
            /Zi /O2 /GF /GR /MP /EHsc /MT /Zc:__cplusplus /arch:SSE2 /fp:precise /utf-8
        >

        # ---------- C ----------
        $<$<AND:$<COMPILE_LANGUAGE:C>,$<C_COMPILER_ID:MSVC>>:
            /Zi /O2 /GF /MP /arch:SSE2 /fp:precise /utf-8
        >
    >
)

target_compile_options(OpenTS PRIVATE ${OPENTS_COMPILE_OPTIONS})

#
# ---------------------------------------------------------
# Subprojects
# ---------------------------------------------------------
#
# The VQA player carries its own project, and hands OpenTS its include path along with the
# library, so the movie code is reached the same way from anywhere in the solution.
add_subdirectory(vqalib)

message(STATUS "${PROJECT_NAME}: Adding include paths...")
target_include_directories(OpenTS PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${OPENTS_GENERATED_DIR}"
)

# The generated build stamp has to exist before anything compiles.
add_dependencies(OpenTS OpenTSBuildStamp)

# bgfx's include paths and settings apply only to the two files that include it: the frame
# renderer and the UI overlay renderer.
set(BGFX_ROOT "${CMAKE_SOURCE_DIR}/thirdparty/bgfx.cmake/bgfx")
set(OPENTS_BGFX_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/bgfxbackend.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/rml/rmlrender.cpp"
)
set_source_files_properties(${OPENTS_BGFX_SOURCES} PROPERTIES
    INCLUDE_DIRECTORIES
        "${BGFX_ROOT}/include;${CMAKE_SOURCE_DIR}/thirdparty/bgfx.cmake/bx/include"
    COMPILE_DEFINITIONS
        "BX_CONFIG_DEBUG=$<IF:$<CONFIG:Debug>,1,$<BOOL:${BX_CONFIG_DEBUG}>>"
    COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>"
)

# The frame quad's shaders come from the imgui example and the overlay's from the debug
# draw example. The imgui example directory also holds an imgui.h that would shadow Dear
# ImGui's, so only the file that needs those shaders gets that path.
set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/bgfxbackend.cpp" APPEND PROPERTY
    INCLUDE_DIRECTORIES "${BGFX_ROOT}/examples/common/imgui"
)
set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/ui/rml/rmlrender.cpp" APPEND PROPERTY
    INCLUDE_DIRECTORIES "${BGFX_ROOT}/examples/common/debugdraw"
)

# writepng.cpp takes miniz from bimg, which the link already carries.
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/writepng.cpp" PROPERTIES
    INCLUDE_DIRECTORIES
        "${CMAKE_SOURCE_DIR}/thirdparty/bgfx.cmake/bimg/3rdparty/tinyexr/deps"
)

# bx rewrites __stdcall while its headers are being parsed by clang-cl.  Force the
# compatibility header into the renderer translation units as well as bx/bgfx so the
# MSVC standard-library headers that follow still see the Win32 calling convention.
if(OPENTS_EXPERIMENTAL_CLANG_CL AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    set_property(SOURCE ${OPENTS_BGFX_SOURCES} APPEND PROPERTY
        COMPILE_OPTIONS "/FI${CMAKE_SOURCE_DIR}/thirdparty/bx-clang-compat.h"
    )
endif()

# The texture loader decodes images with the stb_image header from bimg; no other file needs it.
set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/ui/rml/rmltexture.cpp" APPEND PROPERTY
    INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/thirdparty/bgfx.cmake/bimg/3rdparty/stb"
)

message(STATUS "${PROJECT_NAME}: Adding compilier definitions...")
if(WIN32)
    target_compile_definitions(OpenTS PRIVATE
        WIN32
        _WINDOWS
        NOMINMAX
    )
else()
    # code/always.h already makes this a no-op outside Windows, but only once it has been
    # included; some translation units (audio/audiomovie.cpp and its VQA player headers)
    # reach a __cdecl typedef before that happens.
    target_compile_definitions(OpenTS PRIVATE __cdecl=)
endif()

#
# ---------------------------------------------------------
# Platform libraries and linker options
# ---------------------------------------------------------
#
message(STATUS "${PROJECT_NAME}: Applying linker options...")

# Required libraries on Windows:
#  Comctl32.lib
#  DbgHelp.lib
#  DSound.lib
#  Version.lib
#  Winmm.lib
#  wsock32.lib
#  + all Win32 libs
#

target_link_libraries(OpenTS PRIVATE
    VQALib

    bgfx
    bx
    bimg
    miniaudio
    lzo
    RmlUi::Core
    imgui
)

if(WIN32)
    target_link_libraries(OpenTS PRIVATE
        comctl32
        dbghelp
        iphlpapi
        psapi
        version
        winmm
        ws2_32
        kernel32 user32 gdi32 winspool comdlg32 advapi32 shell32
        odbc32 odbccp32
    )
endif()

if(OPENTS_EXPERIMENTAL_LINUX)
    target_link_libraries(OpenTS PRIVATE SDL3::SDL3)
endif()

if(MSVC)
    target_link_options(OpenTS PRIVATE
        /SUBSYSTEM:WINDOWS
        /DEBUG
        /MAP
        # Raises the user address space of a 32-bit process from 2GB to 4GB under 64-bit
        # Windows. Pointers may then have the high bit set, so anything holding one in a
        # signed integer must only round-trip the bits and never compare or scale them. A
        # 64-bit image carries the flag already.
        /LARGEADDRESSAWARE
        # Declares the process per-monitor DPI aware, so a scaled desktop does not blur
        # the frame by stretching the whole window.
        "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/sun.manifest"
        /MANIFEST:EMBED
    )
endif()

#
# ---------------------------------------------------------
# Visual Studio source groups
# ---------------------------------------------------------
#
message(STATUS "${PROJECT_NAME}: Setting up Visual Studio filters...")

# Init Source Files (_*.cpp)
foreach(f ${OPENTS_SRC})
    if(f MATCHES "[/\\\\]_.*\\.cpp$")
        source_group("Source Files\\Init Source Files" FILES ${f})
    endif()
endforeach()

# Init Header Files (_*.h)
foreach(f ${OPENTS_SRC})
    if(f MATCHES "[/\\\\]_.*\\.h$")
        source_group("Header Files\\Init Header Files" FILES ${f})
    endif()
endforeach()

# Define Header Files (*.hh)
foreach(f ${OPENTS_SRC})
    if(f MATCHES "\\.hh$")
        source_group("Header Files\\Define Header Files" FILES ${f})
    endif()
endforeach()

# Resource Files
foreach(f ${OPENTS_SRC})
    if(f MATCHES "\\.(ico|cur|bmp|gif|jpe?g|rc)$")
        source_group("Resource Files" FILES ${f})
    endif()
endforeach()

# The interface headers the locomotors are written against.
source_group("Interface Files" FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/iflyctrl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/iloco.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ipiggy.h"
)

# General source files
source_group("Source Files" REGULAR_EXPRESSION ".*\\.(c|cpp)$")
source_group("Header Files" REGULAR_EXPRESSION ".*\\.(h|hpp)$")

#
# ---------------------------------------------------------
# UI files
# ---------------------------------------------------------
#
# The game reads the ui directory from beside the executable. A separate target copies it
# there, so an edited document reaches the output without a relink.
add_custom_target(OpenTSUIFiles ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/ui" "$<TARGET_FILE_DIR:OpenTS>/ui"
    COMMENT "Copying the UI files beside the executable"
    VERBATIM
)
add_dependencies(OpenTS OpenTSUIFiles)

#
# ---------------------------------------------------------
# Visual Studio startup project
# ---------------------------------------------------------
#
set_property(GLOBAL PROPERTY VS_STARTUP_PROJECT OpenTS)

# ---------------------------------------------------------
# Add the .natvis file for Visual Studio debugging
# ---------------------------------------------------------
if(MSVC)
    set(NATVIS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/sun.natvis")
    source_group("Natvis Files" FILES ${NATVIS_FILE})
    target_sources(OpenTS PRIVATE ${NATVIS_FILE})
    target_link_options(OpenTS PRIVATE "/NATVIS:${NATVIS_FILE}")
endif()
