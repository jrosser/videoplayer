######################################################################
# Automatically generated by qmake (2.01a) Tue Mar 6 17:22:20 2007
######################################################################

TEMPLATE = app
TARGET = 
INCLUDEPATH += . /usr/include  ../include /usr/include/freetype2

LIBS += -lGL -L/project/compression/jrosser/workspace/lib -lftgl

QT += opengl
CONFIG += thread debug

# Input
HEADERS = mainwindow.h GLvideo.h GLvideo_mt.h GLvideo_rt.h videoRead.h readThread.h videoData.h QShuttlePro.h
SOURCES = main.cpp mainwindow.cpp GLvideo.cpp GLvideo_mt.cpp GLvideo_rt.cpp videoRead.cpp readThread.cpp videoData.cpp QShuttlePro.cpp
