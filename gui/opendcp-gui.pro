#-------------------------------------------------
#
# Project created by QtCreator 2011-03-08T19:26:11
#
#-------------------------------------------------

QT       += core gui

TARGET =   OpenDCP
TEMPLATE = app

#RC_FILE = resources/opendcp.rc
ICON =  resources/opendcp.icns

SOURCES +=  main.cpp\
            mainwindow.cpp \
            j2k.cpp \
            mxf.cpp \
            xml.cpp \
            generatetitle.cpp \
            j2kconversion_dialog.cpp \
            conversion.cpp \
            settings.cpp \
            translator.cpp \
            mxf-writer.cpp \
            mxfconversion_dialog.cpp

HEADERS  += mainwindow.h \
            generatetitle.h \
            j2kconversion_dialog.h \
            mxf-writer.h \
            conversion.h \
            settings.h \
            translaor.h \
            mxfconversion_dialog.h

FORMS    += forms/mainwindow.ui \
            forms/generatetitle.ui \
            forms/conversion.ui \
            forms/settings.ui

QMAKE_LFLAGS = -D_FILE_OFFSET_BITS=64
QMAKE_CXXFLAGS = -D_FILE_OFFSET_BITS=64

WIN32_LIBS +=   /home/tmeiczin/Development/OpenDCP/build-win32/lib/libopendcp.a \
                /home/tmeiczin/Development/OpenDCP/build-win32/contrib/lib/libopenjpeg.a \
                /home/tmeiczin/Development/OpenDCP/build-win32/contrib/lib/libtiff.a \
                /home/tmeiczin/Development/OpenDCP/build-win32/contrib/lib/libasdcp.a \
                /home/tmeiczin/Development/OpenDCP/build-win32/contrib/lib/libkumu.a \
                /home/tmeiczin/Development/OpenDCP/build-win32/contrib/lib/libcrypto.a \
                /home/tmeiczin/Development/OpenDCP/build-win32/contrib/lib/libexpat.a \
                /home/tmeiczin/Development/OpenDCP/build-win32/contrib/lib/libssl.a \
                -lz

LIBS +=         ../build/libopendcp/libopendcp.a \
                ../build/libasdcp/libopendcp-asdcp.a \
                ../build/libasdcp/libopendcp-kumu.a \
                -L../build/contrib/lib/ \
                -lopenjpeg -ltiff -lxmlsec1 -lxmlsec1-openssl \
                -lxslt -lxml2 -lexpat -lcrypto -lssl \
                -lz -lgomp

INCLUDEPATH += ../libopendcp \
               ../../build/contrib/include \
               ../libasdcp

RESOURCES += \
    resources/opendcp.qrc

OTHER_FILES += \
    main.qml
