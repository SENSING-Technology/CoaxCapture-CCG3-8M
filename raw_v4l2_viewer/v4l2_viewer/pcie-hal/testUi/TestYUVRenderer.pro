#-------------------------------------------------
#
# Project created by QtCreator 2018-08-03T11:51:33
#
#-------------------------------------------------

QT    += core gui widgets opengl

TARGET = TestYUVRenderer
TEMPLATE = app

CONFIG += c++11


SOURCES += main.cpp\
        TestWidget.cpp \
    OpenGLDisplay.cpp \

LIBS += ../build/libpcie_camera.so
LIBS += /usr/local/lib/libavcodec.so
LIBS += /usr/local/lib/libswscale.so

HEADERS  += TestWidget.h \
    OpenGLDisplay.h \
    ../include/PcieCamera.h \

FORMS    += TestWidget.ui

DESTDIR = $$PWD/bin
