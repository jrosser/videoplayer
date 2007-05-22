/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: mainwindow.cpp,v 1.22 2007-05-22 14:56:28 jrosser Exp $
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

#include "mainwindow.h"

#include <QtGui>

MainWindow::MainWindow()
{
	//some defaults in the abscence of any settings	
	videoWidth = 1920;
	videoHeight = 1080;
	frameRepeats = 0;
	framePolarity = 0;
	luminanceOffset1 = -0.0625;
	chrominanceOffset1 = -0.5;
	luminanceMul = 1.0;
	chrominanceMul = 1.0;
	luminanceOffset2 = 0.0;
	chrominanceOffset2 = 0.0;
	interlacedSource = false;
	deinterlace = false;	
	fileName = "";
	forceFileType = false;
	fileType = "";
	fontFile = "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans-Bold.ttf";
		
	//override settings with command line
	parseCommandLine();
	
	if(allParsed == false)
		return;
	
	setWindowTitle("VideoPlayer");
	
	videoRead = NULL;
	glvideo_mt = NULL;
			
	//threaded video reader
	videoRead = new VideoRead();
	
	//central widget is the threaded openGL video widget
	//which pulls video from the reader
	glvideo_mt = new GLvideo_mt(*videoRead);			
  	setCentralWidget(glvideo_mt);
  				
	//set up menus etc
 	createActions();

#ifdef Q_OS_LINUX		
	//shuttlePro jog dial - linux only native support at the moment	
	shuttle = NULL;	
	shuttle = new QShuttlePro(this);
        
    //shuttlepro jog wheel
	connect(shuttle, SIGNAL(jogForward()), videoRead, SLOT(transportJogFwd()));
	connect(shuttle, SIGNAL(jogBackward()), videoRead, SLOT(transportJogRev()));
	
	//shuttlepro shuttle dial	    
	connect(shuttle, SIGNAL(shuttleRight7()), videoRead, SLOT(transportFwd100()));
	connect(shuttle, SIGNAL(shuttleRight6()), videoRead, SLOT(transportFwd50()));
	connect(shuttle, SIGNAL(shuttleRight5()), videoRead, SLOT(transportFwd20()));
	connect(shuttle, SIGNAL(shuttleRight4()), videoRead, SLOT(transportFwd10()));
	connect(shuttle, SIGNAL(shuttleRight3()), videoRead, SLOT(transportFwd5()));
	connect(shuttle, SIGNAL(shuttleRight2()), videoRead, SLOT(transportFwd2()));
	connect(shuttle, SIGNAL(shuttleRight1()), videoRead, SLOT(transportFwd1()));
	
	connect(shuttle, SIGNAL(shuttleLeft7()), videoRead, SLOT(transportRev100()));
	connect(shuttle, SIGNAL(shuttleLeft6()), videoRead, SLOT(transportRev50()));
	connect(shuttle, SIGNAL(shuttleLeft5()), videoRead, SLOT(transportRev20()));
	connect(shuttle, SIGNAL(shuttleLeft4()), videoRead, SLOT(transportRev10()));
	connect(shuttle, SIGNAL(shuttleLeft3()), videoRead, SLOT(transportRev5()));			
	connect(shuttle, SIGNAL(shuttleLeft2()), videoRead, SLOT(transportRev2()));
	connect(shuttle, SIGNAL(shuttleLeft1()), videoRead, SLOT(transportRev1()));	
	connect(shuttle, SIGNAL(shuttleCenter()), videoRead, SLOT(transportStop()));			
	
	//shuttlepro buttons
	connect(shuttle, SIGNAL(key267Pressed()), videoRead, SLOT(transportPlayPause()));
	connect(shuttle, SIGNAL(key265Pressed()), videoRead, SLOT(transportFwd1()));
	connect(shuttle, SIGNAL(key259Pressed()), this, SLOT(toggleFullScreen()));
	connect(shuttle, SIGNAL(key256Pressed()), glvideo_mt, SLOT(toggleOSD()));
	connect(shuttle, SIGNAL(key257Pressed()), glvideo_mt, SLOT(toggleAspectLock()));
			
	//this is what the IngexPlayer also does
	//key 269, press=previous mark, hold=start of file
	//key 270, press=next mark, hold=end of file
	//key 257, cycle displayed timecode type
	//key 258, press=lock controls, hold=unlock controls
#endif
	
	videoRead->setVideoWidth(videoWidth);
	videoRead->setVideoHeight(videoHeight);
	videoRead->setForceFileType(forceFileType);
	videoRead->setFileType(fileType);	
	videoRead->setFileName(fileName);
	
	glvideo_mt->setFrameRepeats(frameRepeats);
	glvideo_mt->setFramePolarity(framePolarity);
	glvideo_mt->setFontFile(fontFile);	
	glvideo_mt->setLuminanceOffset1(luminanceOffset1);
	glvideo_mt->setChrominanceOffset1(chrominanceOffset1);
	glvideo_mt->setLuminanceMultiplier(luminanceMul);
	glvideo_mt->setChrominanceMultiplier(chrominanceMul);
	glvideo_mt->setLuminanceOffset2(luminanceOffset2);
	glvideo_mt->setChrominanceOffset2(chrominanceOffset2);
	glvideo_mt->setInterlacedSource(interlacedSource);
	glvideo_mt->setDeinterlace(deinterlace);		
}

