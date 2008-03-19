/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: GLvideo_rt.cpp,v 1.54 2008/03/13 11:38:49 jrosser Exp $
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

#include <stdio.h>

#include "GLfuncs.h"
#include "GLloadexts.h"

#include <QtGui>
#include "GLvideo_rt.h"
#include "GLvideo_mt.h"
#include "GLvideo_tradtex.h"
#include "GLvideo_pbotex.h"

#include "GLvideo_glxrep.h"

#ifdef HAVE_FTGL
#include "FTGL/FTGLPolygonFont.h"
#endif

#include "videoData.h"
#include "videoTransport.h"
#include "frameQueue.h"

#include "shaders.h"

//FIXME - nasty global variables
//rendering performance monitoring
int perf_getParams;
int perf_readData;
int perf_convertFormat;
int perf_updateVars;
int perf_repeatWait;
int perf_upload;
int perf_renderVideo;
int perf_renderOSD;
int perf_swapBuffers;
int perf_interval;

//I/O performance
int perf_futureQueue;
int perf_pastQueue;
int perf_IOLoad;
int perf_IOBandwidth;

#define DEBUG 0

GLvideo_rt::GLvideo_rt(GLvideo_mt &gl)
      : QThread(), glw(gl)
{
    m_frameRepeats = 0;
    m_doRendering = true;
    m_aspectLock = true;
    m_osd = true;
    m_perf = false;
    m_changeFont = false;
    m_showLuminance = true;
    m_showChrominance = true;
    m_interlacedSource = false;
    m_deinterlace = false;
    m_matrixScaling = false;
    m_changeMatrix = true;          //always compute colour matrix at least once
    m_displaywidth = 0;
    m_displayheight = 0; 

    m_luminanceOffset1 = -0.0625;   //video luminance has black at 16/255
    m_chrominanceOffset1 = -0.5;    //video chrominace is centered at 128/255

    m_luminanceMultiplier = 1.0;    //unscaled
    m_chrominanceMultiplier = 1.0;

    m_luminanceOffset2 = 0.0;       //no offset
    m_chrominanceOffset2 = 0.0;

    m_matrixKr = 0.2126;            //colour matrix for HDTV
    m_matrixKg = 0.7152;
    m_matrixKb = 0.0722;

    m_osdScale = 1.0;
    m_osdBackTransparency = 0.7;
    m_osdTextTransparency = 0.5;

    memset(caption, 0, sizeof(caption));
    strcpy(caption, "Hello World");

    //renderer = new GLVideoRenderer::TradTex();
    renderer = new GLVideoRenderer::PboTex();
}

void GLvideo_rt::buildColourMatrix(float *matrix, const float Kr, const float Kg, const float Kb, bool Yscale, bool Cscale)
{
    //R row
    matrix[0] = 1.0;             //Y    column
    matrix[1] = 0.0;             //Cb
    matrix[2] = 2.0 * (1-Kr);    //Cr

    //G row
    matrix[3] = 1.0;
    matrix[4] = -1.0 * ( (2.0*(1.0-Kb)*Kb)/Kg );
    matrix[5] = -1.0 * ( (2.0*(1.0-Kr)*Kr)/Kg );

    //B row
    matrix[6] = 1.0;
    matrix[7] = 2.0 * (1.0-Kb);
    matrix[8] = 0.0;

    float Ys = Yscale ? (255.0/219.0) : 1.0;
    float Cs = Cscale ? (255.0/224.0) : 1.0;

    //apply any necessary scaling
    matrix[0] *= Ys;
    matrix[1] *= Cs;
    matrix[2] *= Cs;
    matrix[3] *= Ys;
    matrix[4] *= Cs;
    matrix[5] *= Cs;
    matrix[6] *= Ys;
    matrix[7] *= Cs;
    matrix[8] *= Cs;

    if(DEBUG) {
        printf("Building YUV->RGB matrix with Kr=%f Kg=%f, Kb=%f\n", Kr, Kg, Kb);
        printf("%f %f %f\n", matrix[0], matrix[1], matrix[2]);
        printf("%f %f %f\n", matrix[3], matrix[4], matrix[5]);
        printf("%f %f %f\n", matrix[6], matrix[7], matrix[8]);
        printf("\n");
    }
}


