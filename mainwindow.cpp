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

#include "mainwindow.h"

#include "GLvideo_mt.h"

#include "readerInterface.h"
#include "yuvReader.h"
#ifdef HAVE_DIRAC
#include "diracReader.h"
#endif

#include "frameQueue.h"
#include "videoTransport.h"
#ifdef Q_OS_LINUX
#include "QShuttlePro.h"
#endif

#include "GLvideo_params.h"

using namespace std;

MainWindow::MainWindow(GLvideo_params& vr_params, Qt_params& qt_params) :
	vr_params(vr_params)
{
	QPalette p = palette();
	p.setColor(QPalette::Window, Qt::black);
	setPalette(p);

	quit_at_end = qt_params.quit_at_end;

	if (qt_params.hideMouse) {
		//hide the mouse pointer from the start if requestes
		setCursor(QCursor(Qt::BlankCursor));
	}

	setWindowTitle("VideoPlayer");
	setFullScreen(qt_params.startFullScreen);

	//object containing a seperate thread that manages the lists of frames to be displayed
	frameQueue = new FrameQueue();

	//object that generates frames to be inserted into the frame queue
	reader = NULL;
#ifdef HAVE_DIRAC
	//make dirac reader if required
	if(qt_params.fileType.toLower() == "drc") {
		DiracReader *r = new DiracReader( *frameQueue );

		r->setFileName(qt_params.fileName);
		reader = r;
		r->start(QThread::LowestPriority);
	}
#endif

	//default to YUV reader
	if (reader == NULL) {
		YUVReader *r = new YUVReader( *frameQueue );

		//YUV reader parameters
		r->setVideoWidth(qt_params.videoWidth);
		r->setVideoHeight(qt_params.videoHeight);
		r->setForceFileType(qt_params.forceFileType);
		r->setFileType(qt_params.fileType);
		r->setFileName(qt_params.fileName);

		reader = r;
	}

	frameQueue->setReader(reader);

	//object controlling the video playback 'transport'
	videoTransport = new VideoTransport(frameQueue);

	frameQueue->start();
	//central widget is the threaded openGL video widget
	//which pulls video from the videoRead
	//and gets stats for the OSD from the readThread
	glvideo_mt = new GLvideo_mt(this, videoTransport, frameQueue, vr_params);

	setCentralWidget(glvideo_mt);
	glvideo_mt->start();

	//set up menus etc
	createActions();

#ifdef Q_OS_LINUX
	//shuttlePro jog dial - linux only native support at the moment
	shuttle = NULL;
	shuttle = new QShuttlePro();

	//shuttlepro jog wheel
	connect(shuttle, SIGNAL(jogForward()), videoTransport, SLOT(transportJogFwd()));
	connect(shuttle, SIGNAL(jogBackward()), videoTransport, SLOT(transportJogRev()));

	//shuttlepro shuttle dial
	connect(shuttle, SIGNAL(shuttleRight7()), videoTransport, SLOT(transportFwd100()));
	connect(shuttle, SIGNAL(shuttleRight6()), videoTransport, SLOT(transportFwd50()));
	connect(shuttle, SIGNAL(shuttleRight5()), videoTransport, SLOT(transportFwd20()));
	connect(shuttle, SIGNAL(shuttleRight4()), videoTransport, SLOT(transportFwd10()));
	connect(shuttle, SIGNAL(shuttleRight3()), videoTransport, SLOT(transportFwd5()));
	connect(shuttle, SIGNAL(shuttleRight2()), videoTransport, SLOT(transportFwd2()));
	connect(shuttle, SIGNAL(shuttleRight1()), videoTransport, SLOT(transportFwd1()));

	connect(shuttle, SIGNAL(shuttleLeft7()), videoTransport, SLOT(transportRev100()));
	connect(shuttle, SIGNAL(shuttleLeft6()), videoTransport, SLOT(transportRev50()));
	connect(shuttle, SIGNAL(shuttleLeft5()), videoTransport, SLOT(transportRev20()));
	connect(shuttle, SIGNAL(shuttleLeft4()), videoTransport, SLOT(transportRev10()));
	connect(shuttle, SIGNAL(shuttleLeft3()), videoTransport, SLOT(transportRev5()));
	connect(shuttle, SIGNAL(shuttleLeft2()), videoTransport, SLOT(transportRev2()));
	connect(shuttle, SIGNAL(shuttleLeft1()), videoTransport, SLOT(transportRev1()));
	connect(shuttle, SIGNAL(shuttleCenter()), videoTransport, SLOT(transportStop()));

	//shuttlepro buttons
	connect(shuttle, SIGNAL(key267Pressed()), videoTransport, SLOT(transportPlayPause()));
	connect(shuttle, SIGNAL(key265Pressed()), videoTransport, SLOT(transportFwd1()));
	connect(shuttle, SIGNAL(key259Pressed()), this, SLOT(toggleFullScreen()));
	connect(shuttle, SIGNAL(key256Pressed()), this, SLOT(toggleOSD()));
	connect(shuttle, SIGNAL(key257Pressed()), this, SLOT(toggleAspectLock()));

	//this is what the IngexPlayer also does
	//key 269, press=previous mark, hold=start of file
	//key 270, press=next mark, hold=end of file
	//key 257, cycle displayed timecode type
	//key 258, press=lock controls, hold=unlock controls
#endif

	//video reader parameters
	videoTransport->setLooping(qt_params.looping);
	connect(videoTransport, SIGNAL(endOfFile()), this, SLOT(endOfFile()));

	//rendering thread parameters
}

