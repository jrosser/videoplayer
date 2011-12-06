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

#include <QtGui>

#include "mainwindow.h"

#include "GLvideo_mt.h"
#include "videoTransport.h"

MainWindow::MainWindow(GLvideo_params& vr_params, Qt_params& qt_params, VideoTransport *vt) :
	vr_params(vr_params), vr_params_orig(vr_params), video_transport(vt)
{
	QPalette p = palette();
	p.setColor(QPalette::Window, Qt::black);
	setPalette(p);

	if (qt_params.hidemouse) {
		//hide the mouse pointer from the start if requestes
		setCursor(QCursor(Qt::BlankCursor));
	}

	setWindowTitle("VideoPlayer");
	setFullScreen(qt_params.start_fullscreen);

	//central widget is the threaded openGL video widget
	//which pulls video from the videoRead
	//and gets stats for the OSD from the readThread
	glvideo_mt = new GLvideo_mt(this, vt, vr_params);

	setCentralWidget(glvideo_mt);

	//set up menus etc
	createActions();
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

	ADDACT("Quit", "Q", qApp, quit);
	ADDACT("Use user Matrix", "k", this, setUserMatrix);
	ADDACT("Set HDTV Matrix", "h", this, setHDTVMatrix);
	ADDACT("Set SDTV Matrix", "j", this, setSDTVMatrix);
	ADDACT("Use user matrix scaling",  "m", this, setUserMatrixScaling);
	ADDACT("Use computer graphics matrix scaling", "b", this, setCgrMatrixScaling);
	ADDACT("Use video levels matrix scaling", "n", this, setVideoMatrixScaling);
	ADDACT("Toggle deinterlacing", "i", this, toggleDeinterlace);
	ADDACT("Toggle OSD", "o", this, toggleOSD);
	ADDACT("Toggle performance", "?", this, togglePerf);
	ADDACT("Toggle luma", "y", this, toggleLuminance);
	ADDACT("Toggle chroma", "c", this, toggleChrominance);
	ADDACT("Toggle aspect ratio Lock", "a", this, toggleAspectLock);
	ADDACT("Toggle full screen", "f", this, toggleFullScreen);
	ADDACT("Escape full screen", "Escape", this, escapeFullScreen);
	ADDACT("Forward 1x", "1", video_transport, transportFwd1);
	ADDACT("Forward 2x", "2", video_transport, transportFwd2);
	ADDACT("Forward 5x", "3", video_transport, transportFwd5);
	ADDACT("Forward 10x", "4", video_transport, transportFwd10);
	ADDACT("Forward 20x", "5", video_transport, transportFwd20);
	ADDACT("Forward 50x", "6", video_transport, transportFwd50);
	ADDACT("Forward 100x", "7", video_transport, transportFwd100);
	ADDACT("Stop", "s", video_transport, transportStop);
	ADDACT("Reverse 1x", "Ctrl+1", video_transport, transportRev1);
	ADDACT("Reverse 2x", "Ctrl+2", video_transport, transportRev2);
	ADDACT("Reverse 5x", "Ctrl+3", video_transport, transportRev5);
	ADDACT("Reverse 10x", "Ctrl+4", video_transport, transportRev10);
	ADDACT("Reverse 20x", "Ctrl+5", video_transport, transportRev20);
	ADDACT("Reverse 50x", "Ctrl+6", video_transport, transportRev50);
	ADDACT("Reverse 100x", "Ctrl+7", video_transport, transportRev100);
	ADDACT("Play/Pause", "Space", video_transport, transportPlayPause);
	ADDACT("Jog Forward", ".", video_transport, transportJogFwd);
	ADDACT("Jog Reverse", ",", video_transport, transportJogRev);
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
	vr_params.matrix_Kr = vr_params_orig.matrix_Kr;
	vr_params.matrix_Kg = vr_params_orig.matrix_Kg;
	vr_params.matrix_Kb = vr_params_orig.matrix_Kb;
	vr_params.matrix_valid = false;
}

void MainWindow::setUserMatrixScaling()
{
	vr_params.output_range = vr_params_orig.output_range;
	vr_params.output_blacklevel = vr_params_orig.output_blacklevel;
	vr_params.matrix_valid = false;
}

void MainWindow::setVideoMatrixScaling()
{
	vr_params.output_range = 220;
	vr_params.output_blacklevel = 16;
	vr_params.matrix_valid = false;
}

void MainWindow::setCgrMatrixScaling()
{
	vr_params.output_range = 256;
	vr_params.output_blacklevel = 0;
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
}

void MainWindow::toggleLuminance()
{
	vr_params.show_luma ^= 1;
	if (vr_params.show_luma) {
		vr_params.luminance_mul = vr_params_orig.luminance_mul;
		vr_params.luminance_offset2 = vr_params_orig.luminance_offset2;
	}
	else {
		vr_params.luminance_mul = 0.;
		vr_params.luminance_offset2 = (float)(128./255.);
	}
	vr_params.matrix_valid = false;
}

void MainWindow::toggleChrominance()
{
	vr_params.show_chroma ^= 1;
	if (vr_params.show_chroma) {
		vr_params.chrominance_mul = vr_params_orig.chrominance_mul;
		vr_params.chrominance_offset2 = vr_params_orig.chrominance_offset2;
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
	/* configure the video transport such that if deinterlacing is not occuring,
	 * to only display a frame once when jogging */
	video_transport->setSteppingIgnoreInterlace(!vr_params.deinterlace);
}

