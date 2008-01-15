/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: GLvideo_mt.h,v 1.21 2008-01-15 15:01:34 jrosser Exp $
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

#ifndef GLVIDEO_MT_H
#define GLVIDEO_MT_H

#define GL_GLEXT_LEGACY
#include <GL/gl.h>
#include "glext.h"

#include <QtGui>
#include <QGLWidget>

#include "GLvideo_rt.h"
#include "videoRead.h"

class MainWindow;

class GLvideo_mt : public QGLWidget
{
    Q_OBJECT

public:
	GLvideo_mt(VideoRead &vr);
	void setFrameRepeats(int repeats);
	void setFramePolarity(int p);
	void setFontFile(QString &fontFile);	
	VideoRead &vr;
	void stop();
	
	GLvideo_rt renderThread;
				
protected:
	
public slots:
	void toggleOSD();
	void togglePerf();
	void toggleAspectLock();
	void toggleLuminance();
	void toggleChrominance();
    void toggleDeinterlace();
	void toggleMatrixScaling();    
    void setInterlacedSource(bool i);
    void setDeinterlace(bool d);
	void setLuminanceOffset1(float o);
	void setChrominanceOffset1(float o);	
	void setLuminanceMultiplier(float m);
	void setChrominanceMultiplier(float m);
	void setLuminanceOffset2(float o);
	void setChrominanceOffset2(float o);	
	void setMatrixScaling(bool s);
	void setMatrix(float Kr, float Kg, float Kb);
	void setCaption(QString&);
	void setOsdScale(float s);
	void setOsdTextTransparency(float t);
	void setOsdBackTransparency(float t);
			
private slots:
	void hideMouse();
        
private:
	void initializeGL();

	void paintGL();
	void paintEvent(QPaintEvent * event);
	void resizeEvent(QResizeEvent * event);
	
	void mouseMoveEvent(QMouseEvent *ev);
	QTimer mouseHideTimer;
};

#endif
