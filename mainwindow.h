/* ***** BEGIN LICENSE BLOCK *****
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

#include <QtGui>

#include "GLvideo_params.h"

#ifdef Q_OS_LINUX
class QShuttlePro;
#endif

class ReaderInterface;
class FrameQueue;
class VideoTransport;
class GLvideo_mt;
class GLvideo_params;

struct Qt_params {
	bool startFullScreen;
	QString fileName;
	QString fileType;
	bool forceFileType;
	int videoWidth;
	int videoHeight;
	bool hideMouse;
	bool looping;
	bool quit_at_end;
};

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow(GLvideo_params& vr_params, Qt_params& qt_params);

protected:

private slots:
	void toggleFullScreen();
	void escapeFullScreen();
	void setHDTVMatrix();
	void setSDTVMatrix();
	void setUserMatrix();
	void quit();
	void endOfFile();
	void togglePerf();
	void toggleOSD();
	void toggleAspectLock();
	void toggleLuminance();
	void toggleChrominance();
	void toggleDeinterlace();
	void toggleMatrixScaling();

private:
	void createActions(void);
	void parseCommandLine(int argc, char **argv);
	void usage();

#ifdef Q_OS_LINUX
QShuttlePro *shuttle;
#endif

	void setFullScreen(bool);
	ReaderInterface *reader;
	FrameQueue *frameQueue;
	VideoTransport *videoTransport;
	GLvideo_mt *glvideo_mt;

	GLvideo_params& vr_params;
	GLvideo_params vr_params_orig;

	bool quit_at_end;
};

#endif
