cmake_minimum_required(VERSION 2.8)

MESSAGE(STATUS)
MESSAGE(STATUS "-------------------------------------------------------------------------------")
MESSAGE(STATUS "Creating LIBCRYPTO Library Build Files")
MESSAGE(STATUS "-------------------------------------------------------------------------------")

#--Version----------------------------------------------------------------------
SET(CRYPTO_VERSION_MAJOR 1)
SET(CRYPTO_VERSION_MINOR 0)
SET(CRYPTO_VERSION_BUILD 0)
SET(CRYPTO_VERSION "${CRYPTO_VERSION_MAJOR}.${CRYPTO_VERSION_MINOR}.${CRYPTO_VERSION_BUILD}")
ADD_DEFINITIONS(-DCRYPTO_VERSION="${CRYPTO_VERSION}")
ADD_DEFINITIONS(-DPACKAGE_VERSION="${CRYPTO_VERSION}")
#-------------------------------------------------------------------------------

#--OS specific flags------------------------------------------------------------
IF(WIN32)
    SET(CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
    SET(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS})
ENDIF()
#-------------------------------------------------------------------------------

#--set source files-------------------------------------------------------------
SET(CRYPTO_SRC_FILES
    md5.c
    sha1.c
    aes.c
)
#-------------------------------------------------------------------------------

#--set header files-------------------------------------------------------------
SET(CRYPTO_HDR_FILES
    md5.h
    sha1.h
    aes.h
)


#--compile libraries------------------------------------------------------------
SET(LIB_CRYPTO opendcp-crypto PARENT_SCOPE)
ADD_LIBRARY(opendcp-crypto OBJECT ${CRYPTO_SRC_FILES})
#-------------------------------------------------------------------------------

#--install header---------------------------------------------------------------
IF(INSTALL_LIB)
    INSTALL(FILES ${CRYPTO_HDR_FILES} DESTINATION include)
ENDIF()
#-------------------------------------------------------------------------------
