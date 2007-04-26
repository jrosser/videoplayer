######################################################################
# Automatically generated by qmake (2.01a) Tue Mar 6 17:22:20 2007
######################################################################

TEMPLATE = app
TARGET = 
INCLUDEPATH += . /usr/include  /usr/include/freetype2

QT += opengl
CONFIG += thread debug console

# Input
HEADERS = mainwindow.h GLvideo_mt.h GLvideo_rt.h videoRead.h readThread.h videoData.h
SOURCES = main.cpp mainwindow.cpp GLvideo_mt.cpp GLvideo_rt.cpp videoRead.cpp readThread.cpp videoData.cpp

unix {
  SOURCES += QShuttlePro.cpp
  HEADERS += QShuttlePro.h
  LIBS += -lftgl
  DEFINES += -DHAVE_FTGL
}