//generate actions for menus and keypress handlers
void MainWindow::createActions()
{
#define ADDACT(str, key, target, fn) \
	do { QAction *a = new QAction(str, this); \
	     a->setShortcut(tr(key)); \
	     addAction(a); \
	     connect(a, SIGNAL(triggered()), target, SLOT(fn())); \
	} while(0);

	ADDACT("Quit", "Q", this, quit);
	ADDACT("Use user Matrix", "k", this, setUserMatrix);
	ADDACT("Set HDTV Matrix", "h", this, setHDTVMatrix);
	ADDACT("Set SDTV Matrix", "j", this, setSDTVMatrix);
	ADDACT("Toggle colour matrix scaling", "m", this, toggleMatrixScaling);
	ADDACT("Toggle deinterlacing", "i", this, toggleDeinterlace);
	ADDACT("Toggle OSD", "o", this, toggleOSD);
	ADDACT("Toggle performance", "?", this, togglePerf);
	ADDACT("Toggle luma", "y", this, toggleLuminance);
	ADDACT("Toggle chroma", "c", this, toggleChrominance);
	ADDACT("Toggle aspect ratio Lock", "a", this, toggleAspectLock);
	ADDACT("Toggle full screen", "toggleFullScreen", this, toggleFullScreen);
	ADDACT("Escape full screen", "Escape", this, escapeFullScreen);
	ADDACT("Forward 1x", "1", videoTransport, transportFwd1);
	ADDACT("Forward 2x", "2", videoTransport, transportFwd2);
	ADDACT("Forward 5x", "5", videoTransport, transportFwd5);
	ADDACT("Forward 10x", "10", videoTransport, transportFwd10);
	ADDACT("Forward 20x", "20", videoTransport, transportFwd20);
	ADDACT("Forward 50x", "50", videoTransport, transportFwd50);
	ADDACT("Forward 100x", "100", videoTransport, transportFwd100);
	ADDACT("Stop", "s", videoTransport, transportStop);
	ADDACT("Reverse 1x", "Ctrl+1", videoTransport, transportRev1);
	ADDACT("Reverse 2x", "Ctrl+2", videoTransport, transportRev2);
	ADDACT("Reverse 5x", "Ctrl+3", videoTransport, transportRev5);
	ADDACT("Reverse 10x", "Ctrl+4", videoTransport, transportRev10);
	ADDACT("Reverse 20x", "Ctrl+5", videoTransport, transportRev20);
	ADDACT("Reverse 50x", "Ctrl+6", videoTransport, transportRev50);
	ADDACT("Reverse 100x", "Ctrl+7", videoTransport, transportRev100);
	ADDACT("Play/Pause", "Space", videoTransport, transportPlayPause);
	ADDACT("Jog Forward", ".", videoTransport, transportJogFwd);
	ADDACT("Jog Reverse", ",", videoTransport, transportJogRev);
}

//slot to receive full screen toggle command
void MainWindow::toggleFullScreen()
{
	setFullScreen(!isFullScreen());
}

void MainWindow::escapeFullScreen()
{
	if (isFullScreen())
		setFullScreen(false);
}

//full screen toggle worker
void MainWindow::setFullScreen(bool fullscreen)
{
	if (fullscreen) {
		showFullScreen();
	}
	else {
		showNormal();
	}
}

void MainWindow::setHDTVMatrix()
{
	SetLumaCoeffsRec709(vr_params);
	vr_params.matrix_valid = false;
}

void MainWindow::setSDTVMatrix()
{
	SetLumaCoeffsRec601(vr_params);
	vr_params.matrix_valid = false;
}

void MainWindow::setUserMatrix()
{
	vr_params.matrix_Kr = vr_params.userKr;
	vr_params.matrix_Kg = vr_params.userKg;
	vr_params.matrix_Kb = vr_params.userKb;
	vr_params.matrix_valid = false;
}

void MainWindow::togglePerf()
{
	vr_params.osd_perf ^= 1;
}

void MainWindow::toggleOSD()
{
	++vr_params.osd_bot;
}

void MainWindow::toggleAspectLock()
{
	vr_params.aspect_ratio_lock ^= 1;
	vr_params.view_valid = false;
}
void MainWindow::toggleLuminance()
{
	vr_params.show_luma ^= 1;
	if (vr_params.show_luma) {
		vr_params.luminance_mul = 1.;
		vr_params.luminance_offset2 = 0.;
	}
	else {
		vr_params.luminance_mul = 0.;
		vr_params.luminance_offset2 = 128./255.;
	}
	vr_params.matrix_valid = false;
}
void MainWindow::toggleChrominance()
{
	vr_params.show_chroma ^= 1;
	if (vr_params.show_chroma) {
		vr_params.chrominance_mul = 1.;
		vr_params.chrominance_offset2 = 0.;
	}
	else {
		vr_params.chrominance_mul = 0.;
		vr_params.chrominance_offset2 = 0.;
	}
	vr_params.matrix_valid = false;
}
void MainWindow::toggleDeinterlace()
{
	vr_params.deinterlace ^= 1;
}
void MainWindow::toggleMatrixScaling()
{
	//vr_params.matrix_scaling ^= 1;
	vr_params.matrix_valid = false;
}

void MainWindow::endOfFile()
{
	if (quit_at_end == true)
		quit();
}

void MainWindow::quit()
{
#ifdef Q_OS_LINUX
	if(shuttle) {
		shuttle->stop();
		shuttle->wait();
	}
#endif

	glvideo_mt->stop();
	frameQueue->stop();

	qApp->quit();
}