void MainWindow::parseCommandLine()
{
	QVector<bool> parsed(qApp->arguments().size());
	const QStringList &args = qApp->arguments();
	bool showUsage = false;
	
	for(int i=1; i<parsed.size(); i++)
		parsed[i] = false;
			
	for(int i=1; i<parsed.size(); i++) {
		
		//video width
		if(args[i] == "-w") {
			bool ok;
			int val;
			
			if(parsed.size() > i) {
				parsed[i] = true;
				val = args[i+1].toInt(&ok);
				
				if(ok) {
					i++;
					parsed[i] = true;
					videoWidth = val;	
				}
			}				
		}

		//video height
		if(args[i] == "-h") {
			bool ok;
			int val;
			
			parsed[i] = true;
			val = args[i+1].toInt(&ok);
				
			if(ok) {
				i++;
				parsed[i] = true;
				videoHeight = val;
			}				
		}
		
		//frame repeats
		if(args[i] == "-r") {
			bool ok;
			int val;
			
			parsed[i] = true;
			val = args[i+1].toInt(&ok);
				
			if(ok) {
				i++;
				parsed[i] = true;
				frameRepeats = val;
			}				
		}

		//frame in sequence 0...repeats-1 on which to show new frame
		if(args[i] == "-p") {
			bool ok;
			int val;
			
			parsed[i] = true;
			val = args[i+1].toInt(&ok);
				
			if(ok) {
				i++;
				parsed[i] = true;
				framePolarity = val;
			}				
		}

		//luminance values data offset
		if(args[i] == "-yo") {
			bool ok;
			float val;
			
			parsed[i] = true;
			val = args[i+1].toFloat(&ok);
				
			if(ok) {
				i++;
				parsed[i] = true;
				luminanceOffset1 = val;
			}				
		}

		//chrominance values data offset
		if(args[i] == "-co") {
			bool ok;
			float val;
			
			parsed[i] = true;
			val = args[i+1].toFloat(&ok);
				
			if(ok) {
				i++;
				parsed[i] = true;
				chrominanceOffset1 = val;
			}				
		}

		//luminance values data offset
		if(args[i] == "-yo2") {
			bool ok;
			float val;
			
			parsed[i] = true;
			val = args[i+1].toFloat(&ok);
				
			if(ok) {
				i++;
				parsed[i] = true;
				luminanceOffset2 = val;
			}				
		}

		//chrominance values data offset
		if(args[i] == "-co2") {
			bool ok;
			float val;
			
			parsed[i] = true;
			val = args[i+1].toFloat(&ok);
				
			if(ok) {
				i++;
				parsed[i] = true;
				chrominanceOffset2 = val;
			}				
		}

		//luminance values multipler
		if(args[i] == "-ym") {
			bool ok;
			float val;
			
			parsed[i] = true;
			val = args[i+1].toFloat(&ok);
				
			if(ok) {
				i++;
				parsed[i] = true;
				luminanceMul = val;
			}				
		}

		//luminance values multipler
		if(args[i] == "-cm") {
			bool ok;
			float val;
			
			parsed[i] = true;
			val = args[i+1].toFloat(&ok);
				
			if(ok) {
				i++;
				parsed[i] = true;
				chrominanceMul = val;
			}				
		}

		//source pictures interlace
		if(args[i] == "-i") {
			bool ok;
			bool val;
			
			parsed[i] = true;
			val = (bool)args[i+1].toInt(&ok);
				
			if(ok) {
				i++;
				parsed[i] = true;
				interlacedSource = val;
			}				
		}

		//deinterlace interlaced pictures
		if(args[i] == "-d") {
			bool ok;
			bool val;
			
			parsed[i] = true;
			val = (bool)args[i+1].toInt(&ok);
				
			if(ok) {
				i++;
				parsed[i] = true;
				deinterlace = val;
			}				
		}

		//specify OSD font file
		if(args[i] == "-f") {			
			parsed[i] = true;
			fontFile = args[i+1];
			i++;
			parsed[i] = true;
		}

		//force yuv file type
		if(args[i] == "-t") {			
			parsed[i] = true;
			forceFileType = true;
			fileType = args[i+1];
			i++;
			
			parsed[i] = false;
			if(fileType.toLower() == "i420") parsed[i] = true;
			if(fileType.toLower() == "yv12") parsed[i] = true;
			if(fileType.toLower() == "uyvy") parsed[i] = true;
			if(fileType.toLower() == "v216") parsed[i] = true;
			if(fileType.toLower() == "v210") parsed[i] = true;															
		}
		
		//display usage
		if(args[i] == "-h") {			
			showUsage = true;
			parsed[i] = true;			
		}

	}
		
	fileName = args.last();
	QFileInfo fi(fileName);	
	
	if(fi.exists()) {
		
		parsed[parsed.size()-1] = true;
		
		if(forceFileType == false) {

			//file extension must be one we know about
			parsed[parsed.size()-1] = false;			
			if(fi.suffix().toLower() == "i420") parsed[parsed.size()-1] = true;
			if(fi.suffix().toLower() == "yv12") parsed[parsed.size()-1] = true;
			if(fi.suffix().toLower() == "uyvy") parsed[parsed.size()-1] = true;
			if(fi.suffix().toLower() == "v216") parsed[parsed.size()-1] = true;
			if(fi.suffix().toLower() == "v210") parsed[parsed.size()-1] = true;		
		
			if(parsed[parsed.size()-1] == false) {
				printf("Do not know how to play file with extension %s\n", fi.suffix().toLatin1().data());
				printf("Please specify file format with the -t flag\n");
			}																					
		}
	}
	
	//check all args are parsed
	allParsed = true;	
	for(int i=1; i<args.size(); i++) {
		if(parsed[i] == false) {
			allParsed = false;
			printf("Unknown parameter %s\n", args[i].toLatin1().data());	
		}	
	}
	
	//check video file exists
	QFileInfo info(fileName);
	if(info.exists() == false) {
		printf("Cannot find input file %s\n", fileName.toLatin1().data());
		allParsed = false;	
	} 
	else {
		if(info.isReadable() == false) {
			printf("Cannot read input file %s\n", fileName.toLatin1().data());
			allParsed = false;		
		}
	}

	//check OSD font file exists
	info.setFile(fontFile);
	if(info.exists() == false) {
		printf("Cannot find OSD font file %s\n", fontFile.toLatin1().data());
		allParsed = false;	
	} 
	else {
		if(info.isReadable() == false) {
			printf("Cannot read OSD font file %s\n", fontFile.toLatin1().data());
			allParsed = false;		
		}
	}
	
	if(showUsage == true || allParsed == false) usage();	
}

