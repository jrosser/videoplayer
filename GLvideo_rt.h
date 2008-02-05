/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: GLvideo_rt.h,v 1.31 2008-02-05 00:12:06 asuraparaju Exp $
*
* The MIT License
*
* Copyright (c) 2008 BBC Research
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
* ***** END LICENSE BLOCK ***** */

#ifndef GLVIDEO_RT_H_
#define GLVIDEO_RT_H_

#define GL_GLEXT_LEGACY
#include <GL/gl.h>
#include "glext.h"

#include <QtGui>
#include <QGLWidget>

#ifdef Q_OS_UNIX
#include "GL/glx.h"
#endif

class GLvideo_mt;
class FTFont;
class VideoData;

class GLvideo_rt : public QThread
{
public:

    enum ShaderPrograms { shaderPlanar, shaderUYVY, shaderMax };

    GLvideo_rt(GLvideo_mt &glWidget);
    void resizeViewport(int w, int h);
    void setFrameRepeats(int repeats);
    void setFramePolarity(int polarity);
    void setFontFile(const char *fontFile);
    void setAspectLock(bool lock);
    void setInterlacedSource(bool i);
    void run();
    void stop();
    void toggleAspectLock();
    void toggleOSD();
    void togglePerf();
    void toggleLuminance();
    void toggleChrominance();
    void toggleDeinterlace();
    void toggleMatrixScaling();
    void setLuminanceOffset1(float o);
    void setChrominanceOffset1(float o);
    void setLuminanceMultiplier(float m);
    void setChrominanceMultiplier(float m);
    void setLuminanceOffset2(float o);
    void setChrominanceOffset2(float o);
    void setDeinterlace(bool d);
    void setMatrixScaling(bool s);
    void setMatrix(float Kr, float Kg, float Kb);
    void setCaption(const char *caption);
    void setOsdScale(float s);
    void setOsdState(int s);
    void setOsdTextTransparency(float t);
    void setOsdBackTransparency(float t);

private:

    //openGL function pointers

#ifdef Q_OS_UNIX
    //unix
    PFNGLXWAITVIDEOSYNCSGIPROC glXWaitVideoSyncSGI;
    PFNGLXGETVIDEOSYNCSGIPROC glXGetVideoSyncSGI;
#endif

#ifdef Q_OS_WIN32
    //windows - header file? what header file!!!
    typedef bool (APIENTRY *PFNWGLSWAPINTERVALFARPROC) (int);
    PFNWGLSWAPINTERVALFARPROC wglSwapInterval;
#endif

#ifdef Q_OS_MACX
    typedef bool (APIENTRY *PFNAGLSWAPINTERVALFARPROC) (int);
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

    void getGLfunctions();
    void buildColourMatrix(float *matrix, const float Kr, const float Kg, const float Kb, bool Yscale, bool Cscale);
    void compileFragmentShaders();
    void compileFragmentShader(int n, const char *src);
    void createTextures(VideoData *videoData, int currentShader);
    void uploadTextures(VideoData *videoData);
    void renderVideo(VideoData *videoData);
#ifdef HAVE_FTGL
    void renderOSD(VideoData *videoData, FTFont *font, float fps, int osd, float osdScale);
    void renderPerf(VideoData *videoData, FTFont *font);
#endif

    float m_osdScale;            //caption / OSD font size
    float m_osdBackTransparency;
    float m_osdTextTransparency;

    bool m_doRendering;            //set to false to quit thread
    bool m_doResize;            //resize the openGL viewport
    bool m_aspectLock;            //lock the aspect ratio of the source video
    bool m_changeFont;            //set a new font for the OSD
    int m_osd;                    //type of OSD shown
    bool m_perf;                //show performance data
    bool m_showLuminance;        //use Y data or 0.5
    bool m_showChrominance;        //use C data or 0.5
    bool m_interlacedSource;    //is the source video interlaced?
    bool m_deinterlace;            //do we try to deinterlace an interlaced source?
    bool m_matrixScaling;        //is the colour matrix scaled to produce 0-255 computer levels or 16-235 video levels
    bool m_changeMatrix;

    float m_luminanceOffset1;        //Y & C offsets applied to data passed to the shader directly from the file
    float m_chrominanceOffset1;
    float m_luminanceMultiplier;    //Y & C scaling factors applied to offset Y/C data
    float m_chrominanceMultiplier;
    float m_luminanceOffset2;        //Y & C offsets applied to scaled data
    float m_chrominanceOffset2;
    float m_matrixKr;                //colour matrix specification
    float m_matrixKg;
    float m_matrixKb;

    static const int MAX_OSD = 5;

    int m_displaywidth;            //width of parent widget
    int m_displayheight;        //height of parent widget

    int m_frameRepeats;            //number of times each frame is repeated
    int m_framePolarity;        //which frame in m_frameRepeats to update on

    GLuint programs[shaderMax];    //handles for the shaders and programs
    GLuint shaders[shaderMax];
    GLint compiled[shaderMax];
    GLint linked[shaderMax];    //flags for success
    GLuint io_buf[3];            //io buffer objects
    GLvideo_mt &glw;            //parent widget

    char fontFile[255];            //truetype font used for on screen display
    char caption[255];

    QMutex mutex;
};

#endif /*GLVIDEO_RT_H_*/
