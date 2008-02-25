/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: readThread.h,v 1.8 2008-02-25 15:08:05 jrosser Exp $
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
    ReadThread();
    
    void run();
    void stop();
    void wake();
    
    void setForceFileType(bool f);
    void setFileType(const QString &t);
    void setVideoWidth(int w);
    void setVideoHeight(int h);
    void setFileName(const QString &fn);
    
    VideoData *getNextFrame(int speed, int direction);
    int getFutureQueueLen(void);
    int getPastQueueLen(void);
    int getIOLoad(void);
    int getIOBandwidth(void);
    
private:
    void addFutureFrames();
    void addPastFrames();

    bool m_doReading;

    //lists of pointers to frames
    QList<VideoData *> pastFrames;
    QList<VideoData *> futureFrames;
    QMutex listMutex;                     //protect the lists of frames
    
    //the extents of the sequence
    int firstFrameNum;
    int lastFrameNum;

    //what is currently on the screen
    VideoData *displayFrame;
    unsigned long currentFrameNum;		 //the frame number displayed last
    
    //playback status
    QMutex playStatusMutex;    
    int direction;
    int speed;

    //information about the video data
    VideoData::DataFmt videoFormat;
    QString fileType;  
    QString fileName;
    bool forceFileType;
    int videoWidth;
    int videoHeight;

    //video file
    int fd;

    //bandwidth counter
    int bandwidth_count;
    int ioLoad;
    int ioBandwidth;
    QMutex perfMutex;
    
    //a list of used frames for recycling, so that we don't need to re-allocate storage
    QList<VideoData *> usedFrames;

    //wait condition that is released each time the a frame is consumed for display
    QWaitCondition frameConsumed;         //condition variable to pause/wake reading thread, synchronising display rate and data reading thread
    QMutex frameMutex;
    
    
};

#endif
