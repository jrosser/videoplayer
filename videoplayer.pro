TEMPLATE = app

QT += opengl
CONFIG += thread console debug_and_release

# source files
HEADERS = mainwindow.h videoData.h readerInterface.h yuvReader.h frameQueue.h videoTransport.h config.h
HEADERS += stats.h version.h program_options_lite.h fileDialog.h

SOURCES = main.cpp mainwindow.cpp videoData.cpp yuvReader.cpp frameQueue.cpp videoTransport.cpp
SOURCES += stats.cpp version.c program_options_lite.cpp fileDialog.cpp

# openGL video widget source files
HEADERS += GLvideo_params.h GLvideo_mt.h GLvideo_rt.h GLvideo_rtAdaptor.h GLvideo_renderer.h shaders.h
SOURCES += GLvideo_mt.cpp GLvideo_rt.cpp GLvideo_rtAdaptor.cpp

# GL utility functions
HEADERS += GLutil.h colourMatrix.h
SOURCES += GLutil.cpp colourMatrix.cpp

# video texture transfer engines
HEADERS += GLvideo_tradtex.h GLvideo_pbotex.h
SOURCES += GLvideo_tradtex.cpp GLvideo_pbotex.cpp

# enable or disable the optional features here
DEFINES += WITH_OSD

contains(DEFINES, WITH_OSD) {
  SOURCES += GLvideo_osd.cpp
  HEADER += GLvideo_osd.h
}

unix {
  SOURCES += QConsoleInput.cpp
  HEADERS += QConsoleInput.h

  # mmap gives a performance increase when reading
  HEADERS += yuvReaderMmap.h
  SOURCES += yuvReaderMmap.cpp
}

linux-g++ {
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

		contains(DEFINES, WITH_OSD) {
			#----------------------------------------------------
			# freetype
			FT_LIB = $$WINLIBS\freetype-2.3.5\objs\freetype235
			CONFIG(debug, debug|release):FT_LIB = $$join(FT_LIB,,, MT_D.lib)
			else:FT_LIB = $$join(FT_LIB,,, MT.lib)
			LIBS += $$FT_LIB
			INCLUDEPATH += $$WINLIBS\freetype-2.3.5\include

			#----------------------------------------------------
			# ftgl
			FTGL_LIB += $$WINLIBS\FTGL\win32_vcpp\build\ftgl_static
			CONFIG(debug, debug|release):FTGL_LIB = $$join(FTGL_LIB,,, _d.lib)
			else:FTGL_LIB = $$join(FTGL_LIB,,, .lib)
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
}

VersionGen.name = Generate version.c based on git describe
VersionGen.CONFIG += target_predeps
VersionGen.target = version.c
VersionGen.depends = FORCE
VersionGen.commands = \( export GIT_DIR=$${_PRO_FILE_PWD_}/.git ; \
                         export GIT_WORK_TREE=$${_PRO_FILE_PWD_}/ ; \
                         echo \'/* This file is automatically generated */\' ; \
                         echo -n \'const char* BUILD_VERSION = \' ; \
                         echo -n \\\"\$\$( git describe --always --tags HEAD )\\\" ; \
                         git diff --exit-code --quiet HEAD || echo -n \\\"+uncommitted changes\\\" ; \
                         echo \\; \
                      \) > version.c
QMAKE_EXTRA_TARGETS += VersionGen
