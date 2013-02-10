MESSAGE(STATUS)
MESSAGE(STATUS "-------------------------------------------------------------------------------")
MESSAGE(STATUS "Creating Upload Package Target")
MESSAGE(STATUS "-------------------------------------------------------------------------------")


#--arch string----------------------------------------------------------------
IF(TARGET_ARCH STREQUAL "x86_64")
   SET(ARCH_STRING "64-bit")
ELSE()
   SET(ARCH_STRING "32-bit")
ENDIF()

#--package name---------------------------------------------------------------
IF(WIN32)
    STRING(TOLOWER "${PACKAGE_NAME}-${OPENDCP_VERSION}-${CMAKE_SYSTEM_NAME}.${TARGET_ARCH}.exe" PACKAGE_FILE_NAME)
    SET(SYSTEM_STRING "Windows ${ARCH_STRING}")
    SET(LABELS "Type-Installer, Featured, OpSys-Windows")
ENDIF()

IF(APPLE)
    STRING(TOLOWER "${PACKAGE_NAME}-${OPENDCP_VERSION}-mac.${TARGET_ARCH}.dmg" PACKAGE_FILE_NAME)
    SET(SYSTEM_STRING "Macintosh OSX ${ARCH_STRING}")
    SET(LABELS "Type-Installer, Featured, OpSys-OSX")
ENDIF()

IF(DEB)
    IF(TARGET_ARCH STREQUAL "x86_64")
        SET(TARGET_ARCH amd64)
    ELSE()
        SET(TARGET_ARCH i386)
    ENDIF()
    STRING(TOLOWER "${PACKAGE_NAME}-${OPENDCP_VERSION}-${CMAKE_SYSTEM_NAME}.${TARGET_ARCH}.deb" PACKAGE_FILE_NAME)
    SET(SYSTEM_STRING "Linux DEB ${ARCH_STRING}")
    SET(LABELS "Type-Package, Featured, OpSys-Linux")
ENDIF()

IF(RPM)
    STRING(TOLOWER "${PACKAGE_NAME}-${OPENDCP_VERSION}-${CMAKE_SYSTEM_NAME}.${TARGET_ARCH}.rpm" PACKAGE_FILE_NAME)
    SET(SYSTEM_STRING "Linux RPM ${ARCH_STRING}")
    SET(LABELS "Type-Package, Featured, OpSys-Linux")
ENDIF()
#-----------------------------------------------------------------------------

#--google parameters----------------------------------------------------------
SET(UPLOADER ${PROJECT_SOURCE_DIR}/scripts/googlecode_upload.py)
SET(GOOGLE_PROJECT ${PROJECT_NAME})
SET(USER terrence72@gmail.com)
SET(SUMMARY "OpenDCP ${OPENDCP_VERSION} ${SYSTEM_STRING}")
SET(UPLOAD_FILE ${PROJECT_BINARY_DIR}/${PACKAGE_FILE_NAME})
SET(CONFIG_FILE "$ENV{HOME}/.opendcp/google.txt")
IF(EXISTS ${CONFIG_FILE})
    FILE(READ ${CONFIG_FILE} PASSWORD)
    STRING(REGEX REPLACE "\n" "" PASSWORD "${PASSWORD}")
    SET(PASSWORD "--password=${PASSWORD}")    
ENDIF()
#-----------------------------------------------------------------------------

#--upload target--------------------------------------------------------------
MESSAGE(STATUS "${UPLOADER} -s \"${SUMMARY}\" -p \"${GOOGLE_PROJECT}\" -u \"${USER}\" ${PASSWORD} -l \"${LABELS}\" ${UPLOAD_FILE}")
ADD_CUSTOM_TARGET(upload
  COMMAND ${UPLOADER} -s "${SUMMARY}" -p "${GOOGLE_PROJECT}" -u "${USER}" ${PASSWORD} -l "${LABELS}" ${UPLOAD_FILE}
)
#-----------------------------------------------------------------------------
