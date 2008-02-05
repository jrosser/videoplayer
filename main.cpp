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

int main(int argc, char **argv)
{
#ifdef Q_OS_LINUX
    XInitThreads();
#endif

#ifdef Q_OS_MACX
    aglInitEntryPoints();
#endif

    QApplication app(argc, argv);

    MainWindow window(argc, argv);

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

