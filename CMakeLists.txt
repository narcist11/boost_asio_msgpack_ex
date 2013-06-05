cmake_minimum_required (VERSION 2.8)

# msgpack test
project (msgpack_test)

include_directories ("${msgpack_test_SOURCE_DIR}/include")

set (cxx_flags "")

set (link_flags )
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set (link_flags "${link_flags} -pthread")
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

# msgpack test server
add_executable (mts
                src/msgpack_test_server.cpp
                )

target_link_libraries (mts msgpack boost_system)

set_target_properties (mts PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY
    "${msgpack_test_BINARY_DIR}"
    COMPILE_FLAGS
    "${CMAKE_CXX_FLAGS} ${cxx_flags}"
    LINK_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} ${link_flags}"
    )

# msgpack test client
add_executable (mtc
                src/msgpack_test_client.cpp
                )

target_link_libraries (mtc msgpack boost_system)

set_target_properties (mtc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY
    "${msgpack_test_BINARY_DIR}"
    COMPILE_FLAGS
    "${CMAKE_CXX_FLAGS} ${cxx_flags}"
    LINK_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} ${link_flags}"
    )

