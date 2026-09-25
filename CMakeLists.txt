cmake_minimum_required(VERSION 3.23)

project(OpenTS VERSION 0.2.0 LANGUAGES C CXX)

# A SemVer prerelease label, which project() cannot carry because it takes numbers only. Set it
# to "beta1" and the version reads 0.1.0-beta1; leave it empty outside a prerelease series. The
# manual's release registry is checked against both this and the project version.
set(OPENTS_VERSION_PRERELEASE "")

# Marks a build published under the version it declares, so that it is named by that version
# alone. Every other build names the commit it came from as well.
option(OPENTS_OFFICIAL_BUILD "Build as an official release of the declared version" OFF)

option(OPENTS_EXPERIMENTAL_CLANG_CL "Build with clang-cl using the MSVC ABI" OFF)
option(OPENTS_EXPERIMENTAL_LINUX "Build a native, unsupported Linux configuration with SDL3" OFF)

# Lets a target's FOLDER place it in a Visual Studio solution folder. The bgfx submodule sets
# this as well, so declare it here rather than inherit it from a vendored build.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)


if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
   AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    if(NOT OPENTS_EXPERIMENTAL_CLANG_CL)
        message(FATAL_ERROR
            "The unsupported clang-cl experiment must be configured with "
            "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/clang-cl-msvc.cmake.")
    endif()
elseif(MSVC)
    if(MSVC_VERSION LESS 1930)
        message(FATAL_ERROR "OpenTS requires MSVC 19.30 or newer.")
    endif()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4 AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR
            "OpenTS targets Win32 and x64. Reconfigure with -A Win32 or -A x64.")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(NOT OPENTS_EXPERIMENTAL_LINUX)
        message(FATAL_ERROR
            "The unsupported native Linux build must be configured with "
            "-DOPENTS_EXPERIMENTAL_LINUX=ON.")
    endif()
else()
    message(FATAL_ERROR
        "OpenTS requires the Visual Studio 2022 MSVC toolchain. "
        "For the unsupported clang-cl experiment, configure with "
        "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/clang-cl-msvc.cmake. "
        "For the unsupported native Linux build, configure with "
        "-DOPENTS_EXPERIMENTAL_LINUX=ON.")
endif()

string(TOLOWER "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" OPENTS_ARCH)
if(NOT OPENTS_ARCH)
    set(OPENTS_ARCH "unknown")
endif()

if(NOT OPENTS_EXPERIMENTAL_LINUX)
    enable_language(RC)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OPENTS_GAME_DIR "${CMAKE_SOURCE_DIR}/Run" CACHE PATH "Tiberian Sun game data directory")

#
# ---------------------------------------------------------
# Build identity stamp
# ---------------------------------------------------------
#
# The version resources, the version displays, and the debug log's opening banner all report
# the version and commit that produced the binary. Stamping runs once here so the headers exist
# before the first compile, and again per build through OpenTSBuildStamp so that committing does
# not require a reconfigure to be reflected. The UI shell's string-name table is generated from
# language.h on the same schedule.
set(OPENTS_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
set(OPENTS_VERSION_HEADER "${OPENTS_GENERATED_DIR}/opents_version.h")
set(OPENTS_STAMP_HEADER "${OPENTS_GENERATED_DIR}/opents_build.h")
set(OPENTS_STRINGS_HEADER "${OPENTS_GENERATED_DIR}/opents_strings.h")
file(MAKE_DIRECTORY "${OPENTS_GENERATED_DIR}")

set(OPENTS_STAMP_ARGS
    "-DOPENTS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}"
    "-DOPENTS_VERSION_MINOR=${PROJECT_VERSION_MINOR}"
    "-DOPENTS_VERSION_PATCH=${PROJECT_VERSION_PATCH}"
    "-DOPENTS_VERSION_PRERELEASE=${OPENTS_VERSION_PRERELEASE}"
    "-DOPENTS_OFFICIAL_BUILD=${OPENTS_OFFICIAL_BUILD}"
    "-DOPENTS_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
    "-DOPENTS_VERSION_HEADER=${OPENTS_VERSION_HEADER}"
    "-DOPENTS_STAMP_HEADER=${OPENTS_STAMP_HEADER}"
    "-DOPENTS_ARCH=${OPENTS_ARCH}"
    -P "${CMAKE_SOURCE_DIR}/cmake/GitStamp.cmake"
)

set(OPENTS_STRINGS_ARGS
    "-DOPENTS_LANGUAGE_HEADER=${CMAKE_SOURCE_DIR}/code/language/language.h"
    "-DOPENTS_STRINGS_HEADER=${OPENTS_STRINGS_HEADER}"
    -P "${CMAKE_SOURCE_DIR}/cmake/StringTable.cmake"
)

execute_process(COMMAND ${CMAKE_COMMAND} ${OPENTS_STAMP_ARGS})
execute_process(COMMAND ${CMAKE_COMMAND} ${OPENTS_STRINGS_ARGS})

set(OPENTS_STAMP_COMMANDS
    COMMAND ${CMAKE_COMMAND} ${OPENTS_STAMP_ARGS}
    COMMAND ${CMAKE_COMMAND} ${OPENTS_STRINGS_ARGS}
)

# The experimental Linux build has no Language.dll to load its strings from at run time
# (see the "Language string loading" note in code/data.cpp); it compiles them in instead.
if(OPENTS_EXPERIMENTAL_LINUX)
    set(OPENTS_LANGUAGE_STRINGS_HEADER "${OPENTS_GENERATED_DIR}/opents_languagestrings.h")
    set(OPENTS_LANGUAGE_STRINGS_ARGS
        "-DOPENTS_LANGUAGE_HEADER=${CMAKE_SOURCE_DIR}/code/language/language.h"
        "-DOPENTS_LANGUAGE_RC=${CMAKE_SOURCE_DIR}/code/language/language.rc"
        "-DOPENTS_LANGUAGE_STRINGS_HEADER=${OPENTS_LANGUAGE_STRINGS_HEADER}"
        -P "${CMAKE_SOURCE_DIR}/cmake/LanguageStrings.cmake"
    )
    execute_process(COMMAND ${CMAKE_COMMAND} ${OPENTS_LANGUAGE_STRINGS_ARGS})
    list(APPEND OPENTS_STAMP_COMMANDS COMMAND ${CMAKE_COMMAND} ${OPENTS_LANGUAGE_STRINGS_ARGS})
endif()

add_custom_target(OpenTSBuildStamp ALL
    ${OPENTS_STAMP_COMMANDS}
    COMMENT "Stamping the build identity and the string-name table"
    VERBATIM
)

add_subdirectory(thirdparty)
add_subdirectory(code)

# Language.dll is a Windows resource-only DLL; the experimental Linux build compiles its
# strings into the binary instead (see cmake/LanguageStrings.cmake).
if(NOT OPENTS_EXPERIMENTAL_LINUX)
    add_subdirectory(code/language)
endif()

enable_testing()
add_subdirectory(tests)
