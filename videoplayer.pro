TEMPLATE = app

QT += opengl
CONFIG += thread console debug_and_release

# source files
HEADERS = mainwindow.h videoData.h readerInterface.h yuvReader.h frameQueue.h videoTransport.h config.h
HEADERS += stats.h

SOURCES = main.cpp mainwindow.cpp videoData.cpp yuvReader.cpp frameQueue.cpp videoTransport.cpp util.cpp
SOURCES += stats.cpp

# openGL video widget source files
HEADERS += GLvideo_params.h GLvideo_mt.h GLvideo_rt.h GLvideo_renderer.h GLvideo_repeater.h shaders.h
SOURCES += GLvideo_mt.cpp GLvideo_rt.cpp

# video texture transfer engines
HEADERS += GLvideo_tradtex.h GLvideo_pbotex.h
SOURCES += GLvideo_tradtex.cpp GLvideo_pbotex.cpp

# enable or disable the optional features here
#DEFINES += HAVE_DIRAC
DEFINES += WITH_OSD

contains(DEFINES, WITH_OSD) {
  SOURCES += GLvideo_osd.cpp
  HEADER += GLvideo_osd.h
}

unix {
  SOURCES += QConsoleInput.cpp
  HEADERS += QConsoleInput.h
}

contains(DEFINES, HAVE_DIRAC) {
	#andrea's wrapper library around the schro and dirac libraries
	PARSER_PATH = /project/compression/jrosser/workspace/dirac1.0/branches/dg_demo
	INCLUDEPATH += $$PARSER_PATH/include/dirac1.0
	INCLUDEPATH += $$PARSER_PATH/src/
	LIBPATH += $$PARSER_PATH/lib
	LIBS += -ldirac1.0_parser -ldirac1.0_decoder
	DEFINES += DIRAC_COMPILER_IS_GNUC DIRAC_64BIT_SUPPORT
	HEADERS += diracReader.h
	SOURCES += diracReader.cpp

	#schroedinger library
	SCHRO_INSTALL_PATH = /usr/local
	#SCHRO_INSTALL_PATH = /Users/andrea/build/schroedinger-Darwin-i386
	INCLUDEPATH += $$SCHRO_INSTALL_PATH/include/schroedinger-1.0
	LIBPATH += $$SCHRO_INSTALL_PATH/lib
	LIBS += -lschroedinger-1.0
}

linux-g++ {
  # video frame repeating engines
  HEADERS += GLvideo_x11rep.h
  SOURCES += GLvideo_x11rep.cpp

  SOURCES += QShuttlePro.cpp
  HEADERS += QShuttlePro.h

  # GLEW is not managed by pkgconfig
  LIBS += -lGLEW

  contains(DEFINES, WITH_OSD) {
    CONFIG += link_pkgconfig
    PKGCONFIG += ftgl
  }
}

macx {
  #helper functions for OS X openGL
  SOURCES += agl_getproc.cpp
  HEADERS += agl_getproc.h

  #boost
  INCLUDEPATH += /opt/local/include/boost-1_34_1/

  contains(DEFINES, WITH_OSD) {
    CONFIG += link_pkgconfig
    PKGCONFIG += ftgl
  }

  #glew
  LIBS += -lGLEW

  #see http://developer.apple.com/qa/qa2007/qa1567.html
  QMAKE_LFLAGS += -dylib_file \
    /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:\
    /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib
}

win32 {

	contains(QMAKE_CXX, cl) {
		#win32 builds using the msvc toolchain
		message("configuring for win32 msvc build")

		WINLIBS = c:\libs-msvc2008
		QMAKE_LFLAGS += /VERBOSE:LIB
		DEFINES += _CRT_SECURE_NO_WARNINGS

		#----------------------------------------------------
		# glew
		DEFINES += GLEW_STATIC
		LIBS += $$WINLIBS\glew\lib\glew32s.lib
		INCLUDEPATH += $$WINLIBS\glew\include

		#----------------------------------------------------
		# boost

		#for auto-linking
		LIBS += -L$$WINLIBS\boost_1_35_0\stage\lib
		INCLUDEPATH += $$WINLIBS\boost_1_35_0\

		contains(DEFINES, WITH_OSD) {
			#----------------------------------------------------
			# freetype
			FT_LIB = $$WINLIBS\freetype-2.3.5\objs\freetype235
			CONFIG(debug, debug|release) {
				FT_LIB = $$join(FT_LIB,,, _D.lib)
			} else {
				FT_LIB = $$join(FT_LIB,,, .lib)
			}
			LIBS += $$FT_LIB
			INCLUDEPATH += $$WINLIBS\freetype-2.3.5\include

			#----------------------------------------------------
			# ftgl
			FTGL_LIB += $$WINLIBS\FTGL\win32_vcpp\build\ftgl_static
			CONFIG(debug, debug|release) {
				FTGL_LIB = $$join(FTGL_LIB,,, _MT_d.lib)
			} else {
				FTGL_LIB = $$join(FTGL_LIB,,, _MT.lib)
			}
			LIBS += $$FTGL_LIB
			INCLUDEPATH += $$WINLIBS\ftgl
			DEFINES += FTGL_LIBRARY_STATIC
		}

	} else {
		#win32 builds using the mingw toolchain
		message("configuring for win32 mingw build")

		MINGWLIBS = c:\libs-mingw

		#----------------------------------------------------
		# glew
		DEFINES += GLEW_STATIC
		LIBS = $$MINGWLIBS\glew\lib\libglew32.a
		INCLUDEPATH += $$MINGWLIBS\glew\include

		#----------------------------------------------------
		# boost
		LIBS += $$MINGWLIBS\boost_1_35_0\stage\lib\libboost_program_options-mgw34-mt-s-1_35.lib
		INCLUDEPATH += $$MINGWLIBS\boost_1_35_0

		contains(DEFINES, WITH_OSD) {
			#----------------------------------------------------
			# ftgl
			LIBS += $$MINGWLIBS\FTGL\src\libftgl.a
			INCLUDEPATH += $$MINGWLIBS\FTGL
			DEFINES += HAVE_FTGL

			#----------------------------------------------------
			# freetype
			LIBS += $$MINGWLIBS\freetype-2.3.5\lib\libfreetype.a
			INCLUDEPATH += $$MINGWLIBS\freetype-2.3.5\include

			#doh! these need to be on the linker command line after the freetype and ftgl .a files
			LIBS += -lopengl32 -lglu32
		}
	}
} else {
	LIBS += -lboost_program_options
}
