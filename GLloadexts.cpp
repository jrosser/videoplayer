#include "GLfuncs.h"

#ifdef Q_WS_X11
#define XglGetProcAddress(str) glXGetProcAddress((GLubyte *)str)
#endif

#ifdef Q_OS_WIN32
#define XglGetProcAddress(str) wglGetProcAddress((LPCSTR)str)
#endif

#ifdef Q_OS_MACX
#include "agl_getproc.h"
#define XglGetProcAddress(str) aglGetProcAddress((char *)str)
#endif

#include <stdio.h>
#define DEBUG 0

namespace GLfuncs {
#ifdef Q_WS_X11
PFNGLXWAITVIDEOSYNCSGIPROC glXWaitVideoSyncSGI;
PFNGLXGETVIDEOSYNCSGIPROC glXGetVideoSyncSGI;
#endif

#ifdef Q_OS_WIN32
PFNWGLSWAPINTERVALFARPROC wglSwapInterval;
#endif

#ifdef Q_OS_MACX
PFNAGLSWAPINTERVALFARPROC aglSwapInterval;
#endif

PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLUNIFORM1IARBPROC glUniform1iARB;
PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
PFNGLUNMAPBUFFERPROC glUnmapBuffer;
PFNGLMAPBUFFERPROC glMapBuffer;
PFNGLUNIFORM3FVARBPROC glUniform3fvARB;
PFNGLUNIFORMMATRIX3FVARBPROC glUniformMatrix3fvARB;
PFNGLUNIFORM1FARBPROC glUniform1fARB;
PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLACTIVETEXTUREPROC glActiveTexture;

void loadGLExtSyms()
{
#ifdef Q_OS_WIN32
    //enable openGL vsync on windows
    wglSwapInterval = (PFNWGLSWAPINTERVALFARPROC) XglGetProcAddress("wglSwapIntervalEXT");
    if (wglSwapInterval)
        wglSwapInterval(1);

    if(DEBUG) printf("wglSwapInterval=%p\n",  wglSwapInterval);
#endif

#ifdef Q_WS_X11
    //functions for controlling vsync on X11
    glXGetVideoSyncSGI = (PFNGLXGETVIDEOSYNCSGIPROC) XglGetProcAddress("glXGetVideoSyncSGI");
    glXWaitVideoSyncSGI = (PFNGLXWAITVIDEOSYNCSGIPROC) XglGetProcAddress("glXWaitVideoSyncSGI");

    if(DEBUG) {
        printf("glXGetVideoSyncSGI=%p\n",  glXGetVideoSyncSGI);
        printf("glXWaitVideoSyncSGI=%p\n",  glXWaitVideoSyncSGI);
    }
#endif

#ifdef Q_OS_MACX
//    aglInitEntryPoints();
    my_aglSwapInterval(1);
#endif

    glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC) XglGetProcAddress("glLinkProgramARB");
    glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC) XglGetProcAddress("glAttachObjectARB");
    glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC) XglGetProcAddress("glCreateProgramObjectARB");
    glGetInfoLogARB = (PFNGLGETINFOLOGARBPROC) XglGetProcAddress("glGetInfoLogARB");
    glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC) XglGetProcAddress("glGetObjectParameterivARB");
    glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC) XglGetProcAddress("glCompileShaderARB");
    glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC) XglGetProcAddress("glShaderSourceARB");
    glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC) XglGetProcAddress("glCreateShaderObjectARB");
    glBufferData = (PFNGLBUFFERDATAPROC) XglGetProcAddress("glBufferData");
    glBindBuffer = (PFNGLBINDBUFFERPROC) XglGetProcAddress("glBindBuffer");
    glUniform1iARB = (PFNGLUNIFORM1IARBPROC) XglGetProcAddress("glUniform1iARB");
    glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC) XglGetProcAddress("glGetUniformLocationARB");

    glUnmapBuffer = (PFNGLUNMAPBUFFERPROC) XglGetProcAddress("glUnmapBuffer");
    glMapBuffer = (PFNGLMAPBUFFERPROC) XglGetProcAddress("glMapBuffer");
    glUniform3fvARB = (PFNGLUNIFORM3FVARBPROC) XglGetProcAddress("glUniform3fvARB");
    glUniformMatrix3fvARB = (PFNGLUNIFORMMATRIX3FVARBPROC) XglGetProcAddress("glUniformMatrix3fvARB");
    glUniform1fARB = (PFNGLUNIFORM1FARBPROC) XglGetProcAddress("glUniform1fARB");
    glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC) XglGetProcAddress("glUseProgramObjectARB");
    glGenBuffers = (PFNGLGENBUFFERSPROC) XglGetProcAddress("glGenBuffers");

    glActiveTexture = (PFNGLACTIVETEXTUREPROC) XglGetProcAddress("glActiveTexture");
}
}
