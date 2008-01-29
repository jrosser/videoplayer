TEMPLATE = app
LIBS += -lboost_program_options

QT += opengl
CONFIG += thread console release

# Input
HEADERS = mainwindow.h GLvideo_mt.h GLvideo_rt.h videoRead.h readThread.h videoData.h
SOURCES = main.cpp mainwindow.cpp GLvideo_mt.cpp GLvideo_rt.cpp videoRead.cpp readThread.cpp videoData.cpp

linux-g++ {
  SOURCES += QShuttlePro.cpp
  HEADERS += QShuttlePro.h
  LIBS += -lftgl
  DEFINES += HAVE_FTGL
  INCLUDEPATH += /usr/include/freetype2
}

macx {
  #helper functions for OS X openGL
  QMAKE_CXXFLAGS += -fpascal-strings
  SOURCES += agl_getproc.cpp
  HEADERS += agl_getproc.h

  #boost
  INCLUDEPATH += /usr/local/include/boost-1_34_1/

  #ftgl
  INCLUDEPATH += /opt/local/include
  LIBPATH += /opt/local/lib
  LIBS    += -lftgl
  DEFINES += HAVE_FTGL

  #freetype
  INCLUDEPATH += /usr/X11/include/freetype2
  LIBPATH += $(SDKROOT)/usr/X11R6/lib
  LIBS += -lfreetype

  #openGL
  INCLUDEPATH += /usr/X11/include

  #see http://developer.apple.com/qa/qa2007/qa1567.html
  QMAKE_LFLAGS += -dylib_file \
    /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:\ 
    /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib
}


