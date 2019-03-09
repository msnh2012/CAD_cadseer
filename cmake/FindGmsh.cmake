# Find the gmsh library and header.
#
# Sets the usual variables expected for find_package scripts:
#
# GMSH_INCLUDE_DIR - header location
# GMSH_LIBRARIES - library to link against
# GMSH_FOUND - true if header and lib found


FIND_PATH(GMSH_INCLUDE_DIR gmsh.h)

FIND_LIBRARY(GMSH_LIBRARY
    NAMES
    gmsh libgmsh
)

# Support the REQUIRED and QUIET arguments, and set GMSH_FOUND if found.
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Gmsh DEFAULT_MSG GMSH_LIBRARY
                                  GMSH_INCLUDE_DIR)

if(GMSH_FOUND)
    set(GMSH_LIBRARIES ${GMSH_LIBRARY})
endif()
