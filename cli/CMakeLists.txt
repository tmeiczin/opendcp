cmake_minimum_required(VERSION 2.8)

MESSAGE(STATUS)
MESSAGE(STATUS "-------------------------------------------------------------------------------")
MESSAGE(STATUS "Creating CLI Build Files")
MESSAGE(STATUS "-------------------------------------------------------------------------------")

#--set output targets and paths-----------------------------------------------
SET(OPENDCP_TARGETS opendcp_xml opendcp_j2k opendcp_mxf opendcp_largefile)
IF(ENABLE_XMLSEC)
    SET(OPENDCP_TARGETS ${OPENDCP_TARGETS} opendcp_xml_verify)
ENDIF(ENABLE_XMLSEC)
SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
#-----------------------------------------------------------------------------

#--compile cli tools----------------------------------------------------------
ADD_EXECUTABLE(opendcp_largefile opendcp_largefile.c)
TARGET_LINK_LIBRARIES(opendcp_largefile ${LIBS})

ADD_EXECUTABLE(opendcp_xml opendcp_xml_cmd.c)
TARGET_LINK_LIBRARIES(opendcp_xml ${OPENDCP_LIB} ${ASDCP_LIBRARIES} ${LIBS})

ADD_EXECUTABLE(opendcp_j2k opendcp_j2k_cmd.c opendcp_cli.c)
TARGET_LINK_LIBRARIES(opendcp_j2k ${OPENDCP_LIB} ${ASDCP_LIBRARIES} ${LIBS})

ADD_EXECUTABLE(opendcp_mxf opendcp_mxf_cmd.c opendcp_cli.c)
TARGET_LINK_LIBRARIES(opendcp_mxf ${OPENDCP_LIB} ${ASDCP_LIBRARIES} ${LIBS})

IF(ENABLE_XMLSEC)
    ADD_EXECUTABLE(opendcp_xml_verify opendcp_xml_verify_cmd.c)
    TARGET_LINK_LIBRARIES(opendcp_xml_verify ${OPENDCP_LIB} ${ASDCP_LIBRARIES} ${LIBS})
ENDIF(ENABLE_XMLSEC)
#-----------------------------------------------------------------------------

#--install cli tools----------------------------------------------------------
SET_TARGET_PROPERTIES(${OPENDCP_TARGETS} PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
SET_TARGET_PROPERTIES(${OPENDCP_TARGETS} PROPERTIES HAS_CXX TRUE)
INSTALL(TARGETS ${OPENDCP_TARGETS} 
  DESTINATION bin 
)
#-----------------------------------------------------------------------------