void MainWindow::usage()
{
    printf("\nOpenGL accelerated YUV video player.");
    printf("\n");
    printf("\nUsage: progname -<flag1> [<flag1_val>] ... <input>");
    printf("\nIn case of multiple assignment to the same parameter, the last holds.");
    printf("\n");
    printf("\nSupported file formats:");
    printf("\n");
    printf("\n  .i420         4:2:0 YUV planar");
    printf("\n  .yv12         4:2:0 YVU planar");
    printf("\n  .uyvy         4:2:2 YUV 8 bit packed");
    printf("\n  .v210         4:2:2 YUV 10 bit packed");
    printf("\n  .v216         4:2:2 YUV 16 bit packed");
    printf("\n");                            
    printf("\nFlag              Type    Default Value   Description");
    printf("\n====              ====    =============== ===========");
    printf("\nw                 ulong   1920            Width of video luminance component");
    printf("\nh                 ulong   1080            Height of video luminance component");
    printf("\nr                 ulong   0               Number of additional times each frame is displayed");
    printf("\np                 ulong   0               Set frame to display video on (0,r-1)");
    printf("\ni                 bool    0               Source video is interlaced (1), progressive (0)");
    printf("\nd                 bool    0               Deinterlace video if source is interlaced (1), no deinterlacing (0)");        
    printf("\nyo                float   -0.0625         Luminance data values offset");
    printf("\nco                float   -0.5            Chrominance data values offset");                        
    printf("\nym                float   1.0             Luminance multipler");
    printf("\ncm                float   1.0             Chrominance multipler");
    printf("\nyo2               float   0.0             Multiplied Luminance offset 2");
    printf("\nco2               float   0.0             Multiplied Chrominance offset 2");                    
    printf("\nf                 string                  TrueType font file for OSD");
    printf("\nt                 string  FileExtension   Force input file type to [i420|yv12|uyvy|v210|v216]");
	printf("\nh                                         Show this usage information");            
    printf("\n");
	printf("\nKeypress               Action");
    printf("\n========               ======");
    printf("\no                      Toggle OSD on/off");
	printf("\nf                      Toggle full screen mode");
	printf("\nEsc                    Return from full screen mode to windowed");
	printf("\na                      Toggle aspect ratio lock");
	printf("\nSpace                  Play/Pause");
	printf("\ns                      Stop");
	printf("\n1,2,3,4,5,6,7          Play forward at 1,2,5,10,20,50,100x");
	printf("\nCTRL + 1,2,3,4,5,6,7   Play backward at 1,2,5,10,20,50,100x");
	printf("\n>                      Jog one frame forward when paused");
	printf("\n<                      Jog one frame backward when paused");
	printf("\ny                      Toggle display of luminance  on/off");
	printf("\nc                      Toggle display of chrominance on/off");	
	printf("\nq                      Quit");	
    printf("\n");
    printf("\n");        
}

