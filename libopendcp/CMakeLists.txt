cmake_minimum_required(VERSION 2.8)

MESSAGE(STATUS)
MESSAGE(STATUS "-------------------------------------------------------------------------------")
MESSAGE(STATUS "Creating Library Build Files")
MESSAGE(STATUS "-------------------------------------------------------------------------------")

#--configure files--------------------------------------------------------------
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/opendcp.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/opendcp.h)
#-------------------------------------------------------------------------------

#--set opendcplib source files--------------------------------------------------
SET(OPENDCP_LIB_SRC
     opendcp_j2k.c
     opendcp_xml.c
     opendcp_common.c
     opendcp_error.c
     opendcp_log.c
     asdcp_intf.cpp
     opendcp_image.c
)

SET(OPENDCP_CODEC_SRC
     codecs/opendcp_decoder.c
     codecs/opendcp_decoder_tif.c
     codecs/opendcp_decoder_bmp.c
     codecs/opendcp_decoder_dpx.c
     codecs/opendcp_decoder_exr.c
     codecs/opendcp_decoder_openjpeg.c
     codecs/opendcp_encoder.c
     codecs/opendcp_encoder_kakadu.c
     codecs/opendcp_encoder_openjpeg.c
     codecs/opendcp_encoder_tif.c
     codecs/opendcp_encoder_ragnarok.c
     codecs/opendcp_encoder_remote.c
)

IF(ENABLE_XMLSEC)
    SET(OPENDCP_XMLSEC_SRC  opendcp_xml_sign.c)
ENDIF(ENABLE_XMLSEC)

SET(OPENDCP_SRC_FILES ${OPENDCP_LIB_SRC} ${OPENDCP_CODEC_SRC} ${OPENDCP_XMLSEC_SRC})
MESSAGE(STATUS, "--- ${OPENDCP_SRC_FILES} ---")
#-------------------------------------------------------------------------------

#--set output targets and paths-------------------------------------------------
SET(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/libopendcp/")
#-------------------------------------------------------------------------------

#--compile opendcplib-----------------------------------------------------------
SET(OPENDCP_LIB opendcp-lib PARENT_SCOPE)

IF(BUILD_SHARED)
    ADD_LIBRARY(opendcp-lib-shared SHARED ${OPENDCP_SRC_FILES})
    SET_TARGET_PROPERTIES(opendcp-lib-shared PROPERTIES OUTPUT_NAME "opendcp")
    SET_TARGET_PROPERTIES(opendcp-lib-shared PROPERTIES PREFIX "lib")
    TARGET_LINK_LIBRARIES(opendcp-lib-shared ${ASDCP_SHARED_LIBRARIES} ${LIBS} ${LIB_RAGNAROK})
    IF(INSTALL_LIB)
        INSTALL(TARGETS opendcp-lib-shared DESTINATION ${LIB_INSTALL_PATH})
    ENDIF()
ENDIF()
IF(BUILD_STATIC)
    ADD_LIBRARY(opendcp-lib STATIC ${OPENDCP_SRC_FILES} $<TARGET_OBJECTS:${ASDCP_LIBRARIES}> $<TARGET_OBJECTS:${LIB_CRYPTO}>)
    SET_TARGET_PROPERTIES(opendcp-lib PROPERTIES OUTPUT_NAME "opendcp")
    SET_TARGET_PROPERTIES(opendcp-lib PROPERTIES PREFIX "lib")
    TARGET_LINK_LIBRARIES(opendcp-lib ${LIBS} ${LIB_RAGNAROK})
    IF(INSTALL_LIB)
        INSTALL(TARGETS opendcp-lib DESTINATION ${LIB_INSTALL_PATH})
    ENDIF()
ENDIF()
#-------------------------------------------------------------------------------

#--install header---------------------------------------------------------------
IF(INSTALL_LIB)
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/opendcp.h DESTINATION include)
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/libopendcp/codecs/opendcp_encoders.h DESTINATION include)
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/libopendcp/codecs/opendcp_decoders.h DESTINATION include)
ENDIF()
#-------------------------------------------------------------------------------
