/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: frameQueue.h,v 1.1 2008-02-25 15:08:05 jrosser Exp $
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

#ifndef FRAMEQUEUE_H_
#define FRAMEQUEUE_H_

#include <QtGui>

#include "readerInterface.h"
#include "videoData.h"

class VideoRead;

class FrameQueue : public QThread
{
public:
    FrameQueue();
    
    void run();
    void stop();
    void wake();
    
    void setForceFileType(bool f);
    void setFileType(const QString &t);
    void setVideoWidth(int w);
    void setVideoHeight(int h);
    void setFileName(const QString &fn);
    void setReader(ReaderInterface *r) { reader = r; }
    
    VideoData *getNextFrame(int speed, int direction);
    int getFutureQueueLen(void);
    int getPastQueueLen(void);
    int getIOLoad(void);
    int getIOBandwidth(void);
    
private:
	int wantedFrameNum(bool future);
    void addFrames(bool future);


    bool m_doReading;

    //the reader
    ReaderInterface *reader;
    
    //lists of pointers to frames
    QList<VideoData *> pastFrames;
    QList<VideoData *> futureFrames;
    QMutex listMutex;                     //protect the lists of frames
    
    //what is currently on the screen
    VideoData *displayFrame;
    
    //playback status    
    int direction;
    int speed;

    //a list of used frames for recycling, so that we don't need to re-allocate storage
    QList<VideoData *> usedFrames;

    //wait condition that is released each time the a frame is consumed for display
    QWaitCondition frameConsumed;         //condition variable to pause/wake reading thread, synchronising display rate and data reading thread
    QMutex frameMutex;
    
    
};

#endif
