ACLOCAL_AMFLAGS=-I m4

CPPFLAGS += -DWITH_CONFIG_H

#
# common: conditionally generate version.c
#

if NONDISTVERSION
VERSION_GEN = $(top_srcdir)/scripts/version-gen.sh
VERSION_STRING_ := $(shell $(VERSION_GEN))
force: ;
src/version.c: src/version.c.in force
	$(AM_V_GEN) sed -e 's!__VERSION_STRING__!'`$(VERSION_GEN) raw`'!' < $< > $@
else
VERSION_STRING_ = $(VERSION_STRING)
endif

EXTRA_DIST = \
	src/version.c \
	scripts/version-gen.sh \
	scripts/version.c.gen.sh \
	scripts/Makefile.dmg \
	scripts/Makefile.dist \
	qt.conf.in \
	redist/Vera.ttf \
	redist/COPYING.Vera \
	$(NULL)

VERSION = $(VERSION_STRING_)

#
# the video player application
#

bin_PROGRAMS = videoplayer

EXTRA_DIST += src/videoplayer.pro

videoplayer_CPPFLAGS = $(QTGUI_CFLAGS) $(QTGL_CFLAGS) $(GLEW_CFLAGS)
videoplayer_LDADD = libqttv-@QTTV_MAJORMINOR@.la

videoplayer_SOURCES = \
	src/fileDialog.cpp \
	src/fileDialog.h \
	src/mainwindow.cpp \
	src/mainwindow.h \
	src/frameQueue.cpp \
	src/frameQueue.h \
	src/main.cpp \
	src/program_options_lite.cpp \
	src/program_options_lite.h \
	src/videoTransport.cpp \
	src/videoTransport.h \
	src/videoTransportQT.h \
	src/readerInterface.h \
	src/playlistReader.cpp \
	src/playlistReader.h \
	src/yuvReader.cpp \
	src/yuvReader.h \
	src/yuvReaderMmap.h \
	src/util.cpp \
	src/util.h \
	src/version.h \
	$(NULL)

nodist_videoplayer_SOURCES = \
	src/moc_fileDialog.cpp \
	src/moc_mainwindow.cpp \
	src/moc_videoTransportQT.cpp \
	$(NULL)

if LINUX
videoplayer_SOURCES += \
	src/QConsoleInput.h \
	src/QConsoleInput.cpp \
	src/QShuttlePro.cpp \
	src/QShuttlePro.h
	$(NULL)

nodist_videoplayer_SOURCES += \
	src/moc_QShuttlePro.cpp \
	$(NULL)
endif

CLEANFILES = $(nodist_videoplayer_SOURCES) src/version.c

#
# the library
#

lib_LTLIBRARIES = libqttv-@QTTV_MAJORMINOR@.la

libqttv_@QTTV_MAJORMINOR@_la_LIBADD = $(QTGUI_LIBS) $(QTGL_LIBS) $(GLEW_LIBS)
libqttv_@QTTV_MAJORMINOR@_la_LDFLAGS =
libqttv_@QTTV_MAJORMINOR@_la_CPPFLAGS = $(QTGUI_CFLAGS) $(QTGL_CFLAGS) $(GLEW_CFLAGS)

libqttv_@QTTV_MAJORMINOR@_la_SOURCES = \
	src/GLutil.cpp \
	src/GLvideo_rt.cpp \
	src/GLvideo_mt.cpp \
	src/GLvideo_rtAdaptor.cpp \
	src/GLfrontend_old.cpp \
	src/GLfrontend_old.h \
	src/colourMatrix.cpp \
	src/stats.cpp \
	src/fourcc.h \
	src/videoData.cpp \
	src/version.c \
	src/freetype-gl/freetype-gl.h \
	src/freetype-gl/texture-atlas.cpp \
	src/freetype-gl/texture-atlas.h \
	src/freetype-gl/texture-font.cpp \
	src/freetype-gl/texture-font.h \
	src/freetype-gl/utf8.cpp \
	src/freetype-gl/utf8.h \
	src/freetype-gl/vec234.h \
	src/freetype-gl/vector.cpp \
	src/freetype-gl/vector.h \
	src/freetype-gl/vertex-buffer.cpp \
	src/freetype-gl/vertex-buffer.h \
	$(NULL)


if OSX
libqttv_@QTTV_MAJORMINOR@_la_SOURCES += \
	src/agl_getproc.cpp \
	src/agl_getproc.h \
	$(NULL)
endif

if WITH_OSD
libqttv_@QTTV_MAJORMINOR@_la_SOURCES += src/GLvideo_osd.cpp
libqttv_@QTTV_MAJORMINOR@_la_LIBADD += $(FREETYPE_LIBS)
libqttv_@QTTV_MAJORMINOR@_la_CPPFLAGS += $(FREETYPE_CFLAGS)
endif

nodist_libqttv_@QTTV_MAJORMINOR@_la_SOURCES = \
	src/moc_GLvideo_mt.cpp \
	$(NULL)

pkginclude_HEADERS = \
	src/GLutil.h \
	src/GLvideo_mt.h \
	src/GLvideo_osd.h \
	src/GLvideo_params.h \
	src/GLvideo_rt.h \
	src/GLvideo_rtAdaptor.h \
	src/colourMatrix.h \
	src/shaders.h \
	src/stats.h \
	src/videoData.h \
	$(NULL)

noinst_HEADERS =

CLEANFILES += $(nodist_libqttv_@QTTV_MAJORMINOR@_la_SOURCES)

moc_%.cpp: %.h
	$(AM_V_GEN)$(QT4_MOC) $< -o $@
