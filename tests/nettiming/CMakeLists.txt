if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.23)
    project(NetTimingContract LANGUAGES CXX)
    enable_testing()
    set(OPENTS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
else()
    set(OPENTS_ROOT "${CMAKE_SOURCE_DIR}")
endif()

# Compile the transport timing sources directly so this harness needs no game assets.
add_executable(NetTiming
    "${CMAKE_CURRENT_SOURCE_DIR}/nettiming.cpp"
    "${OPENTS_ROOT}/code/nettime.cpp"
    "${OPENTS_ROOT}/code/nettiming.cpp"
)

add_executable(NetConnection
    "${CMAKE_CURRENT_SOURCE_DIR}/connectiontiming.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/connectionstubs.cpp"
    "${OPENTS_ROOT}/code/combuf.cpp"
    "${OPENTS_ROOT}/code/connect.cpp"
    "${OPENTS_ROOT}/code/netadmit.cpp"
    "${OPENTS_ROOT}/code/nettime.cpp"
    "${OPENTS_ROOT}/code/nettiming.cpp"
)

target_compile_definitions(NetConnection PRIVATE NOMINMAX)
# The charset flags are MSVC's, so a harness built by another compiler takes its default.
set_source_files_properties("${OPENTS_ROOT}/code/combuf.cpp" PROPERTIES
    COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/source-charset:437;/execution-charset:437>"
)

foreach(target NetTiming NetConnection)
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_include_directories(${target} PRIVATE "${OPENTS_ROOT}/code")
    target_compile_definitions(${target} PRIVATE WIN32 _WINDOWS _MBCS)
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Debug>:/MTd>>
        $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Release>:/MT>>
        $<$<CXX_COMPILER_ID:MSVC>:/EHsc>
        $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
    )
    if(MSVC)
        target_link_libraries(${target} PRIVATE kernel32 winmm)
    endif()
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endforeach()

add_test(NAME nettiming COMMAND NetTiming)
add_test(NAME netconnection COMMAND NetConnection)