void GLvideo_rt::compileFragmentShaders()
{
    compileFragmentShader((int)shaderPlanar, shaderPlanarSrc);
    compileFragmentShader((int)shaderUYVY,   shaderUYVYSrc);
}

void GLvideo_rt::compileFragmentShader(int n, const char *src)
{
    GLint length = 0;            //log length

    shaders[n] = GLfuncs::glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

    if(DEBUG) printf("Shader program is %s\n", src);

    GLfuncs::glShaderSourceARB(shaders[n], 1, (const GLcharARB**)&src, NULL);
    GLfuncs::glCompileShaderARB(shaders[n]);

    GLfuncs::glGetObjectParameterivARB(shaders[n], GL_OBJECT_COMPILE_STATUS_ARB, &compiled[n]);
    GLfuncs::glGetObjectParameterivARB(shaders[n], GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
    char *s=(char *)malloc(length);
    GLfuncs::glGetInfoLogARB(shaders[n], length, &length ,s);
    if(DEBUG)printf("Compile Status %d, Log: (%d) %s\n", compiled[n], length, length ? s : NULL);
    if(compiled[n] <= 0) {
        printf("Compile Failed: %s\n", gluErrorString(glGetError()));
        throw /* */;
    }
    free(s);

       /* Set up program objects. */
    programs[n]=GLfuncs::glCreateProgramObjectARB();
    GLfuncs::glAttachObjectARB(programs[n], shaders[n]);
    GLfuncs::glLinkProgramARB(programs[n]);

    /* And print the link log. */
    if(compiled[n]) {
        GLfuncs::glGetObjectParameterivARB(programs[n], GL_OBJECT_LINK_STATUS_ARB, &linked[n]);
        GLfuncs::glGetObjectParameterivARB(shaders[n], GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
        s=(char *)malloc(length);
        GLfuncs::glGetInfoLogARB(shaders[n], length, &length, s);
        if(DEBUG) printf("Link Status %d, Log: (%d) %s\n", linked[n], length, s);
        free(s);
    }
}

void GLvideo_rt::stop()
{
    mutex.lock();
    m_doRendering = false;
    mutex.unlock();
}

void GLvideo_rt::setAspectLock(bool l)
{
    mutex.lock();

    if(m_aspectLock != l) {
        m_aspectLock = l;
        m_doResize = true;
    }

    mutex.unlock();
}

void GLvideo_rt::setFrameRepeats(int repeats)
{
    mutex.lock();
    m_frameRepeats = repeats;
    mutex.unlock();
}

void GLvideo_rt::setFontFile(const char *f)
{
    mutex.lock();
    strncpy(fontFile, f, 255);
    m_changeFont = true;
    mutex.unlock();
}

void GLvideo_rt::setOsdScale(float s)
{
    mutex.lock();
    m_osdScale = s;
    mutex.unlock();
}

void GLvideo_rt::setOsdTextTransparency(float t)
{
    mutex.lock();
    m_osdTextTransparency = t;
    mutex.unlock();
}

void GLvideo_rt::setOsdBackTransparency(float t)
{
    mutex.lock();
    m_osdBackTransparency = t;
    mutex.unlock();
}



void GLvideo_rt::setCaption(const char *c)
{
    mutex.lock();
    strncpy(caption, c, 255);
    mutex.unlock();
}

void GLvideo_rt::toggleAspectLock(void)
{
    mutex.lock();
    m_aspectLock = !m_aspectLock;
    m_doResize = true;
    mutex.unlock();
}

void GLvideo_rt::toggleOSD(void)
{
    mutex.lock();
    m_osd = (m_osd + 1) % MAX_OSD;
    mutex.unlock();
}

void GLvideo_rt::setOsdState(int s)
{
    mutex.lock();
    m_osd = s % MAX_OSD;
    mutex.unlock();
}

void GLvideo_rt::togglePerf(void)
{
    mutex.lock();
    m_perf = !m_perf;
    mutex.unlock();
}


void GLvideo_rt::setInterlacedSource(bool i)
{
    mutex.lock();

    m_interlacedSource = i;
    if(m_interlacedSource == false) m_deinterlace=false;

    mutex.unlock();
}

void GLvideo_rt::setDeinterlace(bool d)
{
    mutex.lock();

    if(m_interlacedSource)
        m_deinterlace = d;
    else
        m_deinterlace = false;

    mutex.unlock();
}

void GLvideo_rt::toggleDeinterlace(void)
{
    mutex.lock();

    if(m_interlacedSource)
        m_deinterlace = !m_deinterlace;
    else
        m_deinterlace = false;

    mutex.unlock();
}

void GLvideo_rt::toggleLuminance(void)
{
    mutex.lock();
    m_showLuminance = !m_showLuminance;
    mutex.unlock();
}

void GLvideo_rt::toggleChrominance(void)
{
    mutex.lock();
    m_showChrominance = !m_showChrominance;
    mutex.unlock();
}

void GLvideo_rt::setMatrixScaling(bool s)
{
    mutex.lock();
    m_matrixScaling = s;
    m_changeMatrix = true;
    mutex.unlock();
}

void GLvideo_rt::toggleMatrixScaling()
{
    mutex.lock();
    m_matrixScaling = !m_matrixScaling;
    m_changeMatrix = true;
    mutex.unlock();
}

void GLvideo_rt::setMatrix(float Kr, float Kg, float Kb)
{
    mutex.lock();
    m_matrixKr = Kr;
    m_matrixKg = Kg;
    m_matrixKb = Kb;
    m_changeMatrix = true;
    mutex.unlock();
}

void GLvideo_rt::setLuminanceMultiplier(float m)
{
    mutex.lock();
    m_luminanceMultiplier = m;
    mutex.unlock();
}

void GLvideo_rt::setChrominanceMultiplier(float m)
{
    mutex.lock();
    m_chrominanceMultiplier = m;
    mutex.unlock();
}

void GLvideo_rt::setLuminanceOffset1(float o)
{
    mutex.lock();
    m_luminanceOffset1 = o;
    mutex.unlock();
}

void GLvideo_rt::setChrominanceOffset1(float o)
{
    mutex.lock();
    m_chrominanceOffset1 = o;
    mutex.unlock();
}

void GLvideo_rt::setLuminanceOffset2(float o)
{
    mutex.lock();
    m_luminanceOffset2 = o;
    mutex.unlock();
}

void GLvideo_rt::setChrominanceOffset2(float o)
{
    mutex.lock();
    m_chrominanceOffset2 = o;
    mutex.unlock();
}

void GLvideo_rt::resizeViewport(int width, int height)
{
    mutex.lock();
    m_displaywidth = width;
    m_displayheight = height;
    m_doResize = true;
    mutex.unlock();
}

#ifdef HAVE_FTGL
void GLvideo_rt::renderOSD(VideoData *videoData, FTFont *font, float fps, int osd, float osdScale)
{
    //bounding box of text
    float cx1, cy1, cz1, cx2, cy2, cz2;

    //text string
    char str[255];
    switch (osd) {
    case 1:
        sprintf(str, "%06ld", videoData->frameNum);
         font->BBox("000000", cx1, cy1, cz1, cx2, cy2, cz2);
        break;

    case 2:
        sprintf(str, "%06ld, fps:%3.2f", videoData->frameNum, fps);
         font->BBox("000000, fps:000.00", cx1, cy1, cz1, cx2, cy2, cz2);
        break;

    case 3:
        sprintf(str, "%06ld, %s:%s", videoData->frameNum, m_interlacedSource ? "Int" : "Prog", m_deinterlace ? "On" : "Off");
         font->BBox("000000, Prog:Off", cx1, cy1, cz1, cx2, cy2, cz2);
        break;

    case 4:
        sprintf(str, "%s", caption);
         font->BBox(caption, cx1, cy1, cz1, cx2, cy2, cz2);
        break;


    default: str[0] = '\0';
    }

    //text box location, defaults to bottom left
    float tx=0.05 * videoData->Ywidth;
    float ty=0.05 * videoData->Yheight;

    if(osd==4) {
        //put the caption in the middle of the screen
        tx = videoData->Ywidth - ((cx2 - cx1) * osdScale);
        tx/=2;
    }

    //black box that text is rendered onto, larger than the text by 'border'
    float border = 10;
    float bx1, by1, bx2, by2;
    bx1 = cx1 - border;
    by1 = cy1 - border;
    bx2 = cx2 + border;
    by2 = cy2 + border;

    //box beind text
    glPushMatrix();
    glTranslated(tx, ty, 0);
    glScalef(osdScale, osdScale, 0);    //scale the on screen display

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0, 0.0, 0.0, m_osdBackTransparency);

    glBegin(GL_QUADS);
    glVertex2f(bx1, by1);
    glVertex2f(bx1, by2);
    glVertex2f(bx2, by2);
    glVertex2f(bx2, by1);
    glEnd();

    //text
    glColor4f(1.0, 1.0, 1.0, m_osdTextTransparency);
    glEnable(GL_POLYGON_SMOOTH);
    font->Render(str);
    glDisable(GL_POLYGON_SMOOTH);
    glPopMatrix();
}

void drawText(char *str, FTFont *font)
{
    glEnable(GL_POLYGON_SMOOTH);
    glPushMatrix();
    font->Render(str);
    glPopMatrix();
    glDisable(GL_POLYGON_SMOOTH);
}

void draw2Text(const char *str1, const char *str2, int h_spacing, FTFont *font)
{
    glEnable(GL_POLYGON_SMOOTH);
    glPushMatrix();

    glPushMatrix();
    font->Render(str1);
    glPopMatrix();

    glTranslated(h_spacing, 0, 0);

    glPushMatrix();
    font->Render(str2);
    glPopMatrix();

    glPopMatrix();
    glDisable(GL_POLYGON_SMOOTH);
}

void drawPerfTimer(const char *str, int num, const char *units, int h_spacing, FTFont *font)
{
	char str2[255];
	sprintf(str2, "%d%s", num, units);
	draw2Text(str, str2, h_spacing, font);
}

void GLvideo_rt::renderPerf(VideoData *videoData, FTFont *font)
{
    float tx = 0.05 * videoData->Ywidth;
    float ty = 0.95 * videoData->Yheight;
    char str[255];
    float cx1, cy1, cz1, cx2, cy2, cz2;

    font->BBox("0", cx1, cy1, cz1, cx2, cy2, cz2);
    float spacing = (cy2-cy1) * 1.5;
    float width = cx2 - cx1;

    //black box that text is rendered onto, larger than the text by 'border'
    float border = 10;
    float bx1, by1, bx2, by2;
    bx1 = -border;
    by1 = spacing;
    bx2 = width * 25;
    by2 = (spacing * -15) - border*2;

    glPushMatrix();
    glTranslated(tx, ty, 0);    //near the top left corner
    glScalef(0.25, 0.25, 0);    //reduced in size compared to OSD

    //box beind text
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0, 0.0, 0.0, 0.7);

    glBegin(GL_QUADS);
    glVertex2f(bx1, by1);
    glVertex2f(bx1, by2);
    glVertex2f(bx2, by2);
    glVertex2f(bx2, by1);
    glEnd();

    glColor4f(0.0, 1.0, 0.0, 0.5);

    sprintf(str, "Rendering");
    drawText(str, font);

    glColor4f(1.0, 1.0, 1.0, 0.5);

    glTranslated(0, -spacing, 0);
    drawPerfTimer("ReadData", perf_readData ,"ms", width*15, font);

    glTranslated(0, -spacing, 0);
    drawPerfTimer("ConvertFormat", perf_convertFormat, "ms", width*15, font);

    glTranslated(0, -spacing, 0);
    drawPerfTimer("UpdateVars", perf_updateVars, "ms", width*15, font);

    glTranslated(0, -spacing, 0);
    drawPerfTimer("RepeatWait", perf_repeatWait, "ms", width*15, font);

    glTranslated(0, -spacing, 0);
    drawPerfTimer("Upload", perf_upload, "ms", width*15, font);

    glTranslated(0, -spacing, 0);
    drawPerfTimer("RenderVideo", perf_renderVideo, "ms", width*15, font);

    glTranslated(0, -spacing, 0);
    drawPerfTimer("RenderOSD", perf_renderOSD, "ms", width*15, font);

    glTranslated(0, -spacing, 0);
    drawPerfTimer("SwapBuffers", perf_swapBuffers, "ms", width*15, font);

    glTranslated(0, -spacing, 0);
    drawPerfTimer("Interval", perf_interval, "ms", width*15, font);

    glColor4f(0.0, 1.0, 0.0, 0.5);

    glTranslated(0, -spacing, 0);
    glTranslated(0, -spacing, 0);
    sprintf(str, "I/O");
    drawText(str, font);

    glColor4f(1.0, 1.0, 1.0, 0.5);

    glTranslated(0, -spacing, 0);
    drawPerfTimer("Future Queue", perf_futureQueue, "", width*15, font);

    glTranslated(0, -spacing, 0);
    drawPerfTimer("Past Queue", perf_pastQueue, "", width*15, font);

    glTranslated(0, -spacing, 0);
    drawPerfTimer("I/O Thread Load", perf_IOLoad, "%", width*15, font);

    glTranslated(0, -spacing, 0);
    drawPerfTimer("ReadRate", perf_IOBandwidth, "MB/s", (int)width*17, font);

    glPopMatrix();
}
#endif

