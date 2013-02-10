#--set OS Specific Compiler Flags-----------------------------------------------
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOPJ_STATIC -DKM_WIN32 -static")
SET(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS}) 
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
#-------------------------------------------------------------------------------

#--set os specifc linking mode-------------------------------------------------
SET(SYSROOT_PATH ${CMAKE_INSTALL_PREFIX})
SET(LIB_DIR ${SYSROOT_PATH}/lib)
SET(PREFIX ${SYSROOT_PATH})
SET(PREFIX_ARG --prefix=${PREFIX})

IF(TARGET_ARCH STREQUAL "i686")
    SET(CMAKE_RC_COMPILER /usr/bin/i686-w64-mingw32-windres) 
ELSE()
    SET(CMAKE_RC_COMPILER /usr/bin/x86_64-w64-mingw32-windres) 
ENDIF()

INCLUDE_DIRECTORIES(${SYSROOT_PATH}/include/libxml2)

IF(ENABLE_XMLSEC)
    INCLUDE_DIRECTORIES(${SYSROOT_PATH}/include/libxslt)
    INCLUDE_DIRECTORIES(${SYSROOT_PATH}/include/xmlsec1)
    SET(COMPILE_XMLSEC 1)
    SET(LIBS ${LIBS} ${LIB_DIR}/libxmlsec1-openssl.a ${LIB_DIR}/libxmlsec1.a)
    SET(LIBS ${LIBS} -L${LIB_DIR} -lxslt)
ENDIF(ENABLE_XMLSEC)

SET(COMPILE_LIBXML2 0)
SET(LIBS ${LIBS} -L{LIB_DIR} -lxml2)

SET(COMPILE_EXPAT 0)
SET(LIBS ${LIBS} -L${LIB_DIR} -lexpat)

SET(COMPILE_OPENJPEG 1)
SET(LIBS ${LIBS} ${LIB_DIR}/libopenjpeg.a)

SET(LIBS ${LIBS} ${LIB_DIR}/libtiff.a ${LIB_DIR}/libjpeg.a)
SET(LIBS ${LIBS} -L${LIB_DIR} -lcrypto -lssl)
SET(LIBS ${LIBS} -L${LIB_DIR} -lz -lws2_32)
#-------------------------------------------------------------------------------

IF(TARGET_ARCH STREQUAL x86_64)
    SET(MING mingw64)
ELSE()
    SET(MING mingw32)
ENDIF()

 SET(CONFIGURE ${MING}-configure)
 SET(MAKE ${MING}-make)
 SET(INSTALL sudo make)
