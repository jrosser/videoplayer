AC_PREREQ([2.67])
AC_INIT([videoplayer], [1.0], [davidf@rd.bbc.co.uk])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])
AM_SILENT_RULES([yes])
LT_INIT

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

# conditionally build depending upon platform
AC_CANONICAL_HOST
AS_CASE([$host],
	[*-*-linux*],[linux=true],
	[*-*-darwin*],[osx=true],
	[*-mingw*|*-winnt*],[win32=true])

AM_CONDITIONAL([LINUX],[test x$linux = xtrue])
AM_CONDITIONAL([OSX],[test x$osx = xtrue])
AM_CONDITIONAL([WIN32],[test x$win32 = xtrue])

# if version.c.in exists, version.c needs to be generated from the
# git commit id.
if test ! -f ${srcdir}/src/version.c.in
then
	distversion=true
	VERSION_STRING=`${srcdir}/version-gen.sh`
	AC_SUBST(VERSION_STRING)
fi
AM_CONDITIONAL([NONDISTVERSION], [test x$distversion = x])

QTTV_MAJORMINOR=1.0
AC_SUBST(QTTV_MAJORMINOR)

# Checks for libraries.
PKG_CHECK_MODULES(QTGUI, QtGui > 4.7)
PKG_CHECK_MODULES(QTGL, QtOpenGL > 4.7)
QT4_MOC=`$PKG_CONFIG --variable=moc_location QtGui`
AC_SUBST(QT4_MOC)

PKG_CHECK_MODULES(FTGL, ftgl)
PKG_CHECK_MODULES(GLEW, glew)

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([fcntl.h malloc.h stdint.h stdlib.h string.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday memset select])

# allow features to be enabled/disabled
AC_ARG_ENABLE([osd], AS_HELP_STRING([--disable-osd], [Disable on-screen display]), [], [enable_osd=yes])
AS_IF([test x$enable_osd = xyes], AC_DEFINE([WITH_OSD], [1]))
AH_TEMPLATE([WITH_OSD], [Define to enable execution of the on-screen display code])
AM_CONDITIONAL([WITH_OSD], [test x$enable_osd = xyes])

# choose which font to use
AH_TEMPLATE([DEFAULT_FONTFILE], [Platform specific path to a ttf font])
AS_CASE([$host],
	[*-*-linux*],[AC_DEFINE([DEFAULT_FONTFILE], ["/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans-Bold.ttf"])],
	[*-*-darwin*],[AC_DEFINE([DEFAULT_FONTFILE], ["/Library/Fonts/GillSans.dfont"])],
	[*-mingw*|*-winnt*],[AC_DEFINE([DEFAULT_FONTFILE], ["c:\\windows\\fonts\\arial.ttf"])])


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
