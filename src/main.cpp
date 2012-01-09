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

#ifdef WITH_CONFIG_H
# include <config.h>
#endif

#include "mainwindow.h"

#ifdef Q_OS_LINUX
#include "X11/Xlib.h"
#endif

#include <QApplication>

#include <string>
using namespace std;

#include <iostream>
#include "program_options_lite.h"

#include "readerInterface.h"
#include "yuvReader.h"
#include "yuvReaderMmap.h"
#include "playlistReader.h"

#include "videoTransportQT.h"
#ifdef Q_OS_LINUX
#include "QShuttlePro.h"
#endif

#ifdef Q_OS_UNIX
#include "QConsoleInput.h"
#endif

#include "GLvideo_params.h"
#include "GLfrontend_old.h"

#include "version.h"
#include "fileDialog.h"

struct Transport_params {
	bool looping;
	std::list<QString> file_names;
	QString fileType;
	bool quit_at_end;
	bool forceFileType;
	int videoWidth;
	int videoHeight;
	int frame_repeats;
	double vdu_fps;
	double src_fps;
	/* @interlaced_source@ modifies behaviour when repeating
	 * frames (paused) and playing backwards (field reversal) */
	bool interlaced_source;
	int read_ahead;
	int lru_cache;
};

static void
version()
{
	printf("\nOpenGL accelerated YUV video player.");
	printf("\n  Version: %s", BUILD_VERSION);
	printf("\nCopyright (c) 2007-2010 BBC Research & Development.");
	printf("\nCopyright (c) The Authors.");
	printf("\n");
}

void
usage()
{
	version();
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
	printf("\n?                      Toggle display of statistics on/off");
	printf("\nf                      Toggle full screen mode");
	printf("\nm                      Switch to output levels from user flags out-range and out-black");
	printf("\nn                      Switch to video output levels, out-range=220 out-black=16");
	printf("\nb                      Switch to computer output levels, out-range=256, out-black=0");
	printf("\nEsc                    Return from full screen mode to windowed");
	printf("\na                      Toggle aspect ratio lock");
	printf("\ni                      Toggle deinterlacing of an interlaced source");
	printf("\nSpace                  Play/Pause");
	printf("\ns                      Stop");
	printf("\n1,2,3,4,5,6,7          Play forward at 1,2,5,10,20,50,100x");
	printf("\nCTRL + 1,2,3,4,5,6,7   Play backward at 1,2,5,10,20,50,100x");
	printf("\n>                      Jog one frame forward when paused");
	printf("\n<                      Jog one frame backward when paused");
	printf("\ny                      Toggle display of luma on/off");
	printf("\nc                      Toggle display of chroma on/off");
	printf("\nh                      Switch to HDTV colour matrix kr=0.2126 kg=0.7152 kb=0.0722");
	printf("\nj                      Switch to SDTV colour matrix kr=0.2990 kg=0.5870 kb=0.1140");
	printf("\nk                      Switch to colour matrix kr, kg, kb from user flags");
	printf("\nq                      Quit");
	printf("\n");
	printf("\n");
}

/* string parsing is specialized -- copy the whole string, not just the
 * first word */
namespace df { namespace program_options_lite {
	template<>
	inline void
	Option<QString>::parse(const std::string& arg)
	{
		opt_storage = arg.c_str();
	}
}}

