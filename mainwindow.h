/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: mainwindow.h,v 1.27 2008-01-29 11:52:25 jrosser Exp $
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtAssistant/QAssistantClient>
#include <QtGui>

#include "GLvideo_mt.h"
#include "videoRead.h"

#ifdef Q_OS_LINUX
#include "QShuttlePro.h"
#endif

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(int argc, char **argv);
	bool allParsed;		//flag to say that all commandline parameters were parsed OK.
	    
protected:

private slots:
	void toggleFullScreen();
	void escapeFullScreen();
	void setHDTVMatrix();
	void setSDTVMatrix();
	void setUserMatrix();
	void quit();
	void endOfFile();
	        
private:
	void createActions(void);
	void parseCommandLine(int argc, char **argv);
	void usage();

#ifdef Q_OS_LINUX	
	QShuttlePro *shuttle;
#endif

	QAction *quitAct;

	//display actions
	QAction *setUserMatrixAct;	
	QAction *setHDTVMatrixAct;
	QAction *setSDTVMatrixAct;	
	QAction *toggleMatrixScalingAct;
	QAction *toggleOSDAct;
	QAction *togglePerfAct;	
	QAction *toggleAspectLockAct;		
	QAction *viewFullScreenAct;
	QAction *escapeFullScreenAct;
	QAction *toggleLuminanceAct;
	QAction *toggleChrominanceAct;
	QAction *toggleDeinterlaceAct;
	
	//transport control actions
	QAction *transportFwd100Act;
	QAction *transportFwd50Act;
	QAction *transportFwd20Act;
	QAction *transportFwd10Act;
	QAction *transportFwd5Act;
	QAction *transportFwd2Act;
	QAction *transportFwd1Act;
	QAction *transportStopAct;
	QAction *transportRev1Act;
	QAction *transportRev2Act;
	QAction *transportRev5Act;
	QAction *transportRev10Act;				
	QAction *transportRev20Act;
	QAction *transportRev50Act;
	QAction *transportRev100Act;								

	QAction *transportPlayPauseAct;
	QAction *transportJogFwdAct;
	QAction *transportJogRevAct;
	
	void setFullScreen(bool);
	VideoRead *videoRead;
	GLvideo_mt *glvideo_mt;

	//data from the command line parameters
	int videoWidth;
	int videoHeight;
	int frameRepeats;
	int framePolarity;
	QString fileName;
	QString fontFile;
	
	bool forceFileType;
	QString fileType;
	QString caption;
	
	float luminanceOffset1;
	float chrominanceOffset1;	
	float luminanceMul;
	float chrominanceMul;
	float luminanceOffset2;
	float chrominanceOffset2;
	bool  interlacedSource;
	bool  deinterlace;
	bool  matrixScaling;
	float matrixKr;
	float matrixKg;
	float matrixKb;
	bool startFullScreen;
	float osdScale;
	int osdState;
	float osdBackTransparency;
	float osdTextTransparency;
	bool looping;
	bool quitAtEnd;
	bool hideMouse;		
};

#endif
