# FindOmniORB.cmake
# Locates omniORB headers and libraries from a system installation.
#
# Usage:
#   find_package(omniORB REQUIRED)
#   target_link_libraries(my_target PRIVATE omniORB::omniORB4 omniORB::omniDynamic4)
#
# Hints (set before find_package if non-standard install):
#   OMNIORB_ROOT   - root of omniORB install prefix
#
# Exports:
#   omniORB::omniORB4       - core CORBA library
#   omniORB::omniDynamic4   - DynAny library
#   omniORB_FOUND
#   omniORB_INCLUDE_DIRS
#   omniORB_VERSION

set(_omniorb_search_roots
    ${OMNIORB_ROOT}
    $ENV{OMNIORB_ROOT}
    /usr
    /usr/local
    /opt/homebrew
    /opt/homebrew/opt/omniorb
    /opt/local
)

# --- Headers ---
find_path(omniORB_INCLUDE_DIR
    NAMES omniORB4/CORBA.h
    PATHS ${_omniorb_search_roots}
    PATH_SUFFIXES include
)

# --- Core library ---
find_library(omniORB4_LIBRARY
    NAMES omniORB4
    PATHS ${_omniorb_search_roots}
    PATH_SUFFIXES lib lib64
)

# --- DynAny library ---
find_library(omniDynamic4_LIBRARY
    NAMES omniDynamic4
    PATHS ${_omniorb_search_roots}
    PATH_SUFFIXES lib lib64
)

# --- omnithread (usually needed) ---
find_library(omnithread_LIBRARY
    NAMES omnithread
    PATHS ${_omniorb_search_roots}
    PATH_SUFFIXES lib lib64
)

# --- omniidl tool ---
find_program(OMNIIDL_EXECUTABLE
    NAMES omniidl
    PATHS ${_omniorb_search_roots}
    PATH_SUFFIXES bin
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(omniORB
    REQUIRED_VARS omniORB_INCLUDE_DIR omniORB4_LIBRARY omniDynamic4_LIBRARY OMNIIDL_EXECUTABLE
)

if(omniORB_FOUND AND NOT TARGET omniORB::omniORB4)
    set(omniORB_INCLUDE_DIRS ${omniORB_INCLUDE_DIR})

    add_library(omniORB::omniORB4 UNKNOWN IMPORTED)
    set_target_properties(omniORB::omniORB4 PROPERTIES
        IMPORTED_LOCATION "${omniORB4_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${omniORB_INCLUDE_DIR}"
    )

    add_library(omniORB::omniDynamic4 UNKNOWN IMPORTED)
    set_target_properties(omniORB::omniDynamic4 PROPERTIES
        IMPORTED_LOCATION "${omniDynamic4_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${omniORB_INCLUDE_DIR}"
    )

    if(omnithread_LIBRARY)
        add_library(omniORB::omnithread UNKNOWN IMPORTED)
        set_target_properties(omniORB::omnithread PROPERTIES
            IMPORTED_LOCATION "${omnithread_LIBRARY}"
        )
    endif()
endif()

mark_as_advanced(
    omniORB_INCLUDE_DIR omniORB4_LIBRARY omniDynamic4_LIBRARY
    omnithread_LIBRARY OMNIIDL_EXECUTABLE
)