bool
parseCommandLine(int argc, char **argv, GLvideo_params& vp, Transport_params& tp, Qt_params& qt)
{
	bool allParsed = true; //default to command line parsing succeeding
	int cols = 81;

	if (getenv("COLUMNS"))
		cols = atoi(getenv("COLUMNS"));

	bool opt_sdmatrix, opt_help, opt_version;
	namespace po = df::program_options_lite;
	po::Options opts;
	opts.addOptions()
		("width,w",       tp.videoWidth,                  "Width of video luma component")
		("height,h",      tp.videoHeight,                 "Height of video luma component")
		("vdu-fps",       tp.vdu_fps,                     "Display's frame rate")
		("fps",           tp.src_fps,                     "Source frame rate")
		("repeats,r",     tp.frame_repeats,               "Frame is repeated r extra times")
		("loop,l",        tp.looping,                     "Number of times to loop video (1=inf)")
		("quit,q",        tp.quit_at_end,                 "Exit at end of video file (implies --loop=0)")
		("interlace,i",   tp.interlaced_source,           "Source is interlaced")
		("deinterlace,d", vp.deinterlace,                 "Enable Deinterlacer (requires -i)")
		("yrange",        vp.input_luma_range,            "Range of input luma (white-black+1)")
		("yblack",        vp.input_luma_blacklevel,       "Blacklevel of input luma")
		("cblack",        vp.input_chroma_blacklevel,     "Blacklevel of input chroma")
		("out-range",     vp.output_range,                "Range of R'G'B' output (white-black+1)")
		("out-black",     vp.output_blacklevel,           "Blacklevel of R'G'B' output")
		("ymult",         vp.luminance_mul,               "User luma multipler")
		("cmult",         vp.chrominance_mul,             "User chroma multiplier")
		("yoffset2",      vp.luminance_offset2,           "User luma offset2")
		("coffset2",      vp.chrominance_offset2,         "User chroma offset2")
		("matrixkr",      vp.matrix_Kr,                   "Luma coefficient Kr")
		("matrixkg",      vp.matrix_Kg,                   "Luma coefficient Kg")
		("matrixkb",      vp.matrix_Kb,                   "Luma coefficient Kb\nITU-R BT709/BT1361, SMPTE274M/296M")
		("sdmatrix,s",    opt_sdmatrix, false,            "As '-kr 0.299 -kg 0.587 -kb 0.114'\nITU-R BT601/BT470, SMPTE170M/293M")
		("vt.readahead",  tp.read_ahead,                  "Video transport read ahead list length")
		("vt.lrucache",   tp.lru_cache,                   "Video transport least recently used cache length")
#if WITH_OSD
		("fontfile",      vp.font_file,                   "TrueType font file for OSD")
		("osdscale",      vp.osd_scale,                   "OSD size scaling factor")
		("osdbackalpha",  vp.osd_back_alpha,              "Transparency for OSD background")
		("osdtextalpha",  vp.osd_text_alpha,              "Transparency for OSD text")
		("osdstate",      (int&)vp.osd_bot,               "OSD initial state")
		("caption",       vp.caption,                     "OSD Caption text")
#endif
		("filetype,t",    tp.fileType,                    "Force file type\n[i420|yv12|uyvy|v210|v216]")
		("full,f",        qt.start_fullscreen,            "Start in full screen mode")
		("hidemouse",     qt.hidemouse,                   "Never show the mouse pointer")
		("help",          opt_help, false,                "Show usage information")
		("version,V",     opt_version, false,             "Show version information");

	po::setDefaults(opts);
	const list<const char*>& argv_unhandled = po::scanArgv(opts, argc, (const char**) argv);

	if (opt_help) {
		usage();
		po::doHelp(cout, opts, cols);
		exit(0);
	}

	if (opt_version) {
		version();
		exit(0);
	}

	if (opt_sdmatrix) {
		SetLumaCoeffsRec601(vp);
	}

	if (vp.matrix_Kr + vp.matrix_Kg + vp.matrix_Kb < 1.0) {
		printf("Warning, luma coefficients do not sum to unity\n");
	}

#ifdef WITH_OSD
	if (1) {
		/* unconditionally verify this, even if the default */

		//check OSD font file exists
		QFileInfo fi(vp.font_file);
		if(fi.exists() == false) {
			printf("Cannot find OSD font file %s\n", vp.font_file.toLatin1().data());
			allParsed = false;
		}
		else {
			if(fi.isReadable() == false) {
				printf("Cannot read OSD font file %s\n", vp.font_file.toLatin1().data());
				allParsed = false;
			}
		}
	}
#endif

	QString known_extensions("i420 yv12 420p 422p 444p uyvy v216 v210 16p4 16p2 16p0 plst");

	if (!tp.fileType.isEmpty()) {
		if(known_extensions.contains(tp.fileType.toLower(), Qt::CaseInsensitive))
			tp.forceFileType=true;
		else {
			printf("Unknown file type %s\n", tp.fileType.toLatin1().data());
			allParsed = false;
		}
	}

	for (list<const char*>::const_iterator it = argv_unhandled.begin(); it != argv_unhandled.end(); it++) {
		QString file_name(*it);
		QFileInfo fi(file_name);

		if(fi.exists()) {
			if(tp.forceFileType == false) {
				//file extension must be one we know about
				if(known_extensions.contains(fi.suffix().toLower(), Qt::CaseInsensitive) == false) {
					printf("Do not know how to play file with extension %s\n", fi.suffix().toLatin1().data());
					printf("Please specify file format with the -t flag\n");
					allParsed = false;
					break;
				}
				else {
					//the file type has not been forced, and it is an allowed file type
					tp.fileType = fi.suffix().toLower();
				}
			}
		}
		else {
			printf("File %s does not exist\n", *it);
			allParsed = false;
			break;
		}
		tp.file_names.push_back(file_name);
	}

	return allParsed;
}

