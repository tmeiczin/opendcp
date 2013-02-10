cmake_minimum_required(VERSION 2.6)

MESSAGE(STATUS)
MESSAGE(STATUS "-------------------------------------------------------------------------------")
MESSAGE(STATUS "Creating Package Files")
MESSAGE(STATUS "-------------------------------------------------------------------------------")

#--cpackage-------------------------------------------------------------------
SET(CPACK_PACKAGE_NAME ${PACKAGE_NAME})
SET(CPACK_PACKAGE_VERSION ${OPENDCP_VERSION})
SET(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${VERSION_BUILD})
SET(CPACK_PACKAGE_ICON ${PROJECT_SOURCE_DIR}/gui/resources/opendcp.icns) 
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.txt")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenDCP Digital Cinema Package Tool")
SET(CPACK_PACKAGE_DESCRIPTION "This tool is used to created digital cinema packages (DCP)")
SET(CPACK_PACKAGE_VENDOR "OpenDCP")
sET(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.txt)
SET(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYRIGHT.txt")
SET(DOCUMENTATION_PATH share/doc/${CPACK_PACKAGE_NAME})
SET(SHARE_PATH         share/${CPACK_PACKAGE_NAME})
#-----------------------------------------------------------------------------

#--Win32 package--------------------------------------------------------------
IF(WIN32)
    IF(TARGET_ARCH STREQUAL "i686")
        SET(TARGET_ARCH i386)
        SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
    ELSE()
        SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    ENDIF()
    FILE(GLOB GUI_DLLS
             ${SYSROOT_PATH}/bin/zlib1.dll
             ${SYSROOT_PATH}/bin/libpng*.dll
             ${SYSROOT_PATH}/bin/QtGui4.dll
             ${SYSROOT_PATH}/bin/QtCore4.dll
    )
    SET(COMMON_DLLS 
             ${SYSROOT_PATH}/bin/libgomp-1.dll
             ${SYSROOT_PATH}/bin/pthreadGC2.dll
             ${SYSROOT_PATH}/bin/libgcc_s_sjlj-1.dll
             ${SYSROOT_PATH}/bin/libstdc++-6.dll
             ${SYSROOT_PATH}/bin/libcrypto-10.dll
             ${SYSROOT_PATH}/bin/libssl-10.dll
             ${SYSROOT_PATH}/bin/libxml2-2.dll
             ${SYSROOT_PATH}/bin/libxslt-1.dll
             ${SYSROOT_PATH}/bin/libexpat-1.dll
             ${SYSROOT_PATH}/bin/iconv.dll
    )
    INSTALL(FILES ${GUI_DLLS}    DESTINATION bin)
    INSTALL(FILES ${COMMON_DLLS} DESTINATION bin)

    STRING(TOLOWER "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}.${TARGET_ARCH}" CPACK_PACKAGE_FILE_NAME)
    SET(CPACK_GENERATOR NSIS)
    SET(CPACK_NSIS_PACKAGE_NAME ${PROJECT_NAME})
    SET(CPACK_NSIS_DISPLAY_NAME ${PROJECT_NAME})
    SET(CPACK_PACKAGE_INSTALL_DIRECTORY ${PROJECT_NAME})
    SET(CPACK_PACKAGE_EXECUTABLES ${PACKAGE_NAME};${PROJECT_NAME})
    SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CMAKE_PROJECT_NAME}-${OPENDCP_VERSION}")
    SET(CPACK_MONOLITHIC_INSTALL 1)
    SET(CPACK_NSIS_MUI_ICON  ${PROJECT_SOURCE_DIR}/gui/resources/opendcp.ico)
    SET(DOCUMENTATION_PATH doc)
ENDIF()
#-----------------------------------------------------------------------------

#--OSX package-------------------------------------------------------------
IF(APPLE)
    STRING(TOLOWER "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-mac.${TARGET_ARCH}" CPACK_PACKAGE_FILE_NAME)
    SET(CPACK_PACKAGE_ICON ${PROJECT_SOURCE_DIR}/gui/resources/opendcp.icns) 
    SET(CPACK_GENERATOR "DragNDrop")
    SET(CPACK_BINARY_DRAGNDROP ON)
    SET(DOCUMENTATION_PATH doc)
ENDIF()
#-----------------------------------------------------------------------------

#--debian package-------------------------------------------------------------
IF(DEB)
    SET(CPACK_GENERATOR "DEB")
    IF(TARGET_ARCH STREQUAL "x86_64")
        SET(TARGET_ARCH amd64)
    ELSE()
        SET(TARGET_ARCH i386)
    ENDIF()
    STRING(TOLOWER "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}.${TARGET_ARCH}" CPACK_PACKAGE_FILE_NAME)
    SET(CPACK_DEBIAN_PACKAGE_SECTION "Video")
    SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Terrence Meiczinger <terrence72@gmail.com>")
    SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "This tool is used to created digital cinema packages (DCP)")
    SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${TARGET_ARCH})
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS "openssl (>= 0.9.8),libxml2 (>= 2.7),libxslt1.1 (>= 1.1.26),xmlsec1 (>= 1.2),libxmlsec1-openssl (>= 1.2),libtiff4 (>= 3.9),libqtcore4 (>= 4.6),libqtgui4 (>= 4.6)")
    SET(DOCUMENTATION_PATH share/doc/${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION})
ENDIF()
#-----------------------------------------------------------------------------

#--rpm package----------------------------------------------------------------
IF(RPM)
    SET(CPACK_GENERATOR "RPM")
    STRING(TOLOWER "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}.${TARGET_ARCH}" CPACK_PACKAGE_FILE_NAME)
    SET(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
    SET(CPACK_RPM_PACKAGE_DESCRIPTION "This tool is used to created digital cinema packages (DCP)")
    SET(CPACK_RPM_PACKAGE_ARCHITECTURE ${TARGET_ARCH})
    SET(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
    SET(DOCUMENTATION_PATH share/doc/${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION})
ENDIF()
#-----------------------------------------------------------------------------

#--documentation--------------------------------------------------------------
INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/licenses        DESTINATION ${DOCUMENTATION_PATH})
INSTALL(FILES     ${PROJECT_SOURCE_DIR}/COPYRIGHT.txt   DESTINATION ${DOCUMENTATION_PATH})
INSTALL(FILES     ${PROJECT_SOURCE_DIR}/README.txt      DESTINATION ${DOCUMENTATION_PATH})
#-----------------------------------------------------------------------------

#--package--------------------------------------------------------------------
INCLUDE(CPack)
#-----------------------------------------------------------------------------
