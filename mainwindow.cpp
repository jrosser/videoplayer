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

#include <iostream>
#include <boost/program_options.hpp>
namespace po = boost::program_options;
using namespace std;

MainWindow::MainWindow(int argc, char **argv, GLvideo_params& vr_params) :
	vr_params(vr_params)
{
	QPalette p = palette();
	p.setColor(QPalette::Window, Qt::black);
	setPalette(p);

	startFullScreen = false;
	fileName = "";
	fileType = "";
	forceFileType = false;
	videoWidth = 1920;
	videoHeight = 1080;

	userKr = (float)0.2126;
	userKg = (float)0.7152;
	userKb = (float)0.0722;

	hideMouse = false;
	looping = true;
	quitAtEnd = false;

	//override settings with command line
	parseCommandLine(argc, argv);

	if (allParsed == false)
		return;

	if (hideMouse) {
		//hide the mouse pointer from the start if requestes
		setCursor(QCursor(Qt::BlankCursor));
	}

	setWindowTitle("VideoPlayer");
	setFullScreen(startFullScreen);

	//object containing a seperate thread that manages the lists of frames to be displayed
	frameQueue = new FrameQueue();

	//object that generates frames to be inserted into the frame queue
	reader = NULL;
#ifdef HAVE_DIRAC    
	//make dirac reader if required
	if(fileType.toLower() == "drc") {
		DiracReader *r = new DiracReader( *frameQueue );

		r->setFileName(fileName);
		reader = r;
		r->start(QThread::LowestPriority);
	}
#endif

	//default to YUV reader
	if (reader == NULL) {
		YUVReader *r = new YUVReader( *frameQueue );

		//YUV reader parameters
		r->setVideoWidth(videoWidth);
		r->setVideoHeight(videoHeight);
		r->setForceFileType(forceFileType);
		r->setFileType(fileType);
		r->setFileName(fileName);

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
	videoTransport->setLooping(looping);
	connect(videoTransport, SIGNAL(endOfFile()), this, SLOT(endOfFile()));

	//rendering thread parameters
}

void MainWindow::parseCommandLine(int argc, char **argv)
{
	allParsed = true; //default to command line parsing succeeding
	try {

        // Declare the supported options.
        po::options_description desc("Allowed options            type   default  description\n"
                                     "===============            ====   =======  ===========");
        desc.add_options()
            ("width,w",       po::value(&videoWidth),         "int    1920    Width of video luminance component")
            ("height,h",      po::value(&videoHeight),        "int    1080    Height of video luminance component")
            ("repeats,r",     po::value(&vr_params.frame_repeats),       "int    0       Frame is repeated r extra times")
            ("loop,l",        po::value(&looping),            "bool   1       Video file is played repeatedly")
            ("quit,q",                                        "               Exit at end of video file")
            ("interlace,i",   po::value(&vr_params.interlaced_source),   "bool   false   Interlaced source [1], progressive[0]")
            ("deinterlace,d", po::value(&vr_params.deinterlace),        "bool   false   Deinterlace on [1], off [0]")
            ("yrange",        po::value(&vr_params.input_luma_range),   "int    220     Range of input luma (white-black+1)")
            ("yblack",        po::value(&vr_params.input_luma_blacklevel),   "int    16      Blacklevel of input luma")
            ("cblack",        po::value(&vr_params.input_chroma_blacklevel),   "int    16      Blacklevel of input chroma")
            ("out-range",     po::value(&vr_params.output_range),   "int    220     Range of R'G'B' output (white-black+1)")
            ("out-black",     po::value(&vr_params.output_blacklevel),   "int    16      Blacklevel of R'G'B' output")
            ("ymult",         po::value(&vr_params.luminance_mul),       "float  1.0     User luma multipler")
            ("cmult",         po::value(&vr_params.chrominance_mul),     "float  1.0     User chroma multiplier")
            ("yoffset2",      po::value(&vr_params.luminance_offset2),   "float  0.0     User luma offset2")
            ("coffset2",      po::value(&vr_params.chrominance_offset2), "float  0.0     User chroma offset2")
            ("matrixkr",      po::value(&userKr),           "float  0.2126  Colour Matrix Kr")
            ("matrixkg",      po::value(&userKg),           "float  0.7152  Colour Matrix Kg")
            ("matrixkb",      po::value(&userKb),           "float  0.0722  Colour Matrix Kb\n"
                                                              "               ITU-R BT709/BT1361, SMPTE274M/296M")
            ("sdmatrix,s",                                    "               As '-kr 0.299 -kg 0.587 -kb 0.114'\n"
                                                              "               ITU-R BT601/BT470, SMPTE170M/293M")
            ("fontfile",      po::value<string>(),            "string         TrueType font file for OSD")
            ("osdscale",      po::value(&vr_params.osd_scale),           "float  1.0     OSD size scaling factor")
            ("osdbackalpha",  po::value(&vr_params.osd_back_alpha),"float  0.7     Transparency for OSD background")
            ("osdtextalpha",  po::value(&vr_params.osd_text_alpha),"float  0.5     Transparency for OSD text")
            ("osdstate",      po::value(&vr_params.osd_bot),           "int    0       OSD initial state")
            ("filetype,t",    po::value<string>(),            "string         Force file type\n"
                                                              "               [i420|yv12|uyvy|v210|v216]")
            ("full,f",                                        "               Start in full screen mode")
            ("hidemouse",                                     "               Never show the mouse pointer")
            ("caption",       po::value<string>(),            "string         OSD Caption text")
            ("video",                                         "               Video file to play")
            ("help",                                          "               Show usage information");

        //file filename is a 'positional option' that we give the name 'video' to
		po::positional_options_description p;
		p.add("video", -1);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv). options(desc).positional(p).run(), vm);
		po::notify(vm);

		if (vm.count("help")) {
			usage();
			std::cout << desc << std::endl;
		}

		if (vm.count("sd")) {
			setSDTVMatrix();
		}

		if (vm.count("full")) {
			startFullScreen=true;
		}

		if (vm.count("quit")) {
			quitAtEnd=true;
		}

		if (vm.count("hidemouse")) {
			hideMouse=true;
		}

		if (vm.count("matrixkr") || vm.count("matrixkg") || vm.count("matrixkb"))
			setUserMatrix();

		if (vm.count("fontfile")) {
			string tmp = vm["fontfile"].as<string>();
			vr_params.font_file = tmp.data();

#ifdef WITH_OSD
			//check OSD font file exists
			QFileInfo fi(vr_params.font_file);
			if(fi.exists() == false) {
				printf("Cannot find OSD font file %s\n", vr_params.font_file.toLatin1().data());
				allParsed = false;
			}
			else {
				if(fi.isReadable() == false) {
					printf("Cannot read OSD font file %s\n", vr_params.font_file.toLatin1().data());
					allParsed = false;
				}
			}
#endif
		}

		if(vm.count("caption")) {
			string tmp = vm["caption"].as<string>();
			vr_params.caption = tmp.data();
		}

		QString known_extensions("i420 yv12 420p 422p 444p uyvy v216 v210 16p4 16p2 16p0");
#ifdef HAVE_DIRAC
		known_extensions.append(" drc");
#endif

		if (vm.count("filetype")) {
			string tmp = vm["filetype"].as<string>();
			fileType = tmp.data();

			if(known_extensions.contains(fileType.toLower(), Qt::CaseInsensitive))
			{
				forceFileType=true;
			}
			else {
				printf("Unknown file type %s\n", fileType.toLatin1().data());
				allParsed = false;
			}

		}

		if (vm.count("video") == 0) {
			printf("No file to play!\n");
			allParsed = false;
		}
		else {
			string tmp = vm["video"].as<string>();
			fileName = tmp.data();

			QFileInfo fi(fileName);

			if(fi.exists()) {
				if(forceFileType == false) {
					//file extension must be one we know about
					if(known_extensions.contains(fi.suffix().toLower(), Qt::CaseInsensitive) == false)
					{
						printf("Do not know how to play file with extension %s\n", fi.suffix().toLatin1().data());
						printf("Please specify file format with the -t flag\n");
						allParsed = false;
					}
				}
			}
			else {
				printf("File %s does not exist\n", fileName.toLatin1().data());
				allParsed = false;
			}
		}
	}
	catch(exception& e)
	{
		printf("Command line error : ");
		cout << e.what() << "\n";
		allParsed = false;
	}
}

