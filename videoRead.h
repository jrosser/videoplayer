/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: videoRead.h,v 1.13 2008-02-05 00:12:06 asuraparaju Exp $
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

#ifndef VIDEOREAD_H
#define VIDEOREAD_H

#include <QtGui>

#include "readThread.h"
#include "videoData.h"
#include "GLvideo_rt.h"
class VideoRead : public QObject
{
    Q_OBJECT

public:

    enum TransportControls { Rev100, Rev50, Rev20, Rev10, Rev5, Rev2, Rev1, PlayPause, Pause, Stop, Fwd1, Fwd2, Fwd5, Fwd10, Fwd20, Fwd50, Fwd100, JogFwd, JogRev, Unknown }; //transport control status

    VideoRead();
    void setFileName(const QString &fn);
    void setVideoWidth(int width);
    void setVideoHeight(int height);
    void setForceFileType(bool flag);
    void setFileType(const QString &t);
    void setLooping(bool l);
    void stop();

    QList<VideoData *> pastFrames;
    QList<VideoData *> futureFrames;
    QMutex listMutex;                     //protect the lists of frames

    int currentFrameNum;
    int firstFrameNum;
    int lastFrameNum;
    bool looping;

    int videoWidth;                       //input file
    int videoHeight;
    QString fileName;
    VideoData::DataFmt dataFormat;        //data format type
    QMutex fileInfoMutex;                 //protect the file information

    QWaitCondition frameConsumed;         //condition variable to pause/wake reading thread, synchronising display rate and data reading thread
    QMutex frameMutex;

    TransportControls transportStatus;    //what we are doing now - shared with readThread
    int transportSpeed;
    QMutex transportMutex;                //protect the transport status

    QMutex perfMutex;                     //protect the performance information
    int ioLoad;
    int ioBandwidth;

    VideoData* getNextFrame(void);        //the frame that is currently being displayed, this pointer is only manipulated from the read thread
    int getDirection();                   //return the current transport direction -1 reverse, 0 stopped/paused, 1 forward.
    int getFutureQueueLen();              //number of frames in the future queue
    int getPastQueueLen();                //number of frames in the past queue
    int getIOLoad();                      //percentage utilisation of the read thread
    int getIOBandwidth();                 //Mbytes/sec IO bandwidth

protected:

signals:
    void endOfFile(void);

private slots:
    void transportController(TransportControls c);

    //slots to connect keys / buttons to play and scrub controls
    void transportFwd100();
    void transportFwd50();
    void transportFwd20();
    void transportFwd10();
    void transportFwd5();
    void transportFwd2();
    void transportFwd1();
    void transportStop();
    void transportRev1();
    void transportRev2();
    void transportRev5();
    void transportRev10();
    void transportRev20();
    void transportRev50();
    void transportRev100();

    //frame jog controls
    void transportJogFwd();
    void transportJogRev();

    //play / pause
    void transportPlayPause();
    void transportPause();

private:
    ReadThread readThread;
    VideoData *displayFrame;                   //a pointer to the video data for the frame being displayed

    bool forceFileType;
    QString fileType;

    TransportControls lastTransportStatus;    //what we were doing before 'pause'
};

#endif
