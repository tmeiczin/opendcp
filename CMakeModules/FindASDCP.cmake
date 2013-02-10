# - Try to find libASDCP
# Once done, this will define
#
#  ASDCP_FOUND - system has asdcplib
#  ASDCP_INCLUDE_DIRS - the asdcplib include directories
#  ASDCP_LIBRARIES - link these to use asdcplib


# Include dir
find_path(ASDCP_INCLUDE_DIR
  NAMES AS_DCP.h
)

find_path(KUMU_INCLUDE_DIR
  NAMES KM_error.h
)

# Finally the library itself
find_library(ASDCP_LIBRARY
  NAMES asdcp
)
find_library(KUMU_LIBRARY
  NAMES kumu
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ASDCP DEFAULT_MSG ASDCP_LIBRARY ASDCP_INCLUDE_DIR)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(KUMU DEFAULT_MSG KUMU_LIBRARY KUMU_INCLUDE_DIR)

IF(ASDCP_FOUND AND KUMU_FOUND)
  SET(ASDCP_LIBRARIES ${ASDCP_LIBRARY} ${KUMU_LIBRARY})
ENDIF(ASDCP_FOUND AND KUMU_FOUND)