void GLvideo_rt::run()
{
    if(DEBUG) printf("Starting renderthread\n");

    VideoData *videoData = NULL;

    QTime fpsIntervalTime;        //measures FPS, averaged over several frames
    QTime frameIntervalTime;    //measured the total period of each frame
    QTime perfTimer;            //performance timer for measuring indivdual processes during rendering
    int fpsAvgPeriod=1;
    float fps = 0;

    //flags and status
    int lastsrcwidth = 0;
    int lastsrcheight = 0;
    bool createGLTextures = false;
    bool updateShaderVars = false;

    //declare shadow variables for the thread worker and initialise them
    mutex.lock();
    bool aspectLock = m_aspectLock;
    bool doRendering = m_doRendering;
    bool doResize = m_doResize;
    bool showLuminance = m_showLuminance;
    bool showChrominance = m_showChrominance;
    bool changeFont = m_changeFont;
    int displaywidth = m_displaywidth;
    int displayheight = m_displayheight;
    int frameRepeats = m_frameRepeats;
    bool interlacedSource = m_interlacedSource;
    bool deinterlace = m_deinterlace;
    bool matrixScaling = m_matrixScaling;
    bool changeMatrix = m_changeMatrix;
    int repeat = 0;
    int field = 0;
    int direction = 0;
    int currentShader = 0;
    int osd = m_osd;
    bool perf = m_perf;
    float luminanceOffset1 = m_luminanceOffset1;
    float chrominanceOffset1 = m_chrominanceOffset1;
    float luminanceMultiplier = m_luminanceMultiplier;
    float chrominanceMultiplier = m_chrominanceMultiplier;
    float luminanceOffset2 = m_luminanceOffset2;
    float chrominanceOffset2 = m_chrominanceOffset2;
    float colourMatrix[9];
    float matrixKr = m_matrixKr;
    float matrixKg = m_matrixKg;
    float matrixKb = m_matrixKb;
    float osdScale = m_osdScale;

    mutex.unlock();

#ifdef HAVE_FTGL
    FTFont *font = NULL;
#endif

    //initialise OpenGL
    glw.makeCurrent();
    GLfuncs::loadGLExtSyms();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0,0, displayheight, displaywidth);
    glClearColor(0, 0, 0, 0);
    glEnable(GL_BLEND);
    glClear(GL_COLOR_BUFFER_BIT);

    compileFragmentShaders();

    while (doRendering) {
        perfTimer.start();
        //update shadow variables
        mutex.lock();

        //values
        displaywidth = m_displaywidth;
        displayheight = m_displayheight;
        frameRepeats = m_frameRepeats;
        osd = m_osd;
        osdScale = m_osdScale;
        perf = m_perf;

        doResize = m_doResize;
        m_doResize = false;

        //boolean flags
        doRendering = m_doRendering;

        if(m_changeMatrix == true) {
            matrixScaling = m_matrixScaling;
            matrixKr = m_matrixKr;
            matrixKg = m_matrixKg;
            matrixKb = m_matrixKb;
            changeMatrix = true;
            updateShaderVars = true;
            m_changeMatrix = false;
        }

        if(m_changeFont) {
            changeFont = true;
            m_changeFont = false;
        }

        //check for changed aspect ratio lock
        if(aspectLock != m_aspectLock) {
            aspectLock = m_aspectLock;
            doResize = true;
        }

        if(showLuminance != m_showLuminance) {
            showLuminance = m_showLuminance;
            updateShaderVars = true;
        }

        if(showChrominance != m_showChrominance) {
            showChrominance = m_showChrominance;
            updateShaderVars = true;
        }

        if(luminanceMultiplier != m_luminanceMultiplier) {
            luminanceMultiplier = m_luminanceMultiplier;
            updateShaderVars = true;
        }

        if(chrominanceMultiplier != m_chrominanceMultiplier) {
            chrominanceMultiplier = m_chrominanceMultiplier;
            updateShaderVars = true;
        }

        if(luminanceOffset1 != m_luminanceOffset1) {
            luminanceOffset1 = m_luminanceOffset1;
            updateShaderVars = true;
        }

        if(chrominanceOffset1 != m_chrominanceOffset1) {
            chrominanceOffset1 = m_chrominanceOffset1;
            updateShaderVars = true;
        }

        if(luminanceOffset2 != m_luminanceOffset2) {
            luminanceOffset2 = m_luminanceOffset2;
            updateShaderVars = true;
        }

        if(chrominanceOffset2 != m_chrominanceOffset2) {
            chrominanceOffset2 = m_chrominanceOffset2;
            updateShaderVars = true;
        }

        if(interlacedSource != m_interlacedSource) {
            interlacedSource = m_interlacedSource;
            updateShaderVars = true;
        }

        if(deinterlace != m_deinterlace) {
            deinterlace = m_deinterlace;
            updateShaderVars = true;
        }

        mutex.unlock();
        perf_getParams = perfTimer.elapsed();

        if(changeMatrix) {
            buildColourMatrix(colourMatrix, matrixKr, matrixKg, matrixKb, matrixScaling, matrixScaling);
            changeMatrix = false;
        }

        //read frame data, one frame at a time, or after we have displayed the second field
        perfTimer.restart();
        if((interlacedSource == 0 || field == 0) && repeat == 0) {
	    videoData = glw.vt->getNextFrame();
            direction = glw.vt->getDirection();
            perf_futureQueue = glw.fq->getFutureQueueLen();
            perf_pastQueue   = glw.fq->getPastQueueLen();
            //perf_IOLoad      = glw.fq->getIOLoad();
            //perf_IOBandwidth = glw.fq->getIOBandwidth();
            perf_readData = perfTimer.elapsed();
        }

	if(DEBUG) printf("videoData=%p\n", videoData);

        if(videoData) {
        	
        	//perform any format conversions
            perfTimer.restart();
            switch(videoData->renderFormat) {

                case VideoData::V210:
                    //check for v210 data - it's very difficult to write a shader for this
                    //due to the lack of GLSL bitwise operators, and it's insistance on filtering the textures
                	if(DEBUG) printf("Converting V210 frame\n");
                    videoData->convertV210();
                    break;

                case VideoData::V16P4:
                case VideoData::V16P2:
                case VideoData::V16P0:
                    //convert 16 bit data to 8bit
                    //this avoids endian-ness issues between the host and the GPU
                    //and reduces the bandwidth to the GPU
                	if(DEBUG) printf("Converting 16 bit frame\n");
                    videoData->convertPlanar16();
                    break;

                default:
                    //no conversion needed
                    break;
            }
            perf_convertFormat = perfTimer.elapsed();
            
            //check for video dimensions changing
	    if(lastsrcwidth != videoData->Ywidth || lastsrcheight != videoData->Yheight) {
            	if(DEBUG) printf("Changing video dimensions to %dx%d\n", videoData->Ywidth, videoData->Yheight);
                glOrtho(0, videoData->Ywidth ,0 , videoData->Yheight, -1 ,1);

                doResize = true;
                createGLTextures = true;
                updateShaderVars = true;

                lastsrcwidth = videoData->Ywidth;
                lastsrcheight = videoData->Yheight;
            }

            //check for the shader changing
            {
                int newShader = currentShader;

                if(videoData->isPlanar)
                    newShader = shaderPlanar;
                else
                    newShader = shaderUYVY;

                if(newShader != currentShader) {
                    if(DEBUG) printf("Changing shader from %d to %d\n", currentShader, newShader);
                    createGLTextures = true;
                    updateShaderVars = true;
                    currentShader = newShader;
                }
            }
            
            if(createGLTextures) {
                if(DEBUG) printf("Creating GL textures\n");
                renderer->createTextures(videoData);
                createGLTextures = false;
            }

            if(updateShaderVars) {
                perfTimer.restart();
                if(DEBUG) printf("Updating fragment shader variables\n");

    		GLfuncs::glUseProgramObjectARB(programs[currentShader]);
                //data about the input file to the shader
                int i = GLfuncs::glGetUniformLocationARB(programs[currentShader], "Yheight");
                GLfuncs::glUniform1fARB(i, videoData->Yheight);

                i = GLfuncs::glGetUniformLocationARB(programs[currentShader], "Ywidth");
                GLfuncs::glUniform1fARB(i, videoData->Ywidth);

                i = GLfuncs::glGetUniformLocationARB(programs[currentShader], "CHsubsample");
                GLfuncs::glUniform1fARB(i, (float)(videoData->Ywidth / videoData->Cwidth));

                i = GLfuncs::glGetUniformLocationARB(programs[currentShader], "CVsubsample");
                GLfuncs::glUniform1fARB(i, (float)(videoData->Yheight / videoData->Cheight));

                //settings from the c++ program to the shader
                i = GLfuncs::glGetUniformLocationARB(programs[currentShader], "yuvOffset1");
                float offset1[3];
                offset1[0] = matrixScaling ? luminanceOffset1 : 0.0;    //don't subtract the initial Y offset if the matrix is unscaled
                offset1[1] = chrominanceOffset1;
                offset1[2] = chrominanceOffset1;
                GLfuncs::glUniform3fvARB(i, 1, &offset1[0]);

                i = GLfuncs::glGetUniformLocationARB(programs[currentShader], "yuvMul");
                float mul[3];
                mul[0] = showLuminance   ? luminanceMultiplier   : 0.0;
                mul[1] = showChrominance ? chrominanceMultiplier : 0.0;
                mul[2] = showChrominance ? chrominanceMultiplier : 0.0;
                GLfuncs::glUniform3fvARB(i, 1, &mul[0]);

                i = GLfuncs::glGetUniformLocationARB(programs[currentShader], "yuvOffset2");
                float offset2[3];
                offset2[0] = showLuminance ? luminanceOffset2 : 0.5;    //when luminance is off, set to mid grey
                offset2[1] = chrominanceOffset2;
                offset2[2] = chrominanceOffset2;
                GLfuncs::glUniform3fvARB(i, 1, &offset2[0]);

                i = GLfuncs::glGetUniformLocationARB(programs[currentShader], "colorMatrix");
                GLfuncs::glUniformMatrix3fvARB(i, 1, false, colourMatrix);

                i = GLfuncs::glGetUniformLocationARB(programs[currentShader], "interlacedSource");
                GLfuncs::glUniform1iARB(i, interlacedSource);

                i = GLfuncs::glGetUniformLocationARB(programs[currentShader], "deinterlace");
                GLfuncs::glUniform1iARB(i, deinterlace);
                updateShaderVars = false;
                perf_updateVars = perfTimer.elapsed();
    		GLfuncs::glUseProgramObjectARB(0);
            }
            
            if(doResize /* && displaywidth>0 && displayheight>0*/) {
                //resize the viewport, once we have some video
                //if(DEBUG) printf("Resizing to %d, %d\n", displaywidth, displayheight);

                float sourceAspect = (float)videoData->Ywidth / (float)videoData->Yheight;
                float displayAspect = (float)displaywidth / (float)displayheight;

                if(sourceAspect == displayAspect || aspectLock == false) {
                    glViewport(0, 0, displaywidth, displayheight);
                }
                else {
                    if(displayAspect > sourceAspect) {
                        //window is wider than image should be
                        int width = (int)(displayheight*sourceAspect);
                        int offset = (displaywidth - width) / 2;
                        glViewport(offset, 0, width, displayheight);
                    }
                    else {
                        int height = (int)(displaywidth/sourceAspect);
                        int offset = (displayheight - height) / 2;
                        glViewport(0, offset, displaywidth, height);
                    }
                }
                doResize = false;
            }

#ifdef HAVE_FTGL
            if(changeFont) {
            	if(DEBUG) printf("Changing font\n");
            	if(font)
            		delete font;

            	font = new FTGLPolygonFont((const char *)fontFile);
            	font->FaceSize(72);
            	font->CharMap(ft_encoding_unicode);

            	changeFont = false;
            }
#endif
            
            //	//upload the texture for the frame to the GPU. This contains both fields for interlaced
            //  renderer->uploadTextures(videoData);
            //
            //	//both fields, if interlaced
            //	for(int i=0; i<=interlacedSource; i++) {
            //
            //		//load the field and direction variables into the YUV->RGB shader
            //		int i = GLfuncs::glGetUniformLocationARB(programs[currentShader], "field");
            //		GLfuncs::glUniform1iARB(i, field);
            //
            //		i = GLfuncs::glGetUniformLocationARB(programs[currentShader], "direction");
            //		GLfuncs::glUniform1iARB(i, direction);
            //
            //		//the frame repeater renders to an intermediate framebuffer object
            //		repeater->setupRGBdestination();
            //
            //		//render the YUV texture to the framebuffer object
            //		renderer->renderVideo(videoData, programs[currentShader]);
            //
            //		//repeatedly render from the framebuffer object to the screen
            //		repeater->setupScreenDestination();
            //
            //		for(int r=0; r<frameRepeats;) {
            //
            //			//render the video from the framebuffer object to the screen
            //			repeater.renderVideo(videoData);
            //
            //			glFlush();
            //			glw.swapBuffers();
            //
            //			//the frame repeater returns the number of times it managed to repeat the frame
            //			//this may be more than once, depending on the platform
            //			r += repeater->repeat(frameRepeats);            
            //		}
            //	}
            //
            //
            
            if(interlacedSource) {
   		GLfuncs::glUseProgramObjectARB(programs[currentShader]);
                int i = GLfuncs::glGetUniformLocationARB(programs[currentShader], "field");
                GLfuncs::glUniform1iARB(i, field);

                i = GLfuncs::glGetUniformLocationARB(programs[currentShader], "direction");
                GLfuncs::glUniform1iARB(i, direction);
    		GLfuncs::glUseProgramObjectARB(0);
            }

            //upload the texture data for each new frame, or for before we render the first field
            //of interlaced material
            perfTimer.restart();
            if((interlacedSource == 0 || field == 0) && repeat == 0) renderer->uploadTextures(videoData);
            perf_upload = perfTimer.elapsed();

            perfTimer.restart();
            renderer->renderVideo(videoData, programs[currentShader]);
            perf_renderVideo = perfTimer.elapsed();

#ifdef HAVE_FTGL
            perfTimer.restart();
            if(osd && font != NULL) renderOSD(videoData, font, fps, osd, osdScale);
            perf_renderOSD = perfTimer.elapsed();
            if(perf==true && font != NULL) renderPerf(videoData, font);
#endif                  
        }


        glFlush();
        perfTimer.restart();
        glw.swapBuffers();
        perf_swapBuffers = perfTimer.elapsed();

        //move to the next field when the first has been repeated the required number of times
        if(interlacedSource && (repeat == frameRepeats)) {
            //move to the next field
            field++;
            if(field > 1) field = 0;
        }

        //repeat the frame the required number of times
        if(repeat < frameRepeats) {
        	repeat++;
        	if(repeat == frameRepeats) {
        		repeat = 0;        
        	}
        }
        
        if(repeat == 0) {
        	//measure overall frame period
        	perf_interval = frameIntervalTime.elapsed();
        	//if(DEBUG) printf("interval=%d\n", perf_interval);
        	frameIntervalTime.restart();
        	
            //calculate FPS
            if (fpsAvgPeriod == 0) {
                int fintvl = fpsIntervalTime.elapsed();
                if (fintvl) fps = 10*1e3/fintvl;
                fpsIntervalTime.start();
            }
            fpsAvgPeriod = (fpsAvgPeriod + 1) %10;        	
        }

GLenum error;
const GLubyte* errStr;
if ((error = glGetError()) != GL_NO_ERROR)
{
errStr = gluErrorString(error);
fprintf(stderr, "OpenGL Error: %s\n", errStr);
}
                
    }
}

