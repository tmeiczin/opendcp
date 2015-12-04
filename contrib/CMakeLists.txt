# Third Party Libraries
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

INCLUDE(ExternalProject)

IF(NOT WIN32)
    SET(MAKE make)
    SET(CONFIGURE <SOURCE_DIR>/configure)
    SET(CMAKE cmake)
    SET(INSTALL make)
ENDIF()

SET(OPENDCP_CONTRIB_URL ${PROJECT_SOURCE_DIR}/contrib/tarballs)
IF(EXISTS ${OPENDCP_CONTRIB_URL})
    SET(DOWNLOAD 0)
    SET(URL ${OPENDCP_CONTRIB_URL})
ELSE()
    SET(DOWNLOAD 1)
ENDIF()

IF(COMPILE_ZLIB)
    IF(DOWNLOAD)
        SET(URL http://zlib.net)
    ENDIF()
    EXTERNALPROJECT_ADD(
        00_ZLIB
        URL ${URL}/zlib-1.2.5.tar.gz
        BUILD_IN_SOURCE 1
        PATCH_COMMAND patch -d <SOURCE_DIR> -p0 -t -N < ${PROJECT_SOURCE_DIR}/contrib/zlib-1.2.5.patch
        CONFIGURE_COMMAND sh <SOURCE_DIR>/mkzlib.sh ${PREFIX} ${AUTO_CFLAGS} ${AUTO_LDFLAGS}
        BUILD_COMMAND ${MAKE} -f <SOURCE_DIR>/Makefile
        INSTALL_COMMAND ${INSTALL} -f <SOURCE_DIR>/Makefile install ${INSTALL_DIR}
    )
ENDIF(COMPILE_ZLIB)

IF(COMPILE_LIBXML2)
    IF(DOWNLOAD)
        SET(URL ftp://xmlsoft.org/libxml2/)
    ENDIF()
    EXTERNALPROJECT_ADD(
        02_XML2
        URL ${URL}/libxml2-2.9.3.tar.gz
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ${CONFIGURE} ${PREFIX_ARG} ${AUTO_CFLAGS} ${AUTO_CXXFLAGS} ${AUTO_LDFLAGS}
                          --disable-dependency-tracking --enable-static=yes --enable-shared=no
                          --with-iconv=no
                          --with-threads=no
                          --with-python=no
        BUILD_COMMAND ${MAKE} -f <SOURCE_DIR>/Makefile
        INSTALL_COMMAND ${INSTALL} -f <SOURCE_DIR>/Makefile install ${INSTALL_DIR}
    )
ENDIF(COMPILE_LIBXML2)

IF(ENABLE_XMLSEC AND COMPILE_XMLSEC)
    IF(DOWNLOAD)
        SET(URL http://www.aleksey.com/xmlsec/download)
    ENDIF()
    IF(WIN32)
        SET(URL ${URL}/xmlsec1-1.2.18.tar.gz)
    ELSE()
        SET(URL ${URL}/xmlsec1-1.2.20.tar.gz)
    ENDIF()
    EXTERNALPROJECT_ADD(
        06_XMLSEC1
        URL ${URL}
        BUILD_IN_SOURCE 1
        PATCH_COMMAND patch -d <SOURCE_DIR> -p0 -t -N < ${PROJECT_SOURCE_DIR}/contrib/xmlsec1-1.2.20.patch
        CONFIGURE_COMMAND ${CONFIGURE} ${PREFIX_ARG} ${AUTO_CFLAGS} ${AUTO_CXXFLAGS} ${AUTO_LDFLAGS}
                          --enable-pkgconfig=no
                          --disable-dependency-tracking --enable-static=yes --enable-shared=no
                          --enable-crypto-dl=no
                          --enable-apps-crypto-dl=no
                          --enable-static-linking=yes
                          --with-libxml=${PREFIX}
                          --with-libxslt=no
                          --with-openssl=${PREFIX}
                          --with-default-crypto=openssl
        BUILD_COMMAND ${MAKE} -f <SOURCE_DIR>/Makefile
        INSTALL_COMMAND ${INSTALL} -f <SOURCE_DIR>/Makefile install ${INSTALL_DIR}
     )
ENDIF(ENABLE_XMLSEC AND COMPILE_XMLSEC)

IF(COMPILE_EXPAT)
    IF(DOWNLOAD)
        SET(URL http://sourceforge.net/projects/expat/files/expat/2.1.0)
    ENDIF()
    EXTERNALPROJECT_ADD(
        04_EXPAT
        URL ${URL}/expat-2.1.0.tar.gz
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ${CONFIGURE} ${PREFIX_ARG} ${AUTO_CFLAGS} ${AUTO_CXXFLAGS} ${AUTO_LDFLAGS}
                          --disable-dependency-tracking --enable-static=yes --enable-shared=no
        BUILD_COMMAND ${MAKE} -f <SOURCE_DIR>/Makefile
        INSTALL_COMMAND ${INSTALL} -f <SOURCE_DIR>/Makefile install ${INSTALL_DIR}
)
ENDIF(COMPILE_EXPAT)

SET(OPENSSL_MAKE make)
IF(APPLE)
    SET(OPENSSL_CONFIGURE <SOURCE_DIR>/Configure darwin64-x86_64-cc ${PREFIX_ARG})
ELSEIF(MINGW)
    SET(OPENSSL_CONFIGURE <SOURCE_DIR>/config mingw ${PREFIX_ARG})
ELSE()
    SET(OPENSSL_CONFIGURE <SOURCE_DIR>/config ${PREFIX_ARG})
ENDIF()

IF(COMPILE_OPENSSL)
    IF(DOWNLOAD)
        SET(URL http://www.openssl.org/source/)
    ENDIF()
    EXTERNALPROJECT_ADD(
       05_OPENSSL
       URL ${URL}/openssl-0.9.8r.tar.gz
       BUILD_IN_SOURCE 1
       PATCH_COMMAND patch -d <SOURCE_DIR> -p0 -t -N < ${PROJECT_SOURCE_DIR}/contrib/openssl-0.9.8r.patch
       CONFIGURE_COMMAND ${OPENSSL_CONFIGURE}
       BUILD_COMMAND ${OPENSSL_MAKE} -f <SOURCE_DIR>/Makefile
       INSTALL_COMMAND ${INSTALL} -f <SOURCE_DIR>/Makefile install ${INSTALL_DIR}
)
ENDIF(COMPILE_OPENSSL)

IF(PREFIX)
    SET(OPENSSL_PREFIX ${PREFIX})
    SET(EXPAT_PREFIX ${PREFIX})
ELSE()
    SET(OPENSSL_PREFIX "yes")
    SET(EXPAT_PREFIX "yes")
ENDIF()

IF(COMPILE_SNDFILE)
    IF(DOWNLOAD)
        SET(URL http://www.mega-nerd.com/libsndfile/files/)
    ENDIF()
    EXTERNALPROJECT_ADD(
        10_SNDFILE
        URL ${URL}/libsndfile-1.0.25.tar.gz
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ${CONFIGURE} ${PREFIX_ARG} ${AUTO_CFLAGS} ${AUTO_CXXFLAGS} ${AUTO_LDFLAGS}
                          --disable-dependency-tracking --enable-static=yes --enable-shared=no
        BUILD_COMMAND ${MAKE} -f <SOURCE_DIR>/Makefile
        INSTALL_COMMAND ${INSTALL} -f <SOURCE_DIR>/Makefile install ${INSTALL_DIR}
)
ENDIF(COMPILE_SNDFILE)

IF(COMPILE_SAMPLERATE)
    IF(DOWNLOAD)
        SET(URL http://www.mega-nerd.com/SRC)
    ENDIF()
    EXTERNALPROJECT_ADD(
        11_SAMPLERATE
        URL ${URL}/libsamplerate-0.1.8.tar.gz
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ${CONFIGURE} ${PREFIX_ARG} ${AUTO_CFLAGS} ${AUTO_CXXFLAGS} ${AUTO_LDFLAGS}
                          --disable-dependency-tracking --enable-static=yes --enable-shared=no
        BUILD_COMMAND ${MAKE} -f <SOURCE_DIR>/Makefile
        INSTALL_COMMAND ${INSTALL} -f <SOURCE_DIR>/Makefile install ${INSTALL_DIR}
)
ENDIF(COMPILE_SAMPLERATE)

IF(COMPILE_TIFF)
    IF(DOWNLOAD)
        SET(URL http://download.osgeo.org/libtiff/)
    ENDIF()
    EXTERNALPROJECT_ADD(
        08_TIFF
        URL ${URL}tiff-4.0.0.tar.gz
        BUILD_IN_SOURCE 1
        PATCH_COMMAND patch -d <SOURCE_DIR> -p0 -t -N < ${PROJECT_SOURCE_DIR}/contrib/libtiff-4.0.0.patch
        CONFIGURE_COMMAND ${CONFIGURE} ${PREFIX_ARG} ${AUTO_CFLAGS} ${AUTO_CXXFLAGS} ${AUTO_LDFLAGS}
                          --disable-dependency-tracking --enable-static=yes --enable-shared=no
                          --enable-jpeg=no
        BUILD_COMMAND ${MAKE} -f <SOURCE_DIR>/Makefile
        INSTALL_COMMAND ${INSTALL} -f <SOURCE_DIR>/Makefile install ${INSTALL_DIR}
)
ENDIF(COMPILE_TIFF)

IF(COMPILE_OPENJPEG)
    IF(DOWNLOAD)
        SET(URL http://sourceforge.net/projects/openjpeg.mirror/files/2.1.0/openjpeg-2.1.0.tar.gz/download)
    ENDIF()
    SET(INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
    EXTERNALPROJECT_ADD(
        09_OpenJPEG
        GIT_REPOSITORY "https://github.com/uclouvain/openjpeg.git"
        GIT_TAG "tags/version.2.1"
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX}
        CONFIGURE_COMMAND ${CMAKE} ../09_OpenJPEG -DBUILD_CODEC=OFF -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} -DBUILD_SHARED_LIBS=OFF -DOPENJPEG_INSTALL_INCLUDE_DIR=${INSTALL_PREFIX}/include -DCMAKE_BUILD_TYPE=RELEASE
        BUILD_COMMAND ${MAKE}
        INSTALL_COMMAND ${INSTALL} install
        UPDATE_COMMAND ""
    )
ENDIF(COMPILE_OPENJPEG)
