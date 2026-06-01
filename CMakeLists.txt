cmake_minimum_required(VERSION 3.20)
project(nextage_api_cpp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- vcpkg packages ---
find_package(nlohmann_json CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)

# --- omniORB (system package) ---
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(omniORB REQUIRED)

# --- IDL stub generation ---
set(IDL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/idl/NxApi.idl")
set(IDL_GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated")
set(IDL_GENERATED_HH  "${IDL_GENERATED_DIR}/NxApi.hh")
set(IDL_GENERATED_SK  "${IDL_GENERATED_DIR}/NxApiSK.cc")

file(MAKE_DIRECTORY ${IDL_GENERATED_DIR})

add_custom_command(
    OUTPUT  ${IDL_GENERATED_HH} ${IDL_GENERATED_SK}
    COMMAND ${OMNIIDL_EXECUTABLE} -bcxx -Wbh=.hh -Wbs=SK.cc
            -C ${IDL_GENERATED_DIR} ${IDL_FILE}
    DEPENDS ${IDL_FILE}
    COMMENT "Generating C++ stubs from NxApi.idl"
)

add_custom_target(idl_stubs DEPENDS ${IDL_GENERATED_HH} ${IDL_GENERATED_SK})

# --- Sources ---
set(SOURCES
    src/main.cpp
    src/corba/any_helper.cpp
    src/corba/corba_client.cpp
    src/service/robot_api_service.cpp
    src/api/api_server.cpp
    ${IDL_GENERATED_SK}
)

add_executable(nextage_api ${SOURCES})
add_dependencies(nextage_api idl_stubs)

target_include_directories(nextage_api PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${IDL_GENERATED_DIR}
    ${omniORB_INCLUDE_DIRS}
)

target_link_libraries(nextage_api PRIVATE
    nlohmann_json::nlohmann_json
    httplib::httplib
    omniORB::omniORB4
    omniORB::omniDynamic4
)

if(TARGET omniORB::omnithread)
    target_link_libraries(nextage_api PRIVATE omniORB::omnithread)
endif()

# Platform threading
find_package(Threads REQUIRED)
target_link_libraries(nextage_api PRIVATE Threads::Threads)

# --- Install ---
install(TARGETS nextage_api DESTINATION bin)
