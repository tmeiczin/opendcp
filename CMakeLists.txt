CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#--build type-------------------------------------------------------------------
PROJECT(OpenDCP)
SET(CMAKE_BUILD_TYPE Release)
STRING(TOLOWER ${PROJECT_NAME} PACKAGE_NAME)
#-------------------------------------------------------------------------------

#--version----------------------------------------------------------------------
SET(VERSION_MAJOR 0)
SET(VERSION_MINOR 31)
SET(VERSION_BUILD 0)
SET(OPENDCP_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUILD})
SET(OPENDCP_NAME ${PROJECT_NAME})
SET(OPENDCP_COPYRIGHT "(c) 2010-2014 Terrence Meiczinger. All rights reserved.")
SET(OPENDCP_LICENSE "The program is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.\\n\\nDedicated in memory of James Meiczinger")
SET(OPENDCP_URL "http://www.opendcp.org")
#-------------------------------------------------------------------------------

#--options----------------------------------------------------------------------
OPTION(ENABLE_XMLSEC   "Enable XML digital singatures and security features" ON)
OPTION(ENABLE_OPENMP   "Enable OPENMP multithreading" ON)
OPTION(ENABLE_RAGNAROK "Enable Ragnarok Encoder" OFF)
OPTION(ENABLE_GUI      "Enable GUI compiling" ON)
OPTION(ENABLE_CLANG    "Enable CLANG compiling (OSX)" ON)
OPTION(ENABLE_DEBUG    "Enable debug symbols" OFF)
OPTION(BUILD_STATIC    "Enable debug symbols" ON)
OPTION(GENERATE_LANGUAGE_FILES "Generate Translation files" OFF)
OPTION(RPM             "Create RPM package" OFF)
OPTION(DEB             "Create DEB package" OFF)
#-------------------------------------------------------------------------------

#--check 32 or 64-bit-----------------------------------------------------------
IF(CMAKE_SIZEOF_VOID_P MATCHES "8")
    SET(HOST_ARCH x86_64)
ELSE()
    SET(HOST_ARCH i686)
ENDIF()
SET(TARGET_ARCH ${HOST_ARCH})
#-------------------------------------------------------------------------------

#--set optional packages--------------------------------------------------------
IF(ENABLE_XMLSEC)
    ADD_DEFINITIONS(-DXMLSEC)
    ADD_DEFINITIONS(-DLIBXML_STATIC)
    ADD_DEFINITIONS(-DLIBXSLT_STATIC)
    ADD_DEFINITIONS(-DXMLSEC_STATIC)
ENDIF()

IF(ENABLE_RAGNAROK)
    ADD_DEFINITIONS(-DHAVE_RAGNAROK)
    SET(LIB_RAGNAROK "-lopendcp-ragnarok")
ENDIF()

ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64)
#-------------------------------------------------------------------------------

#--cmake options----------------------------------------------------------------
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
#-------------------------------------------------------------------------------

#--set base compiler flags------------------------------------------------------
IF(ENABLE_DEBUG)
    SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -g")
ENDIF()
SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wall -Wextra -O3 ${OPENMP}")
SET(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS})
#-------------------------------------------------------------------------------

#--build paths------------------------------------------------------------------
INCLUDE_DIRECTORIES(${OPENDCP_INCLUDE_DIR}/libopendcp
                    ${PROJECT_BINARY_DIR}/libopendcp/include
                    ${PROJECT_SOURCE_DIR}/libopendcp
                    ${PROJECT_SOURCE_DIR}/libopendcp/codecs
                    ${PROJECT_SOURCE_DIR}/libasdcp
                    ${PROJECT_SOURCE_DIR}/libcrypto
                    ${PROJECT_SOURCE_DIR}
)
#-------------------------------------------------------------------------------

#--contrib paths----------------------------------------------------------------
SET(PREFIX ${PROJECT_BINARY_DIR}/contrib)
SET(PREFIX_ARG --prefix=${PREFIX})
#-------------------------------------------------------------------------------

#--inlcude os specific----------------------------------------------------------
IF (APPLE)
    INCLUDE("toolchains/osx.cmake")
ELSEIF(WIN32)
    INCLUDE("toolchains/win32.cmake")
ELSE()
    INCLUDE("toolchains/linux.cmake")
ENDIF()
#-------------------------------------------------------------------------------

MESSAGE(STATUS)
MESSAGE(STATUS "================================================================================")
MESSAGE(STATUS "OpenDCP Version ${OPENDCP_VERSION} CMake - ${CMAKE_SYSTEM_NAME} (${TARGET_ARCH})")
MESSAGE(STATUS "================================================================================")

#--add source directories-------------------------------------------------------
ADD_SUBDIRECTORY(contrib)
ADD_SUBDIRECTORY(libcrypto)
ADD_SUBDIRECTORY(libasdcp)
ADD_SUBDIRECTORY(libopendcp)
ADD_SUBDIRECTORY(cli)
IF(ENABLE_GUI)
    ADD_SUBDIRECTORY(gui)
ENDIF()
#-------------------------------------------------------------------------------

#--build packages---------------------------------------------------------------
ADD_SUBDIRECTORY(packages)
#-------------------------------------------------------------------------------

#--upload packages--------------------------------------------------------------
ADD_SUBDIRECTORY(upload)
#-------------------------------------------------------------------------------
