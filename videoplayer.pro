TEMPLATE = app
LIBS += -lboost_program_options

QT += opengl
CONFIG += thread console release

# Input
HEADERS = mainwindow.h GLvideo_mt.h GLvideo_rt.h videoData.h readerInterface.h yuvReader.h frameQueue.h videoTransport.h
SOURCES = main.cpp mainwindow.cpp GLvideo_mt.cpp GLvideo_rt.cpp videoData.cpp yuvReader.cpp frameQueue.cpp videoTransport.cpp  

DEFINES += HAVE_DIRAC

contains(DEFINES, HAVE_DIRAC) {
	#andrea's wrapper library around the schro and dirac libraries
	PARSER_PATH = /project/compression/jrosser/workspace/dirac1.0-amd64
	INCLUDEPATH += $$PARSER_PATH/include/dirac1.0
	INCLUDEPATH += $$PARSER_PATH/src/
	LIBPATH += $$PARSER_PATH/lib
	LIBS += -ldirac1.0_parser -ldirac1.0_decoder
	DEFINES += DIRAC_COMPILER_IS_GNUC DIRAC_64BIT_SUPPORT
	HEADERS += diracReader.h
	SOURCES += diracReader.cpp

	#schroedinger library
	#SCHRO_INSTALL_PATH = /usr/local
	SCHRO_INSTALL_PATH = /project/compression/jrosser/workspace/schro-amd64	
	INCLUDEPATH += $$SCHRO_INSTALL_PATH/include/schroedinger-1.0
	LIBPATH += $$SCHRO_INSTALL_PATH/lib
	LIBS += -lschroedinger-1.0
}

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


