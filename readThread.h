/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: readThread.h,v 1.7 2008-02-05 00:12:06 asuraparaju Exp $
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

#ifndef READTHREAD_RT_H_
#define READTHREAD_RT_H_

#include <QtGui>

#include "videoData.h"

class VideoRead;

class ReadThread : public QThread
{
public:
    ReadThread(VideoRead &vr);
    void run();
    void stop();

private:
    void addFutureFrames();
    void addPastFrames();

    bool m_doReading;
    VideoRead &vr;   //parent

    //info about the lists of frames
    int numFutureFrames;
    int numPastFrames;

    //the extents of the sequence
    int firstFrame;
    int lastFrame;

    //playback speed
    int speed;
    int lastSpeed;

    //information about the video data
    VideoData::DataFmt dataFormat;
    int videoWidth;
    int videoHeight;

    //video file
    int fd;

    //bandwidth counter
    int bandwidth_count;

    //a list of used frames for recycling, so that we don't need to re-allocate storage
    QList<VideoData *> usedFrames;

};

#endif
