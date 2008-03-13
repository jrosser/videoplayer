/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: GLvideo_rt.h,v 1.33 2008-03-13 11:38:49 jrosser Exp $
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

#include <QtGui>

class GLvideo_mt;

class GLvideo_rt : public QThread
{
public:

    enum ShaderPrograms { shaderUYVY, shaderPlanar, shaderMax };

    GLvideo_rt(GLvideo_mt &glWidget);
    void resizeViewport(int w, int h);
    void setFrameRepeats(int repeats);
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
};

#endif /*GLVIDEO_RT_H_*/
