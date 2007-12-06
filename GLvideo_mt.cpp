/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: GLvideo_mt.cpp,v 1.19 2007-12-06 16:36:06 jrosser Exp $
*
* Version: MPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Mozilla Public License
* Version 1.1 (the "License"); you may not use this file except in compliance
* with the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
* the specific language governing rights and limitations under the License.
*
* The Original Code is BBC Research and Development code.
*
* The Initial Developer of the Original Code is the British Broadcasting
* Corporation.
* Portions created by the Initial Developer are Copyright (C) 2004.
* All Rights Reserved.
*
* Contributor(s): Jonathan Rosser (Original Author)
*
* Alternatively, the contents of this file may be used under the terms of
* the GNU General Public License Version 2 (the "GPL"), or the GNU Lesser
* Public License Version 2.1 (the "LGPL"), in which case the provisions of
* the GPL or the LGPL are applicable instead of those above. If you wish to
* allow use of your version of this file only under the terms of the either
* the GPL or LGPL and not to allow others to use your version of this file
* under the MPL, indicate your decision by deleting the provisions above
* and replace them with the notice and other provisions required by the GPL
* or LGPL. If you do not delete the provisions above, a recipient may use
* your version of this file under the terms of any one of the MPL, the GPL
* or the LGPL.
* ***** END LICENSE BLOCK ***** */

#include "GLvideo_mt.h"

GLvideo_mt::GLvideo_mt(VideoRead &v) 
	: vr(v), renderThread(*this)
{
	setMouseTracking(true);	
	connect(&mouseHideTimer, SIGNAL(timeout()), this, SLOT(hideMouse()));
	mouseHideTimer.setSingleShot(true);

    /* NB, a) this helps to be last
     *     b) don't put HideMouse anywhere near here */
	renderThread.start();
}

void GLvideo_mt::stop()
{
	renderThread.stop();
	renderThread.wait();	
}

/* Reveal the mouse whenever it is moved,
 * Cause it to be hidden 1second later */
void GLvideo_mt::mouseMoveEvent(QMouseEvent *ev)
{
	ev=ev;
	
	if (!mouseHideTimer.isActive())
		setCursor(QCursor(Qt::ArrowCursor));
	mouseHideTimer.start(1000);
}

void GLvideo_mt::hideMouse()
{
	setCursor(QCursor(Qt::BlankCursor));
}



void GLvideo_mt::toggleOSD() 
{
	renderThread.toggleOSD();	
}

void GLvideo_mt::toggleAspectLock()
{
	renderThread.toggleAspectLock();	
}

void GLvideo_mt::toggleLuminance()
{
	renderThread.toggleLuminance();	
}

void GLvideo_mt::toggleChrominance()
{	
	renderThread.toggleChrominance();	
}

void GLvideo_mt::setInterlacedSource(bool i)
{	
	renderThread.setInterlacedSource(i);	
}

void GLvideo_mt::toggleDeinterlace()
{	
	renderThread.toggleDeinterlace();	
}

void GLvideo_mt::setDeinterlace(bool d)
{	
	renderThread.setDeinterlace(d);
}

void GLvideo_mt::setMatrixScaling(bool s)
{
	renderThread.setMatrixScaling(s);	
}

void GLvideo_mt::setMatrix(float Kr, float Kg, float Kb)
{
	renderThread.setMatrix(Kr, Kg, Kb);	
}

void GLvideo_mt::toggleMatrixScaling()
{
	renderThread.toggleMatrixScaling();	
}

void GLvideo_mt::initializeGL()
{  
	//handled by the worker thread
}

void GLvideo_mt::paintGL()
{
	//handled by the worker thread	
}

void GLvideo_mt::paintEvent(QPaintEvent * event)
{
	event=event;
	//absorb any paint events - let the worker thread update the window	
}

void GLvideo_mt::resizeEvent(QResizeEvent * event)
{
	//added at the suggestion of Trolltech - if the rendering thread
	//is slow to start the first window resize event will issue a makeCurrent() before
	//the rendering thread does, stealing the openGL context from it
	renderThread.resizeViewport(event->size().width(), event->size().height());	
}

void GLvideo_mt::setFrameRepeats(int repeats)
{
	renderThread.setFrameRepeats(repeats);	
}

void GLvideo_mt::setFramePolarity(int p)
{
	renderThread.setFramePolarity(p);
}

void GLvideo_mt::setFontFile(QString &fontFile)
{
	renderThread.setFontFile(fontFile.toLatin1().data());	
}

void GLvideo_mt::setLuminanceMultiplier(float m)
{
	renderThread.setLuminanceMultiplier(m);
}

void GLvideo_mt::setChrominanceMultiplier(float m)
{
	renderThread.setChrominanceMultiplier(m);	
}

void GLvideo_mt::setLuminanceOffset1(float o)
{
	renderThread.setLuminanceOffset1(o);
}

void GLvideo_mt::setChrominanceOffset1(float o)
{
	renderThread.setChrominanceOffset1(o);
}

void GLvideo_mt::setLuminanceOffset2(float o)
{
	renderThread.setLuminanceOffset2(o);
}

void GLvideo_mt::setChrominanceOffset2(float o)
{
	renderThread.setChrominanceOffset2(o);
}
