/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: main.cpp,v 1.9 2008-02-05 00:12:06 asuraparaju Exp $
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

#ifdef Q_OS_MACX
#include "agl_getproc.h"
#endif

#include <QApplication>

#include "GLvideo_params.h"

#include "config.h"

int main(int argc, char **argv)
{
#ifdef Q_OS_LINUX
    XInitThreads();
#endif

#ifdef Q_OS_MACX
    aglInitEntryPoints();
#endif

    QApplication app(argc, argv);

	struct GLvideo_params vr_params;
    /* some defaults in the abscence of any settings */
	vr_params.frame_repeats = 0;
	vr_params.caption = "hello world";
	vr_params.osd_scale = 1.0;
	vr_params.osd_back_alpha = 0.7;
	vr_params.osd_text_alpha = 0.5;
    vr_params.font_file = DEFAULT_FONTFILE;
    vr_params.luminance_offset1 = -0.0625;
    vr_params.chrominance_offset1 = -0.5;
    vr_params.luminance_mul = 1.0;
    vr_params.chrominance_mul = 1.0;
    vr_params.luminance_offset2 = 0.0;
    vr_params.chrominance_offset2 = 0.0;
    vr_params.interlaced_source = false;
    vr_params.deinterlace = false;
    vr_params.matrix_scaling = false;
    vr_params.matrix_Kr = 0.2126;
    vr_params.matrix_Kg = 0.7152;
    vr_params.matrix_Kb = 0.0722;

    MainWindow window(argc, argv, vr_params);

    if(window.allParsed) {
        //only run application if command line parameters all OK
        window.show();
        return app.exec();
    }
    else
    {
        return -1;
    }
}

