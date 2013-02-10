cmake_minimum_required(VERSION 2.8)

MESSAGE(STATUS)
MESSAGE(STATUS "-------------------------------------------------------------------------------")
MESSAGE(STATUS "Creating ASDCPLIB Library Build Files")
MESSAGE(STATUS "-------------------------------------------------------------------------------")

#--Version----------------------------------------------------------------------
SET(ASDCP_VERSION_MAJOR 1)
SET(ASDCP_VERSION_MINOR 8)
SET(ASDCP_VERSION_BUILD 44)
SET(ASDCP_VERSION "${ASDCP_VERSION_MAJOR}.${ASDCP_VERSION_MINOR}.${ASDCP_VERSION_BUILD}")
ADD_DEFINITIONS(-DASDCP_VERSION="${ASDCP_VERSION}")
ADD_DEFINITIONS(-DPACKAGE_VERSION="${ASDCP_VERSION}")
IF(WIN32)
    SET(PLATFORM "win32")
ELSEIF(APPLE)
    SET(PLATFORM "osx")
ELSE()
    SET(PLATFORM "unix")
ENDIF()
ADD_DEFINITIONS(-DASDCP_PLATFORM="${PLATFORM}")
ADD_DEFINITIONS(-DHAVE_EXPAT=1)
#-------------------------------------------------------------------------------

#--OS specific flags------------------------------------------------------------
IF(WIN32)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DKM_WIN32")
    SET(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS})
ENDIF()
#-------------------------------------------------------------------------------

#--set source files-------------------------------------------------------------
SET(KUMU_SRC_FILES
    KM_fileio.cpp
    KM_log.cpp
    KM_prng.cpp
    KM_util.cpp
    KM_xml.cpp
    KM_tai.cpp
)

SET(ASDCP_SRC_FILES
    AS_DCP.cpp
    AS_DCP_AES.cpp
    AS_DCP_JP2K.cpp
    AS_DCP_MPEG2.cpp
    AS_DCP_MXF.cpp
    AS_DCP_PCM.cpp
    AS_DCP_TimedText.cpp
    Dict.cpp
    h__Reader.cpp
    h__Writer.cpp
    Index.cpp
    JP2K_Codestream_Parser.cpp
    JP2K.cpp
    JP2K_Sequence_Parser.cpp
    KLV.cpp
    Metadata.cpp
    MDD.cpp
    MPEG2_Parser.cpp
    MPEG.cpp
    MXF.cpp
    MXFTypes.cpp
    PCM_Parser.cpp
    PCMParserList.cpp
    TimedText_Parser.cpp
    Wav.cpp
)
#-------------------------------------------------------------------------------

#--compile libraries------------------------------------------------------------
IF(BUILD_SHARED)
    ADD_LIBRARY(opendcp-kumu-shared  SHARED ${KUMU_SRC_FILES})
    ADD_LIBRARY(opendcp-asdcp-shared SHARED ${ASDCP_SRC_FILES})
    SET_TARGET_PROPERTIES(opendcp-kumu-shared PROPERTIES OUTPUT_NAME "opendcp-kumu")
    SET_TARGET_PROPERTIES(opendcp-asdcp-shared PROPERTIES OUTPUT_NAME "opendcp-asdcp")
    TARGET_LINK_LIBRARIES(opendcp-kumu-shared opendcp-asdcp-shared ${LIB})
    IF(INSTALL_LIB)
        INSTALL(TARGETS opendcp-asdcp-shared opendcp-kumu-shared DESTINATION ${LIB_INSTALL_PATH})
    ENDIF()
ELSE()
    SET(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/libasdcp/")
    ADD_LIBRARY(opendcp-kumu  STATIC ${KUMU_SRC_FILES})
    ADD_LIBRARY(opendcp-asdcp STATIC ${ASDCP_SRC_FILES})
    TARGET_LINK_LIBRARIES(opendcp-asdcp opendcp-kumu ${LIBS})
    IF(INSTALL_LIB)
        INSTALL(TARGETS opendcp-asdcp opendcp-kumu DESTINATION ${LIB_INSTALL_PATH})
    ENDIF()
ENDIF()
#-------------------------------------------------------------------------------

#--install header---------------------------------------------------------------
IF(INSTALL_LIB)
    INSTALL(FILES AS_DCP.h DESTINATION include)
ENDIF()
#-------------------------------------------------------------------------------