void MainWindow::usage()
{
	printf("\nOpenGL accelerated YUV video player.");
	printf("\n");
	printf("\nUsage: progname -<flag1> [<flag1_val>] ... <input>");
	printf("\n");
	printf("\nSupported file formats:");
	printf("\n");
	printf("\n  .i420         4:2:0 YUV 8 bit planar");
	printf("\n  .yv12         4:2:0 YVU 8 bit planar");
	printf("\n  .420p         4:2:0 YUV 8 bit planar");
	printf("\n  .422p         4:2:2 YUV 8 bit planar");
	printf("\n  .444p         4:4:4 YUV 8 bit planar");
	printf("\n  .uyvy         4:2:2 YUV 8 bit packed");
	printf("\n  .v210         4:2:2 YUV 10 bit packed");
	printf("\n  .v216         4:2:2 YUV 16 bit packed");
	printf("\n  .16p0         4:2:0 YUV 16 bit planar");
	printf("\n  .16p2         4:2:2 YUV 16 bit planar");
	printf("\n  .16p4         4:4:4 YUV 16 bit planar");
	printf("\n");

	printf("\nKeypress               Action");
	printf("\n========               ======");
	printf("\no                      Toggle OSD state");
	printf("\nf                      Toggle full screen mode");
	printf("\nm                      Toggle colour matrix scaling");
	printf("\nEsc                    Return from full screen mode to windowed");
	printf("\na                      Toggle aspect ratio lock");
	printf("\nd                      Toggle deinterlacing of an interlaced source");
	printf("\nSpace                  Play/Pause");
	printf("\ns                      Stop");
	printf("\n1,2,3,4,5,6,7          Play forward at 1,2,5,10,20,50,100x");
	printf("\nCTRL + 1,2,3,4,5,6,7   Play backward at 1,2,5,10,20,50,100x");
	printf("\n>                      Jog one frame forward when paused");
	printf("\n<                      Jog one frame backward when paused");
	printf("\ny                      Toggle display of luminance  on/off");
	printf("\nc                      Toggle display of chrominance on/off");
	printf("\nh                      Switch to HDTV colour matrix kr=0.2126 kg=0.7152 kb=0.0722");
	printf("\nj                      Switch to SDTV colour matrix kr=0.2990 kg=0.5870 kb=0.1140");
	printf("\nk                      Switch to colour matrix kr, kg, kb from user flags");
	printf("\nq                      Quit");
	printf("\n");
	printf("\n");
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
	vr_params.matrix_Kr = 0.2126;
	vr_params.matrix_Kg = 0.7152;
	vr_params.matrix_Kb = 0.0722;
	vr_params.matrix_valid = false;
}

void MainWindow::setSDTVMatrix()
{
	vr_params.matrix_Kr = 0.299;
	vr_params.matrix_Kg = 0.587;
	vr_params.matrix_Kb = 0.114;
	vr_params.matrix_valid = false;
}

void MainWindow::setUserMatrix()
{
	vr_params.matrix_Kr = userKr;
	vr_params.matrix_Kg = userKg;
	vr_params.matrix_Kb = userKb;
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
	if (quitAtEnd==true)
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
