#--set OS Specific Compiler Flags-----------------------------------------------
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OSXMIN} -O3 -mtune=native")
SET(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS})
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-search_paths_first")
SET(AUTO_CFLAGS "CFLAGS=${OSXMIN} ${OSXSDK} -Wl,-search_paths_first -O3 -mtune=native")
SET(AUTO_CXXFLAGS "CXXFLAGS=${OSXMIN} ${OSXSDK} -Wl,-search_paths_first")
SET(AUTO_LDFLAGS "LDFLAGS=-Wl,-search_paths_first")
#-------------------------------------------------------------------------------

#--gett osx version-------------------------------------------------------------
EXEC_PROGRAM(uname ARGS -v  OUTPUT_VARIABLE DARWIN_VERSION)
STRING(REGEX MATCH "[0-9]+" DARWIN_VERSION ${DARWIN_VERSION})
IF (DARWIN_VERSION GREATER 8)
  SET(LZMA 1)
ENDIF ()
#-------------------------------------------------------------------------------


#--set os specifc linking mode--------------------------------------------------
SET(DOWNLOAD ON)
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/contrib/include)

SET(LIB_DIR ${PREFIX}/lib)

SET(LIBS ${LIBS} -lz)
#-- if osx 10.8+ uncomment below --#
IF (LZMA)
    SET(LIBS ${LIBS} -llzma)
ENDIF()
SET(LIBS ${LIBS} -L${LIB_DIR} -lssl -lcrypto)

INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/contrib/include/libxml2)
SET(COMPILE_LIBXML2 1)
SET(LIBS ${LIBS} -L${LIB_DIR} -lxml2)

IF(ENABLE_XMLSEC)
    INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/contrib/include/libxslt)
    INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/contrib/include/xmlsec1)
    SET(COMPILE_LIBXSLT 1)
    SET(COMPILE_XMLSEC 1)
    SET(LIBS ${LIBS} -L${LIB_DIR} -lxmlsec1-openssl -lxmlsec1 -lxslt)
ENDIF(ENABLE_XMLSEC)

SET(COMPILE_EXPAT 1)
SET(LIBS ${LIBS} -L${LIB_DIR} -lexpat)

SET(COMPILE_TIFF 1)
SET(LIBS ${LIBS} -L${LIB_DIR} -ltiff)

SET(COMPILE_OPENJPEG 1)
SET(LIBS ${LIBS} -L${LIB_DIR} -lopenjpeg)

#SET(COMPILE_SNDFILE 0)
#SET(LIBS ${LIBS} -L${LIB_DIR} -lsndfile)
#-------------------------------------------------------------------------------
