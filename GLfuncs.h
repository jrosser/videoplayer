#ifndef __GLFUNCS_H
#define __GLFUNCS_H

#define GL_GLEXT_LEGACY
#include <GL/gl.h>
#include "glext.h"

#include <QtGui>

#ifdef Q_WS_X11
#include <X11/X.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#endif

namespace GLfuncs {
#ifdef Q_WS_X11
    //unix
    extern PFNGLXWAITVIDEOSYNCSGIPROC glXWaitVideoSyncSGI;
    extern PFNGLXGETVIDEOSYNCSGIPROC glXGetVideoSyncSGI;
#endif

#ifdef Q_OS_WIN32
    //windows - header file? what header file!!!
    typedef bool (APIENTRY *PFNWGLSWAPINTERVALFARPROC) (int);
    extern PFNWGLSWAPINTERVALFARPROC wglSwapInterval;
#endif

#ifdef Q_OS_MACX
    typedef bool (APIENTRY *PFNAGLSWAPINTERVALFARPROC) (int);
    extern PFNAGLSWAPINTERVALFARPROC aglSwapInterval;
#endif

    extern PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
    extern PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
    extern PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
    extern PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
    extern PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
    extern PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
    extern PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
    extern PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
    extern PFNGLBUFFERDATAPROC glBufferData;
    extern PFNGLBINDBUFFERPROC glBindBuffer;
    extern PFNGLUNIFORM1IARBPROC glUniform1iARB;
    extern PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
    extern PFNGLUNMAPBUFFERPROC glUnmapBuffer;
    extern PFNGLMAPBUFFERPROC glMapBuffer;
    extern PFNGLUNIFORM3FVARBPROC glUniform3fvARB;
    extern PFNGLUNIFORMMATRIX3FVARBPROC glUniformMatrix3fvARB;
    extern PFNGLUNIFORM1FARBPROC glUniform1fARB;
    extern PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
    extern PFNGLGENBUFFERSPROC glGenBuffers;
    extern PFNGLACTIVETEXTUREPROC glActiveTexture;
};

#endif
