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

#ifdef Q_OS_LINUX
#include "X11/Xlib.h"
#endif

#include <QApplication>

#include <string>
using namespace std;

#include <iostream>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include "GLvideo_params.h"

#include "config.h"

void
usage()
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
	printf("\ny                      Toggle display of luma on/off");
	printf("\nc                      Toggle display of chroma on/off");
	printf("\nh                      Switch to HDTV colour matrix kr=0.2126 kg=0.7152 kb=0.0722");
	printf("\nj                      Switch to SDTV colour matrix kr=0.2990 kg=0.5870 kb=0.1140");
	printf("\nk                      Switch to colour matrix kr, kg, kb from user flags");
	printf("\nq                      Quit");
	printf("\n");
	printf("\n");
}

bool
parseCommandLine(int argc, char **argv, GLvideo_params &vp, Qt_params& qt)
{
	bool allParsed = true; //default to command line parsing succeeding
	int cols = 81;

	if (getenv("COLUMNS"))
		cols = atoi(getenv("COLUMNS"));

	try {
		// Declare the supported options.
		po::options_description desc("Allowed options            type   default  description\n"
		                             "===============            ====   =======  ===========", cols);
		desc.add_options()
		    ("width,w",       po::value(&qt.videoWidth),         "int    1920    Width of video luma component")
		    ("height,h",      po::value(&qt.videoHeight),        "int    1080    Height of video luma component")
		    ("repeats,r",     po::value(&vp.frame_repeats),       "int    0       Frame is repeated r extra times")
		    ("loop,l",        po::value(&qt.looping),            "bool   1       Video file is played repeatedly")
		    ("quit,q",                                        "               Exit at end of video file")
		    ("interlace,i",   po::value(&vp.interlaced_source),   "bool   false   Interlaced source [1], progressive[0]")
		    ("deinterlace,d", po::value(&vp.deinterlace),        "bool   false   Deinterlace on [1], off [0]")
		    ("yrange",        po::value(&vp.input_luma_range),   "int    220     Range of input luma (white-black+1)")
		    ("yblack",        po::value(&vp.input_luma_blacklevel),   "int    16      Blacklevel of input luma")
		    ("cblack",        po::value(&vp.input_chroma_blacklevel),   "int    16      Blacklevel of input chroma")
		    ("out-range",     po::value(&vp.output_range),   "int    220     Range of R'G'B' output (white-black+1)")
		    ("out-black",     po::value(&vp.output_blacklevel),   "int    16      Blacklevel of R'G'B' output")
		    ("ymult",         po::value(&vp.luminance_mul),       "float  1.0     User luma multipler")
		    ("cmult",         po::value(&vp.chrominance_mul),     "float  1.0     User chroma multiplier")
		    ("yoffset2",      po::value(&vp.luminance_offset2),   "float  0.0     User luma offset2")
		    ("coffset2",      po::value(&vp.chrominance_offset2), "float  0.0     User chroma offset2")
		    ("matrixkr",      po::value(&vp.matrix_Kr),           "float  0.2126  Luma coefficient Kr")
		    ("matrixkg",      po::value(&vp.matrix_Kg),           "float  0.7152  Luma coefficient Kg")
		    ("matrixkb",      po::value(&vp.matrix_Kb),           "float  0.0722  Luma coefficient Kb\n"
		                                                      "               ITU-R BT709/BT1361, SMPTE274M/296M")
		    ("sdmatrix,s",                                    "               As '-kr 0.299 -kg 0.587 -kb 0.114'\n"
		                                                      "               ITU-R BT601/BT470, SMPTE170M/293M")
#if WITH_OSD
		    ("fontfile",      po::value<string>(),            "string         TrueType font file for OSD")
		    ("osdscale",      po::value(&vp.osd_scale),           "float  1.0     OSD size scaling factor")
		    ("osdbackalpha",  po::value(&vp.osd_back_alpha),"float  0.7     Transparency for OSD background")
		    ("osdtextalpha",  po::value(&vp.osd_text_alpha),"float  0.5     Transparency for OSD text")
		    ("osdstate",      po::value(&vp.osd_bot),           "int    0       OSD initial state")
#endif
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
			exit(0);
		}

		if (vm.count("sd")) {
			SetLumaCoeffsRec601(vp);
		}

		if (vm.count("full")) {
			qt.startFullScreen=true;
		}

		if (vm.count("quit")) {
			qt.quit_at_end=true;
		}

		if (vm.count("hidemouse")) {
			qt.hideMouse=true;
		}

		if (vm.count("matrixkr") || vm.count("matrixkg") || vm.count("matrixkb")) {
			vp.userKr = vp.matrix_Kr;
			vp.userKg = vp.matrix_Kg;
			vp.userKb = vp.matrix_Kb;

			if (vp.matrix_Kr + vp.matrix_Kg + vp.matrix_Kb < 1.0)
				printf("Warning, luma coefficients do not sum to one\n");
		}

		if (vm.count("fontfile")) {
			string tmp = vm["fontfile"].as<string>();
			vp.font_file = tmp.data();

#ifdef WITH_OSD
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
#endif
		}

		if(vm.count("caption")) {
			string tmp = vm["caption"].as<string>();
			vp.caption = tmp.data();
		}

		QString known_extensions("i420 yv12 420p 422p 444p uyvy v216 v210 16p4 16p2 16p0");
#ifdef HAVE_DIRAC
		known_extensions.append(" drc");
#endif

		if (vm.count("filetype")) {
			string tmp = vm["filetype"].as<string>();
			qt.fileType = tmp.data();

			if(known_extensions.contains(qt.fileType.toLower(), Qt::CaseInsensitive))
			{
				qt.forceFileType=true;
			}
			else {
				printf("Unknown file type %s\n", qt.fileType.toLatin1().data());
				allParsed = false;
			}

		}

		if (vm.count("video") == 0) {
			printf("No file to play!\n");
			allParsed = false;
		}
		else {
			string tmp = vm["video"].as<string>();
			qt.fileName = tmp.data();

			QFileInfo fi(qt.fileName);

			if(fi.exists()) {
				if(qt.forceFileType == false) {
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
				printf("File %s does not exist\n", qt.fileName.toLatin1().data());
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

	return allParsed;
}

int main(int argc, char **argv)
{
#ifdef Q_OS_LINUX
	XInitThreads();
#endif

	struct GLvideo_params vr_params;
	/* some defaults in the abscence of any settings */
	vr_params.frame_repeats = 0;
	vr_params.caption = "hello world";
	vr_params.osd_scale = 1.;
	vr_params.osd_back_alpha = 0.7;
	vr_params.osd_text_alpha = 0.5;
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
	vr_params.interlaced_source = false;
	vr_params.deinterlace = false;
	vr_params.matrix_valid = false;
	//vr_params.matrix_scaling = false;
	SetLumaCoeffsRec709(vr_params);
	vr_params.userKr = vr_params.matrix_Kr;
	vr_params.userKg = vr_params.matrix_Kg;
	vr_params.userKb = vr_params.matrix_Kb;
	vr_params.aspect_ratio_lock = true;
	vr_params.show_luma = true;
	vr_params.show_chroma = true;

	struct Qt_params qt_params;
	qt_params.startFullScreen = false;
	qt_params.fileName = "";
	qt_params.fileType = "";
	qt_params.forceFileType = false;
	qt_params.videoWidth = 1920;
	qt_params.videoHeight = 1080;

	qt_params.hideMouse = false;
	qt_params.looping = true;
	qt_params.quit_at_end = false;

	/* QApplication will munge argc/argv, needs to be called before
	 * parseCommandLine. Eg, useful for X11's -display :0 convention */
	QApplication app(argc, argv);

	//override settings with command line
	if (parseCommandLine(argc, argv, vr_params, qt_params) == false) {
		return -1;
	}

	MainWindow window(vr_params, qt_params);

	window.show();
	return app.exec();
}