int main(int argc, char **argv)
{
#ifdef Q_OS_LINUX
	XInitThreads();
#endif

	struct GLvideo_params vr_params;
	/* some defaults in the abscence of any settings */
	vr_params.caption = "hello world";
	vr_params.osd_scale = 1.;
	vr_params.osd_back_alpha = 0.7f;
	vr_params.osd_text_alpha = 0.5f;
	vr_params.osd_perf = false;
	vr_params.osd_bot = OSD_NONE;
	vr_params.osd_valid = false;
	vr_params.font_file = DEFAULT_FONTFILE;
	vr_params.input_luma_range = 220;
	vr_params.input_luma_blacklevel = 16;
	vr_params.input_chroma_blacklevel = 128;
	vr_params.output_blacklevel = 16;
	vr_params.output_range = 220;
	vr_params.luminance_mul = 1.;
	vr_params.chrominance_mul = 1.;
	vr_params.luminance_offset2 = 0.;
	vr_params.chrominance_offset2 = 0.;
	vr_params.deinterlace = false;
	vr_params.matrix_valid = false;
	SetLumaCoeffsRec709(vr_params);
	vr_params.aspect_ratio_lock = true;
	vr_params.show_luma = true;
	vr_params.show_chroma = true;

	struct Transport_params t_params;
	t_params.looping = true;
	t_params.quit_at_end = false;
	t_params.forceFileType = false;
	t_params.videoWidth = 1920;
	t_params.videoHeight = 1080;
	t_params.interlaced_source = false;
	t_params.frame_repeats = 1;
	/* in case these aren't specified, assume a 1:1 mapping */
	t_params.vdu_fps = 1.;
	t_params.src_fps = 1.;
	t_params.read_ahead = 16;
	t_params.lru_cache = 16;

	struct Qt_params qt_params;
	qt_params.hidemouse = false;
	qt_params.start_fullscreen = false;

	/* QApplication will munge argc/argv, needs to be called before
	 * parseCommandLine. Eg, useful for X11's -display :0 convention */
	QApplication app(argc, argv);

	//override settings with command line
	if (parseCommandLine(argc, argv, vr_params, t_params, qt_params) == false) {
		return -1;
	}

	if (t_params.file_names.empty()) {
		/* user did not provide a file to play.  Prompt the user with an
		 * OpenFile fialog for a more interactive choice */
		VideoFileDialog dlg(0);

		QString file_name;
		if (dlg.exec(file_name,
		             t_params.fileType,
		             t_params.videoWidth,
		             t_params.videoHeight,
		             t_params.interlaced_source) != QDialog::Accepted)
		{
			return -1; //cancelled dialog: exit program
		}

		t_params.forceFileType = 1;
		t_params.file_names.push_back(file_name);
	}

	//object that generates frames to be inserted into the frame queue
	list<ReaderInterface*> readers;
	for (list<QString>::iterator it = t_params.file_names.begin(); it != t_params.file_names.end(); it++) {
		if(t_params.fileType.toLower() == "plst") {
			PlayListReader *r = new PlayListReader();
			r->setFileName(*it);
			readers.push_back(r);
			continue;
		}
#if 0
		YUVReaderMmap *r = new YUVReaderMmap();
#else
		YUVReader *r = new YUVReader();
#endif
		//YUV reader parameters
		r->setVideoWidth(t_params.videoWidth);
		r->setVideoHeight(t_params.videoHeight);
		r->setForceFileType(t_params.forceFileType);
		r->setFileType(t_params.fileType);
		r->setFileName(*it);
		r->setInterlacedSource(t_params.interlaced_source);
		r->setFPS(t_params.src_fps);

		readers.push_back(r);
	}


	//object controlling the video playback 'transport'
	VideoTransportQT* vt = new VideoTransportQT(readers, t_params.read_ahead, t_params.lru_cache);
	//vt->setRepeats(t_params.frame_repeats);
	vt->setVDUfps(t_params.vdu_fps);
	vt->setSteppingIgnoreInterlace(!vr_params.deinterlace);

	GLfrontend_old gl_frontend(vr_params, vt);

	MainWindow* window = new MainWindow(vr_params, qt_params, vt, &gl_frontend);

#ifdef Q_OS_LINUX
	//shuttlePro jog dial - linux only native support at the moment
	QShuttlePro* shuttle = new QShuttlePro();

	//shuttlepro jog wheel
	QObject::connect(shuttle, SIGNAL(jogForward()), vt, SLOT(transportJogFwd()));
	QObject::connect(shuttle, SIGNAL(jogBackward()), vt, SLOT(transportJogRev()));

	//shuttlepro shuttle dial
	QObject::connect(shuttle, SIGNAL(shuttleRight7()), vt, SLOT(transportFwd100()));
	QObject::connect(shuttle, SIGNAL(shuttleRight6()), vt, SLOT(transportFwd50()));
	QObject::connect(shuttle, SIGNAL(shuttleRight5()), vt, SLOT(transportFwd20()));
	QObject::connect(shuttle, SIGNAL(shuttleRight4()), vt, SLOT(transportFwd10()));
	QObject::connect(shuttle, SIGNAL(shuttleRight3()), vt, SLOT(transportFwd5()));
	QObject::connect(shuttle, SIGNAL(shuttleRight2()), vt, SLOT(transportFwd2()));
	QObject::connect(shuttle, SIGNAL(shuttleRight1()), vt, SLOT(transportFwd1()));

	QObject::connect(shuttle, SIGNAL(shuttleLeft7()), vt, SLOT(transportRev100()));
	QObject::connect(shuttle, SIGNAL(shuttleLeft6()), vt, SLOT(transportRev50()));
	QObject::connect(shuttle, SIGNAL(shuttleLeft5()), vt, SLOT(transportRev20()));
	QObject::connect(shuttle, SIGNAL(shuttleLeft4()), vt, SLOT(transportRev10()));
	QObject::connect(shuttle, SIGNAL(shuttleLeft3()), vt, SLOT(transportRev5()));
	QObject::connect(shuttle, SIGNAL(shuttleLeft2()), vt, SLOT(transportRev2()));
	QObject::connect(shuttle, SIGNAL(shuttleLeft1()), vt, SLOT(transportRev1()));
	QObject::connect(shuttle, SIGNAL(shuttleCenter()), vt, SLOT(transportStop()));

	//shuttlepro buttons
	QObject::connect(shuttle, SIGNAL(key267Pressed()), vt, SLOT(transportPlayPause()));
	QObject::connect(shuttle, SIGNAL(key265Pressed()), vt, SLOT(transportFwd1()));
	QObject::connect(shuttle, SIGNAL(key259Pressed()), window, SLOT(toggleFullScreen()));
	QObject::connect(shuttle, SIGNAL(key256Pressed()), window, SLOT(toggleOSD()));
	QObject::connect(shuttle, SIGNAL(key257Pressed()), window, SLOT(toggleAspectLock()));

	//this is what the IngexPlayer also does
	//key 269, press=previous mark, hold=start of file
	//key 270, press=next mark, hold=end of file
	//key 257, cycle displayed timecode type
	//key 258, press=lock controls, hold=unlock controls
#endif

#ifdef Q_OS_UNIX
	QConsoleInput tty(window);
#endif

	if (t_params.quit_at_end) {
		QObject::connect(vt, SIGNAL(endOfFile()), &app, SLOT(quit()));
		t_params.looping = false;
	}
	vt->setLooping(t_params.looping);

	window->show();
	/* app.exec will run until the mainwindow terminates */
	app.exec();

#ifdef Q_OS_LINUX
	if(shuttle) {
		shuttle->stop();
		shuttle->wait();
	}
#endif

	delete window;

	return 0;
}
