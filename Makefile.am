ACLOCAL_AMFLAGS=-I m4

CPPFLAGS += -DWITH_CONFIG_H

#
# common: conditionally generate version.c
#

if NONDISTVERSION
VERSION_GEN = $(top_srcdir)/version-gen.sh
VERSION_STRING_ := $(shell $(VERSION_GEN))
force: ;
src/version.c: src/version.c.in force
	$(AM_V_GEN) sed -e 's!__VERSION_STRING__!'`$(VERSION_GEN) raw`'!' < $< > $@
else
VERSION_STRING_ = $(VERSION_STRING)
endif

EXTRA_DIST = src/version.c version-gen.sh

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
	src/yuvReader.cpp \
	src/yuvReader.h \
	src/version.h \
	$(NULL)

nodist_videoplayer_SOURCES = \
	src/moc_fileDialog.cpp \
	src/moc_mainwindow.cpp \
	src/moc_videoTransportQT.cpp \
	$(NULL)

if LINUX
videoplayer_SOURCES += \
	src/yuvReaderMmap.cpp \
	src/yuvReaderMmap.h \
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
	src/GLvideo_pbotex.cpp \
	src/GLvideo_tradtex.cpp \
	src/GLfrontend_old.cpp \
	src/colourMatrix.cpp \
	src/stats.cpp \
	src/videoData.cpp \
	src/version.c \
	$(NULL)

if WITH_OSD
libqttv_@QTTV_MAJORMINOR@_la_SOURCES += src/GLvideo_osd.cpp
libqttv_@QTTV_MAJORMINOR@_la_LIBADD += $(FTGL_LIBS)
libqttv_@QTTV_MAJORMINOR@_la_CPPFLAGS += $(FTGL_CFLAGS)
endif

nodist_libqttv_@QTTV_MAJORMINOR@_la_SOURCES = \
	src/moc_GLvideo_mt.cpp \
	$(NULL)

pkginclude_HEADERS = \
	src/GLutil.h \
	src/GLvideo_mt.h \
	src/GLvideo_osd.h \
	src/GLvideo_params.h \
	src/GLvideo_renderer.h \
	src/GLvideo_rt.h \
	src/GLvideo_rtAdaptor.h \
	src/GLvideo_tradtex.h \
	src/GLvideo_pbotex.h \
	src/colourMatrix.h \
	src/shaders.h \
	src/stats.h \
	src/videoData.h \
	$(NULL)

noinst_HEADERS =

CLEANFILES += $(nodist_libqttv_@QTTV_MAJORMINOR@_la_SOURCES)

moc_%.cpp: %.h
	$(AM_V_GEN)$(QT4_MOC) $< -o $@
