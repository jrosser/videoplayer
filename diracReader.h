/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: diracReader.h,v 1.1 2008-02-25 15:08:06 jrosser Exp $
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

#ifndef DIRACREADER_H_
#define DIRACREADER_H_

#include <QtGui>

#include <samples/IWriter.hpp>
#include "readerInterface.h"
#include "videoData.h"

class SomeWriter;

class DiracReader : public ReaderInterface, public QThread
{
public:	//from ReaderInterface
	DiracReader();
    virtual void getFrame(int frameNumber, VideoData *dst);    
    
public:	//specific to yuvReader
    void setFileName(const QString &fn);
    void run();
    void stop();
    
private:
    QString fileName;
    bool go;
    
    //frame
    QMutex frameMutex;
    unsigned char *frameData;
    int frameWidth;
    int frameHeight;
    
    
    //condition variable
    QWaitCondition bufferNotEmpty;
    QWaitCondition bufferNotFull;
    
    friend class SomeWriter;
};

#endif