//generate actions for menus and keypress handlers
void MainWindow::createActions()
{	
	quitAct = new QAction("Quit", this);
	quitAct->setShortcut(tr("Q"));
	addAction(quitAct);
	connect(quitAct, SIGNAL(triggered()), this, SLOT(quit()));

	toggleDeinterlaceAct = new QAction("Toggle Deinterlacing", this);
	toggleDeinterlaceAct->setShortcut(tr("i"));
	addAction(toggleDeinterlaceAct);
	connect(toggleDeinterlaceAct, SIGNAL(triggered()), glvideo_mt, SLOT(toggleDeinterlace()));
	
	toggleOSDAct = new QAction("Toggle OSD", this);
	toggleOSDAct->setShortcut(tr("o"));		
	addAction(toggleOSDAct);			
	connect(toggleOSDAct, SIGNAL(triggered()), glvideo_mt, SLOT(toggleOSD()));

	toggleLuminanceAct = new QAction("Toggle Luminance", this);
	toggleLuminanceAct->setShortcut(tr("y"));
	addAction(toggleLuminanceAct);
	connect(toggleLuminanceAct, SIGNAL(triggered()), glvideo_mt, SLOT(toggleLuminance()));

	toggleChrominanceAct = new QAction("Toggle Chrominance", this);
	toggleChrominanceAct->setShortcut(tr("c"));
	addAction(toggleChrominanceAct);
	connect(toggleChrominanceAct, SIGNAL(triggered()), glvideo_mt, SLOT(toggleChrominance()));

	toggleAspectLockAct = new QAction("Toggle Aspect Ratio Lock", this);
	toggleAspectLockAct->setShortcut(tr("a"));		
	addAction(toggleAspectLockAct);		
	connect(toggleAspectLockAct, SIGNAL(triggered()), glvideo_mt, SLOT(toggleAspectLock()));	
		
	viewFullScreenAct = new QAction("View full screen", this);
	viewFullScreenAct->setShortcut(tr("f"));		
	addAction(viewFullScreenAct);		
	connect(viewFullScreenAct, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));	
    
    escapeFullScreenAct = new QAction("Escape full screen", this);
	escapeFullScreenAct->setShortcut(tr("Escape"));    
    addAction(escapeFullScreenAct);
    connect(escapeFullScreenAct, SIGNAL(triggered()), this, SLOT(escapeFullScreen()));
    
	transportFwd100Act = new QAction("Forward 100x", this);
	transportFwd100Act->setShortcut(tr("7"));
	addAction(transportFwd100Act);
	connect(transportFwd100Act, SIGNAL(triggered()), videoRead, SLOT(transportFwd100()));
	
	transportFwd50Act = new QAction("Forward 50x", this);
	transportFwd50Act->setShortcut(tr("6"));
	addAction(transportFwd50Act);
	connect(transportFwd50Act, SIGNAL(triggered()), videoRead, SLOT(transportFwd50()));
	
	transportFwd20Act = new QAction("Forward 20x", this);
	transportFwd20Act->setShortcut(tr("5"));
	addAction(transportFwd20Act);
	connect(transportFwd20Act, SIGNAL(triggered()), videoRead, SLOT(transportFwd20()));
	
	transportFwd10Act = new QAction("Forward 10x", this);
	transportFwd10Act->setShortcut(tr("4"));
	addAction(transportFwd10Act);
	connect(transportFwd10Act, SIGNAL(triggered()), videoRead, SLOT(transportFwd10()));
	
	transportFwd5Act = new QAction("Forward 5x", this);
	transportFwd5Act->setShortcut(tr("3"));
	addAction(transportFwd5Act);
	connect(transportFwd5Act, SIGNAL(triggered()), videoRead, SLOT(transportFwd5()));
	
	transportFwd2Act = new QAction("Forward 2x", this);
	transportFwd2Act->setShortcut(tr("2"));
	addAction(transportFwd2Act);
	connect(transportFwd2Act, SIGNAL(triggered()), videoRead, SLOT(transportFwd2()));
	
	transportFwd1Act = new QAction("Forward 1x", this);
	transportFwd1Act->setShortcut(tr("1"));
	addAction(transportFwd1Act);
	connect(transportFwd1Act, SIGNAL(triggered()), videoRead, SLOT(transportFwd1()));
	
	transportStopAct = new QAction("Stop", this);
	transportStopAct->setShortcut(tr("s"));
	addAction(transportStopAct);
	connect(transportStopAct, SIGNAL(triggered()), videoRead, SLOT(transportStop()));
	
	transportRev1Act = new QAction("Reverse 1x", this);
	transportRev1Act->setShortcut(tr("Ctrl+1"));
	addAction(transportRev1Act);
	connect(transportRev1Act, SIGNAL(triggered()), videoRead, SLOT(transportRev1()));
	
	transportRev2Act = new QAction("Reverse 2x", this);
	transportRev2Act->setShortcut(tr("Ctrl+2"));
	addAction(transportRev2Act);
	connect(transportRev2Act, SIGNAL(triggered()), videoRead, SLOT(transportRev2()));
	
	transportRev5Act = new QAction("Reverse 5x", this);
	transportRev5Act->setShortcut(tr("Ctrl+3"));
	addAction(transportRev5Act);
	connect(transportRev5Act, SIGNAL(triggered()), videoRead, SLOT(transportRev5()));
	
	transportRev10Act = new QAction("Reverse 10x", this);
	transportRev10Act->setShortcut(tr("Ctrl+4"));
	addAction(transportRev10Act);
	connect(transportRev10Act, SIGNAL(triggered()), videoRead, SLOT(transportRev10()));
					
	transportRev20Act = new QAction("Reverse 20x", this);
	transportRev20Act->setShortcut(tr("Ctrl+5"));
	addAction(transportRev20Act);
	connect(transportRev20Act, SIGNAL(triggered()), videoRead, SLOT(transportRev20()));
	
	transportRev50Act = new QAction("Reverse 50x", this);
	transportRev50Act->setShortcut(tr("Ctrl+6"));
	addAction(transportFwd100Act);
	connect(transportFwd100Act, SIGNAL(triggered()), videoRead, SLOT(transportFwd100()));
	
	transportRev100Act = new QAction("Reverse 100x", this);								
	transportRev100Act->setShortcut(tr("Ctrl+7"));
	addAction(transportRev100Act);
	connect(transportRev100Act, SIGNAL(triggered()), videoRead, SLOT(transportRev100()));

	transportPlayPauseAct = new QAction("Play/Pause", this);
	transportPlayPauseAct->setShortcut(tr("Space"));
	addAction(transportPlayPauseAct);
	connect(transportPlayPauseAct, SIGNAL(triggered()), videoRead, SLOT(transportPlayPause()));
	
	transportJogFwdAct = new QAction("Jog Forward", this);
	transportJogFwdAct->setShortcut(tr("."));
	addAction(transportJogFwdAct);
	connect(transportJogFwdAct, SIGNAL(triggered()), videoRead, SLOT(transportJogFwd()));
	
	transportJogRevAct = new QAction("Jog Reverse", this);
	transportJogRevAct->setShortcut(tr(","));
	addAction(transportJogRevAct);
	connect(transportJogRevAct, SIGNAL(triggered()), videoRead, SLOT(transportJogRev()));    
}

//slot to receive full screen toggle command
void MainWindow::toggleFullScreen()
{
	setFullScreen(!isFullScreen());	
}

void MainWindow::escapeFullScreen()
{
	if(isFullScreen())
		setFullScreen(false);	
}

//full screen toggle worker
void MainWindow::setFullScreen(bool fullscreen)
{
	if(fullscreen) {
		showFullScreen();				
	}
	else {
		showNormal();
	}	
}

void MainWindow::quit()
{	
#ifdef Q_OS_LINUX
	if(shuttle) {
		shuttle->stop();
		shuttle->wait();
	}
#endif
	
	if(glvideo_mt) {
		glvideo_mt->stop();
	}
	
	if(videoRead) {
		videoRead->stop();
	}
				
	qApp->quit(); 
}
