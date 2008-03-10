/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: mainwindow.cpp,v 1.48 2008-03-10 11:47:41 jrosser Exp $
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
#include "yuvReader.h"

#ifdef HAVE_DIRAC
#include "diracReader.h"
#endif

#include <QtGui>

#include <iostream>
#include <boost/program_options.hpp>
namespace po = boost::program_options;
using namespace std;

#include "agl_getproc.h"

MainWindow::MainWindow(int argc, char **argv)
{
    QPalette p = palette();
    p.setColor(QPalette::Window, Qt::black);
    setPalette(p);

    //some defaults in the abscence of any settings
    videoWidth = 1920;
    videoHeight = 1080;
    frameRepeats = 0;
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
    matrixScaling = false;
    fileType = "";
    matrixKr = 0.2126;
    matrixKg = 0.7152;
    matrixKb = 0.0722;
    startFullScreen = false;
    osdScale = 1.0;
    osdBackTransparency = 0.7;
    osdTextTransparency = 0.5;
    looping = true;
    quitAtEnd = false;
    hideMouse = false;

#ifdef Q_OS_UNIX
    fontFile = "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans-Bold.ttf";
#endif

#ifdef Q_OS_MACX
    fontFile = "/Library/Fonts/GillSans.dfont";
#endif

    //override settings with command line
    parseCommandLine(argc, argv);

    if(allParsed == false)
        return;

    if(hideMouse) {
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
    
    //central widget is the threaded openGL video widget
    //which pulls video from the videoRead
    //and gets stats for the OSD from the readThread
    glvideo_mt = new GLvideo_mt(videoTransport, frameQueue);

    setCentralWidget(glvideo_mt);

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
    connect(shuttle, SIGNAL(key256Pressed()), glvideo_mt, SLOT(toggleOSD()));
    connect(shuttle, SIGNAL(key257Pressed()), glvideo_mt, SLOT(toggleAspectLock()));

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
    glvideo_mt->setFrameRepeats(frameRepeats);
    glvideo_mt->setFontFile(fontFile);
    glvideo_mt->setLuminanceOffset1(luminanceOffset1);
    glvideo_mt->setChrominanceOffset1(chrominanceOffset1);
    glvideo_mt->setLuminanceMultiplier(luminanceMul);
    glvideo_mt->setChrominanceMultiplier(chrominanceMul);
    glvideo_mt->setLuminanceOffset2(luminanceOffset2);
    glvideo_mt->setChrominanceOffset2(chrominanceOffset2);
    glvideo_mt->setInterlacedSource(interlacedSource);
    glvideo_mt->setDeinterlace(deinterlace);
    glvideo_mt->setMatrixScaling(matrixScaling);
    glvideo_mt->setMatrix(matrixKr, matrixKg, matrixKb);
    glvideo_mt->setCaption(caption);
    glvideo_mt->setOsdScale(osdScale);
    glvideo_mt->setOsdState(osdState);
    glvideo_mt->setOsdTextTransparency(osdTextTransparency);
    glvideo_mt->setOsdBackTransparency(osdBackTransparency);
    glvideo_mt->setAlwaysHideMouse(hideMouse);

    
    frameQueue->start();
}

void MainWindow::parseCommandLine(int argc, char **argv)
{
    allParsed = true;    //default to command line parsing succeeding

    try {

        // Declare the supported options.
        po::options_description desc("Allowed options            type   default  description\n"
                                     "===============            ====   =======  ===========");
        desc.add_options()
            ("width,w",       po::value(&videoWidth),         "int    1920    Width of video luminance component")
            ("height,h",      po::value(&videoHeight),        "int    1080    Height of video luminance component")
            ("repeats,r",     po::value(&frameRepeats),       "int    0       Frame is repeated r extra times")
            ("loop,l",        po::value(&looping),            "bool   1       Video file is played repeatedly")
            ("quit,q",                                        "               Exit at end of video file")
            ("interlace,i",   po::value(&interlacedSource),   "bool   false   Interlaced source [1], progressive[0]")
            ("deinterlace,d", po::value(&deinterlace),        "bool   false   Deinterlace on [1], off [0]")
            ("yoffset",       po::value(&luminanceOffset1),   "float -0.0625  Luminance data values offset")
            ("coffset",       po::value(&chrominanceOffset1), "float -0.5     Chrominance data values offset")
            ("ymult",         po::value(&luminanceMul),       "float  1.0     Luminance multipler")
            ("cmult",         po::value(&chrominanceMul),     "float  1.0     Chrominance multiplier")
            ("yoffset2",      po::value(&luminanceOffset2),   "float  0.0     Multiplied Luminance offset 2")
            ("coffset2",      po::value(&chrominanceOffset2), "float  0.0     Multiplied Chrominance offset 2")
            ("mscale,m",      po::value(&matrixScaling),      "bool   0       Matrix scale [0] Y*1.0, C*1.0,\n"
                                                              "               [1] Y*(255/219) C*(255/224)")
            ("matrixkr",      po::value(&matrixKr),           "float  0.2126  Colour Matrix Kr")
            ("matrixkg",      po::value(&matrixKg),           "float  0.7152  Colour Matrix Kg")
            ("matrixkb",      po::value(&matrixKb),           "float  0.0722  Colour Matrix Kb\n"
                                                              "               ITU-R BT709/BT1361, SMPTE274M/296M")
            ("sdmatrix,s",                                    "               As '-kr 0.299 -kg 0.587 -kb 0.114'\n"
                                                              "               ITU-R BT601/BT470, SMPTE170M/293M")
            ("fontfile",      po::value<string>(),            "string         TrueType font file for OSD")
            ("osdscale",      po::value(&osdScale),           "float  1.0     OSD size scaling factor")
            ("osdbackalpha",  po::value(&osdBackTransparency),"float  0.7     Transparency for OSD background")
            ("osdtextalpha",  po::value(&osdTextTransparency),"float  0.5     Transparency for OSD text")
            ("osdstate",      po::value(&osdState),           "int    0       OSD initial state")
            ("filetype,t",    po::value<string>(),            "string         Force file type\n"
                                                              "               [i420|yv12|uyvy|v210|v216]")
            ("full,f",                                        "               Start in full screen mode")
            ("hidemouse",                                     "               Never show the mouse pointer")
            ("caption",          po::value<string>(),         "string         OSD Caption text")
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

        if (vm.count("fontfile")) {
            string tmp = vm["fontfile"].as<string>();
            fontFile = tmp.data();

#ifdef HAVE_FTGL
            //check OSD font file exists
            QFileInfo fi(fontFile);
            if(fi.exists() == false) {
                printf("Cannot find OSD font file %s\n", fontFile.toLatin1().data());
                allParsed = false;
            }
            else {
                if(fi.isReadable() == false) {
                    printf("Cannot read OSD font file %s\n", fontFile.toLatin1().data());
                    allParsed = false;
                }
            }
#endif
        }

        if (vm.count("filetype")) {
            string tmp = vm["filetype"].as<string>();
            fileType = tmp.data();

            if((fileType.toLower() == "i420") ||
               (fileType.toLower() == "yv12") ||
               (fileType.toLower() == "420p") ||
               (fileType.toLower() == "422p") ||
               (fileType.toLower() == "444p") ||                                      
               (fileType.toLower() == "uyvy") ||
               (fileType.toLower() == "v216") ||
               (fileType.toLower() == "v210") ||
               (fileType.toLower() == "16p4") ||
               (fileType.toLower() == "16p2") ||
               (fileType.toLower() == "16p0") ||               
               (fileType.toLower() == "drc")) {

               forceFileType=true;
            }
            else {
                printf("Unknown file type %s\n", fileType.toLatin1().data());
                allParsed = false;
            }

        }

        if(vm.count("caption")) {
            string tmp = vm["caption"].as<string>();
            caption = tmp.data();
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
                    if((fi.suffix().toLower() == "i420") ||
                       (fi.suffix().toLower() == "yv12") ||
                       (fi.suffix().toLower() == "420p") ||
                       (fi.suffix().toLower() == "422p") ||
                       (fi.suffix().toLower() == "444p") ||                       
                       (fi.suffix().toLower() == "uyvy") ||
                       (fi.suffix().toLower() == "v216") ||
                       (fi.suffix().toLower() == "v210") ||
                       (fi.suffix().toLower() == "16p4") ||
                       (fi.suffix().toLower() == "16p2") ||
                       (fi.suffix().toLower() == "16p0") ||                       
                       (fi.suffix().toLower() == "drc")) {

                    }
                    else {
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
    quitAct = new QAction("Quit", this);
    quitAct->setShortcut(tr("Q"));
    addAction(quitAct);
    connect(quitAct, SIGNAL(triggered()), this, SLOT(quit()));

    setUserMatrixAct = new QAction("Set user Matrix", this);
    setUserMatrixAct->setShortcut(tr("k"));
    addAction(setUserMatrixAct);
    connect(setUserMatrixAct, SIGNAL(triggered()), this, SLOT(setUserMatrix()));

    setHDTVMatrixAct = new QAction("Set HDTV Matrix", this);
    setHDTVMatrixAct->setShortcut(tr("h"));
    addAction(setHDTVMatrixAct);
    connect(setHDTVMatrixAct, SIGNAL(triggered()), this, SLOT(setHDTVMatrix()));

    setSDTVMatrixAct = new QAction("Set SDTV Matrix", this);
    setSDTVMatrixAct->setShortcut(tr("j"));
    addAction(setSDTVMatrixAct);
    connect(setSDTVMatrixAct, SIGNAL(triggered()), this, SLOT(setSDTVMatrix()));

    toggleMatrixScalingAct = new QAction("Toggle Colour Matrix Scaling", this);
    toggleMatrixScalingAct ->setShortcut(tr("m"));
    addAction(toggleMatrixScalingAct);
    connect(toggleMatrixScalingAct, SIGNAL(triggered()), glvideo_mt, SLOT(toggleMatrixScaling()));

    toggleDeinterlaceAct = new QAction("Toggle Deinterlacing", this);
    toggleDeinterlaceAct->setShortcut(tr("i"));
    addAction(toggleDeinterlaceAct);
    connect(toggleDeinterlaceAct, SIGNAL(triggered()), glvideo_mt, SLOT(toggleDeinterlace()));

    toggleOSDAct = new QAction("Toggle OSD", this);
    toggleOSDAct->setShortcut(tr("o"));
    addAction(toggleOSDAct);
    connect(toggleOSDAct, SIGNAL(triggered()), glvideo_mt, SLOT(toggleOSD()));

    togglePerfAct = new QAction("Toggle Performance", this);
    togglePerfAct->setShortcut(tr("?"));
    addAction(togglePerfAct);
    connect(togglePerfAct, SIGNAL(triggered()), glvideo_mt, SLOT(togglePerf()));

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
    connect(transportFwd100Act, SIGNAL(triggered()), videoTransport, SLOT(transportFwd100()));

    transportFwd50Act = new QAction("Forward 50x", this);
    transportFwd50Act->setShortcut(tr("6"));
    addAction(transportFwd50Act);
    connect(transportFwd50Act, SIGNAL(triggered()), videoTransport, SLOT(transportFwd50()));

    transportFwd20Act = new QAction("Forward 20x", this);
    transportFwd20Act->setShortcut(tr("5"));
    addAction(transportFwd20Act);
    connect(transportFwd20Act, SIGNAL(triggered()), videoTransport, SLOT(transportFwd20()));

    transportFwd10Act = new QAction("Forward 10x", this);
    transportFwd10Act->setShortcut(tr("4"));
    addAction(transportFwd10Act);
    connect(transportFwd10Act, SIGNAL(triggered()), videoTransport, SLOT(transportFwd10()));

    transportFwd5Act = new QAction("Forward 5x", this);
    transportFwd5Act->setShortcut(tr("3"));
    addAction(transportFwd5Act);
    connect(transportFwd5Act, SIGNAL(triggered()), videoTransport, SLOT(transportFwd5()));

    transportFwd2Act = new QAction("Forward 2x", this);
    transportFwd2Act->setShortcut(tr("2"));
    addAction(transportFwd2Act);
    connect(transportFwd2Act, SIGNAL(triggered()), videoTransport, SLOT(transportFwd2()));

    transportFwd1Act = new QAction("Forward 1x", this);
    transportFwd1Act->setShortcut(tr("1"));
    addAction(transportFwd1Act);
    connect(transportFwd1Act, SIGNAL(triggered()), videoTransport, SLOT(transportFwd1()));

    transportStopAct = new QAction("Stop", this);
    transportStopAct->setShortcut(tr("s"));
    addAction(transportStopAct);
    connect(transportStopAct, SIGNAL(triggered()), videoTransport, SLOT(transportStop()));

    transportRev1Act = new QAction("Reverse 1x", this);
    transportRev1Act->setShortcut(tr("Ctrl+1"));
    addAction(transportRev1Act);
    connect(transportRev1Act, SIGNAL(triggered()), videoTransport, SLOT(transportRev1()));

    transportRev2Act = new QAction("Reverse 2x", this);
    transportRev2Act->setShortcut(tr("Ctrl+2"));
    addAction(transportRev2Act);
    connect(transportRev2Act, SIGNAL(triggered()), videoTransport, SLOT(transportRev2()));

    transportRev5Act = new QAction("Reverse 5x", this);
    transportRev5Act->setShortcut(tr("Ctrl+3"));
    addAction(transportRev5Act);
    connect(transportRev5Act, SIGNAL(triggered()), videoTransport, SLOT(transportRev5()));

    transportRev10Act = new QAction("Reverse 10x", this);
    transportRev10Act->setShortcut(tr("Ctrl+4"));
    addAction(transportRev10Act);
    connect(transportRev10Act, SIGNAL(triggered()), videoTransport, SLOT(transportRev10()));

    transportRev20Act = new QAction("Reverse 20x", this);
    transportRev20Act->setShortcut(tr("Ctrl+5"));
    addAction(transportRev20Act);
    connect(transportRev20Act, SIGNAL(triggered()), videoTransport, SLOT(transportRev20()));

    transportRev50Act = new QAction("Reverse 50x", this);
    transportRev50Act->setShortcut(tr("Ctrl+6"));
    addAction(transportFwd100Act);
    connect(transportFwd100Act, SIGNAL(triggered()), videoTransport, SLOT(transportFwd100()));

    transportRev100Act = new QAction("Reverse 100x", this);
    transportRev100Act->setShortcut(tr("Ctrl+7"));
    addAction(transportRev100Act);
    connect(transportRev100Act, SIGNAL(triggered()), videoTransport, SLOT(transportRev100()));

    transportPlayPauseAct = new QAction("Play/Pause", this);
    transportPlayPauseAct->setShortcut(tr("Space"));
    addAction(transportPlayPauseAct);
    connect(transportPlayPauseAct, SIGNAL(triggered()), videoTransport, SLOT(transportPlayPause()));

    transportJogFwdAct = new QAction("Jog Forward", this);
    transportJogFwdAct->setShortcut(tr("."));
    addAction(transportJogFwdAct);
    connect(transportJogFwdAct, SIGNAL(triggered()), videoTransport, SLOT(transportJogFwd()));

    transportJogRevAct = new QAction("Jog Reverse", this);
    transportJogRevAct->setShortcut(tr(","));
    addAction(transportJogRevAct);
    connect(transportJogRevAct, SIGNAL(triggered()), videoTransport, SLOT(transportJogRev()));
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

void MainWindow::setHDTVMatrix()
{
    glvideo_mt->setMatrix(0.2126, 0.7152, 0.0722);
}

void MainWindow::setSDTVMatrix()
{
    glvideo_mt->setMatrix(0.299, 0.587, 0.114);
}

void MainWindow::setUserMatrix()
{
    glvideo_mt->setMatrix(matrixKr, matrixKg, matrixKb);
}

void MainWindow::endOfFile()
{
    if(quitAtEnd==true)
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

#ifdef Q_OS_MACX
    aglDellocEntryPoints();
#endif

    qApp->quit();
}
