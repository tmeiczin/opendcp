SET(CMAKE_MACOSX_RPATH 1)

SET(OPENDCP_ENCODER_PLUGINS
#     kakadu
     openjpeg
     tif
     ragnarok
     remote
)

#--compile plugins--------------------------------------------------------------
FOREACH(plugin ${OPENDCP_ENCODER_PLUGINS})
    ADD_LIBRARY(${plugin} SHARED opendcp_encoder_${plugin}.c)
    SET_TARGET_PROPERTIES(${plugin} PROPERTIES PREFIX "")
    SET_TARGET_PROPERTIES(${plugin} PROPERTIES SUFFIX .plugin)
    SET_TARGET_PROPERTIES(${plugin} PROPERTIES OUTPUT_NAME encoder-${plugin})
    TARGET_LINK_LIBRARIES(${plugin}  ${OPENDCP_LIB} ${LIBS})
ENDFOREACH()
#-------------------------------------------------------------------------------

IF(APPLE)
    INSTALL(
        TARGETS ${OPENDCP_ENCODER_PLUGINS} DESTINATION OpenDCP.app/Contents/Resources/Plugins
    )
    #ADD_EXECUTABLE(${EXE_NAME} MACOSX_BUNDLE ${OPENDCP_GUI_CXX_SRC} ${OPENDCP_GUI_RES_CXX} ${OPENDCP_FORM_H} ${OPENDCP_MOC_H} ${OPENDCP_QM_FILES})
    SET(APPLICATION_DIR "\${CMAKE_INSTALL_PREFIX}/${EXE_NAME}.app")
    #INSTALL(
    #    CODE "
    #    set(BU_CHMOD_BUNDLE_ITEMS ON)
    #    include(BundleUtilities)
    #    fixup_bundle(\"${APPLICATION_DIR}\" \"\" \"${OSX_LIB_DIRS}\")
    #    "
    #)
ENDIF()
