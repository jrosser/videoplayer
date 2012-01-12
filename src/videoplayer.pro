TEMPLATE = app

QT += opengl
CONFIG += thread console debug_and_release
unix { CONFIG += link_pkgconfig }

# enable or disable the optional features here
DEFINES += WITH_OSD

include(local.pro)

# source files
HEADERS = mainwindow.h videoData.h readerInterface.h yuvReader.h frameQueue.h videoTransport.h videoTransportQT.h
HEADERS += stats.h util.h version.h program_options_lite.h fileDialog.h playlistreader.h

SOURCES = main.cpp mainwindow.cpp videoData.cpp yuvReader.cpp frameQueue.cpp videoTransport.cpp
SOURCES += stats.cpp util.cpp version.c program_options_lite.cpp fileDialog.cpp playlistreader.cpp

# openGL video widget source files
HEADERS += GLvideo_params.h GLvideo_mt.h GLvideo_rt.h GLvideo_rtAdaptor.h shaders.h
SOURCES += GLvideo_mt.cpp GLvideo_rt.cpp GLvideo_rtAdaptor.cpp

# GL utility functions
HEADERS += GLutil.h colourMatrix.h fourcc.h
SOURCES += GLutil.cpp colourMatrix.cpp

# openGL frontends (for video presentation)
HEADERS += GLfrontend_old.h
SOURCES += GLfrontend_old.cpp


contains(DEFINES, WITH_OSD) {
  SOURCES += GLvideo_osd.cpp
  HEADERS += GLvideo_osd.h

  SOURCES += \
    freetype-gl/texture-atlas.cpp \
    freetype-gl/texture-font.cpp \
    freetype-gl/utf8.cpp \
    freetype-gl/vector.cpp \
    freetype-gl/vertex-buffer.cpp

  HEADERS += \
    freetype-gl/freetype-gl.h \
    freetype-gl/texture-atlas.h \
    freetype-gl/texture-font.h \
    freetype-gl/utf8.h \
    freetype-gl/vec234.h \
    freetype-gl/vector.h \
    freetype-gl/vertex-buffer.h
}

unix {
  SOURCES += QConsoleInput.cpp
  HEADERS += QConsoleInput.h

  # mmap gives a performance increase when reading
#  HEADERS += yuvReaderMmap.h
#  SOURCES += yuvReaderMmap.cpp
}

linux-g++ {
  SOURCES += QShuttlePro.cpp
  HEADERS += QShuttlePro.h
}

macx {
  #helper functions for OS X openGL
  SOURCES += agl_getproc.cpp
  HEADERS += agl_getproc.h
}

link_pkgconfig {
  # override in local.pro by specifying CONFIG-=link_pkgconfig,
  # and then adding manual paths
  PKGCONFIG += glew
  contains(DEFINES, WITH_OSD) {
    PKGCONFIG += freetype2
  }
}

macx {
  #see http://developer.apple.com/qa/qa2007/qa1567.html
  QMAKE_LFLAGS += -dylib_file \
    /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:\
    /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib
}

win32 {
	contains(QMAKE_CXX, cl) {
		#win32 builds using the msvc toolchain
		message("configuring for win32 msvc build")

		QMAKE_LFLAGS += /VERBOSE:LIB
		DEFINES += _CRT_SECURE_NO_WARNINGS

		# default library names
		GLEW_LIB = $$GLEWDIR/lib/glew32s.lib
		FREETYPE_LIB = $$FREETYPEDIR/lib/freetype.lib
	} else {
		#win32 builds using the mingw toolchain
		message("configuring for win32 mingw build")

		# default library names
		GLEW_LIB = $$GLEWDIR/lib/glew32s.lib
		FREETYPE_LIB = $$FREETYPEDIR/lib/freetype.lib
	}

	#
	# glew
	#
	DEFINES += GLEW_STATIC
	exists($$GLEW_LIB): LIBS += $$GLEW_LIB
	else: message("$$GLEW_LIB could not be found.  Carrying on regardless")
	exists($$GLEWDIR/include): INCLUDEPATH += $$GLEWDIR/include
	else: message("$$GLEWDIR/include could not be found.  Carrying on regardless")

	contains(DEFINES, WITH_OSD) {
		#
		# freetype
		#
		exists($$FREETYPE_LIB): LIBS += $$FREETYPE_LIB
		else: message("$$FREETYPE_LIB could not be found.  Carrying on regardless")
		exists($$FREETYPEDIR/include): INCLUDEPATH += $$FREETYPEDIR/include $$FREETYPEDIR/include/freetype2
		else: message("$$FREETYPEDIR/include could not be found.  Carrying on regardless")
	}

	LIBS += -lopengl32 -lglu32
}

app_bundle {
  EXTRADIST.files = $${_PRO_FILE_PWD_}/../redist/Vera.ttf $${_PRO_FILE_PWD_}/../redist/COPYING.Vera
  EXTRADIST.path = Contents/
  QMAKE_BUNDLE_DATA += EXTRADIST
}

VersionGen.name = Generate version.c based on git describe
VersionGen.CONFIG += target_predeps
VersionGen.target = version.c
VersionGen.depends = FORCE
VersionGen.commands = sh $${_PRO_FILE_PWD_}/../scripts/version.c.gen.sh < $${_PRO_FILE_PWD_}/version.c.in > version.c
exists($${_PRO_FILE_PWD_}/../scripts/version-gen.sh) { QMAKE_EXTRA_TARGETS += VersionGen }
