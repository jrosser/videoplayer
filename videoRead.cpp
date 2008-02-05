/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: videoRead.cpp,v 1.19 2008-02-05 00:12:06 asuraparaju Exp $
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

#include "videoRead.h"

#define DEBUG 0

VideoRead::VideoRead()
    : readThread(*this)
{
    lastTransportStatus = Fwd1;
    currentFrameNum=0;
    firstFrameNum=0;
    lastFrameNum=0;
    looping=true;

    displayFrame = NULL;
    readThread.start();

    transportMutex.lock();
    transportStatus = Fwd1;
    transportSpeed = 1;
    transportMutex.unlock();

    forceFileType = false;
    fileType = "";
}

void VideoRead::stop()
{
    readThread.stop();

    frameMutex.lock();
    frameConsumed.wakeOne();
    frameMutex.unlock();

    readThread.wait();
}

void VideoRead::setLooping(bool l)
{
    looping = l;
}

void VideoRead::setVideoWidth(int width)
{
    videoWidth = width;
}

void VideoRead::setVideoHeight(int height)
{
    videoHeight = height;
}

void VideoRead::setForceFileType(bool flag)
{
    forceFileType = flag;
}

void VideoRead::setFileType(const QString &t)
{
    fileType = t;
}

void VideoRead::setFileName(const QString &fn)
{
    QMutexLocker fileInfoLocker(&fileInfoMutex);
    fileName = fn;

    QFileInfo info(fn);

    QString type = forceFileType ? fileType.toLower() : info.suffix().toLower();

    printf("Playing file with type %s\n", type.toLatin1().data());

    if(type == "16p0") {
        dataFormat = VideoData::V16P0;
        //420
        lastFrameNum = info.size() / ((videoWidth * videoHeight * 3 * 2) / 2);
        lastFrameNum--;
    }

    if(type == "16p2") {
        dataFormat = VideoData::V16P2;
        //422
        lastFrameNum = info.size() / (videoWidth * videoHeight * 2 * 2);
        lastFrameNum--;
    }

    if(type == "16p4") {
        dataFormat = VideoData::V16P4;
        //444
        lastFrameNum = info.size() / (videoWidth * videoHeight * 3 * 2);
        lastFrameNum--;
    }

    if(type == "420p") {
        dataFormat = VideoData::V8P0;
        //420
        lastFrameNum = info.size() / ((videoWidth * videoHeight * 3) / 2);
        lastFrameNum--;
    }

    if(type == "422p") {
        dataFormat = VideoData::V8P2;
        //422
        lastFrameNum = info.size() / (videoWidth * videoHeight * 2);
        lastFrameNum--;
    }

    if(type == "444p") {
        dataFormat = VideoData::V8P4;
        //444
        lastFrameNum = info.size() / (videoWidth * videoHeight * 3);
        lastFrameNum--;
    }


    if(type == "i420") {
        dataFormat = VideoData::V8P0;

        lastFrameNum = info.size() / ((videoWidth * videoHeight * 3) / 2);
        lastFrameNum--;
    }

    if(type == "yv12") {
        dataFormat = VideoData::YV12;

        lastFrameNum = info.size() / ((videoWidth * videoHeight * 3) / 2);
        lastFrameNum--;
    }

    if(type == "uyvy") {
        dataFormat = VideoData::UYVY;

        lastFrameNum = info.size() / (videoWidth * videoHeight * 2);
        lastFrameNum--;
    }

    if(type == "v216") {
        dataFormat = VideoData::V216;

        lastFrameNum = info.size() / (videoWidth * videoHeight * 4);
        lastFrameNum--;
    }

    if(type == "v210") {
        dataFormat = VideoData::V210;

        lastFrameNum = info.size() / ((videoWidth * videoHeight * 2 * 4) / 3);
        lastFrameNum--;
    }

}

int VideoRead::getDirection()
{
    QMutexLocker transportLocker(&transportMutex);
    TransportControls ts = transportStatus;

    //stopped or paused
    int direction = 0;

    //any forward speed
    if(ts == Fwd1 || ts == Fwd2 || ts == Fwd5 || ts == Fwd10 || ts == Fwd20 || ts == Fwd50 || ts == Fwd100 || ts == JogFwd)
        direction = 1;

    //any reverse speed
    if(ts == Rev1 || ts == Rev2 || ts == Rev5 || ts == Rev10 || ts == Rev20 || ts == Rev50 || ts == Rev100 || ts == JogRev)
        direction = -1;

    return direction;
}

int VideoRead::getFutureQueueLen()
{
    QMutexLocker listLocker(&listMutex);
    return futureFrames.size();
}

int VideoRead::getPastQueueLen()
{
    QMutexLocker listLocker(&listMutex);
    return pastFrames.size();
}

int VideoRead::getIOLoad()
{
    QMutexLocker perfLocker(&perfMutex);
    return ioLoad;
}

int VideoRead::getIOBandwidth()
{
    QMutexLocker perfLocker(&perfMutex);
    return ioBandwidth;
}


//called from the openGL display widget to get frame data for display
VideoData* VideoRead::getNextFrame()
{
    //get the status of the transport
    QMutexLocker transportLocker(&transportMutex);
    TransportControls ts = transportStatus;
    transportLocker.unlock();

    //paused or stopped - get a frame if there is none
    if((ts == Pause || ts == Stop) && displayFrame == NULL) {
        ts = JogFwd;
    }

    //forwards
    if(ts == Fwd1 || ts == Fwd2 || ts == Fwd5 || ts == Fwd10 || ts == Fwd20 || ts == Fwd50 || ts == Fwd100 || ts == JogFwd) {

        if(futureFrames.size() == 0)
            printf("Dropped frame - no future frame available\n");
        else
        {
            QMutexLocker listLocker(&listMutex);

            //playing forward
            if(displayFrame != NULL)
                pastFrames.prepend(displayFrame);

            displayFrame = futureFrames.takeFirst();
            currentFrameNum = displayFrame->frameNum;
        }

        if(ts == JogFwd) {
            transportLocker.relock();
            transportStatus = Stop;
            transportLocker.unlock();
        }

        //stop if there is no looping - will break at speeds other than 1x
        if(currentFrameNum == lastFrameNum) {
            if(looping==false) {
                QMutexLocker transportLocker(&transportMutex);
                transportStatus = Stop;
                emit(endOfFile());
            }
        }

    }

    //backwards
    if(ts == Rev1 || ts == Rev2 || ts == Rev5 || ts == Rev10 || ts == Rev20 || ts == Rev50 || ts == Rev100 || ts == JogRev) {

        if(pastFrames.size() == 0)
            printf("Dropped frame - no past frame available\n");
        else {
                QMutexLocker listLocker(&listMutex);

            //playing backward
            if(displayFrame != NULL)
                futureFrames.prepend(displayFrame);

            displayFrame = pastFrames.takeFirst();
            currentFrameNum = displayFrame->frameNum;
        }

        if(ts == JogRev) {
            transportLocker.relock();
            transportStatus = Stop;
            transportLocker.unlock();
        }

        //stop if there is no looping - will break at speeds other than 1x
        if(currentFrameNum == 0) {
            if(looping==false) {
                QMutexLocker transportLocker(&transportMutex);
                transportStatus = Stop;
                emit(endOfFile());
            }
        }
    }


    //wake the reader thread - done each time as jogging may move the frame number
    frameMutex.lock();
    frameConsumed.wakeOne();
    frameMutex.unlock();

    return displayFrame;
}

void VideoRead::transportFwd100() {transportController(Fwd100);}
void VideoRead::transportFwd50() {transportController(Fwd50);}
void VideoRead::transportFwd20() {transportController(Fwd20);}
void VideoRead::transportFwd10() {transportController(Fwd10);}
void VideoRead::transportFwd5() {transportController(Fwd5);}
void VideoRead::transportFwd2() {transportController(Fwd2);}
void VideoRead::transportFwd1() {transportController(Fwd1);}
void VideoRead::transportStop() {transportController(Stop);}
void VideoRead::transportRev1() {transportController(Rev1);}
void VideoRead::transportRev2() {transportController(Rev2);}
void VideoRead::transportRev5() {transportController(Rev5);}
void VideoRead::transportRev10() {transportController(Rev10);}
void VideoRead::transportRev20() {transportController(Rev20);}
void VideoRead::transportRev50() {transportController(Rev50);}
void VideoRead::transportRev100() {transportController(Rev100);}
void VideoRead::transportJogFwd() {transportController(JogFwd);}
void VideoRead::transportJogRev() {transportController(JogRev);}
void VideoRead::transportPlayPause() {transportController(PlayPause);}
void VideoRead::transportPause() {transportController(Pause);}

void VideoRead::transportController(TransportControls in)
{
    TransportControls newStatus = Unknown;
    TransportControls currentStatus;

    transportMutex.lock();
    currentStatus = transportStatus;
    int newSpeed  = transportSpeed;
    transportMutex.unlock();

    switch(in) {
        case Stop:
            newStatus = Stop;
            lastTransportStatus = Fwd1;    //after 'Stop', unpausing makes us play forwards
            newSpeed = 1;                //at 1x speed
            break;

        case JogFwd:
            if(currentStatus == Stop || currentStatus == Pause) {
                newStatus = JogFwd; //do jog forward
            }
            break;

        case JogRev:
            if(currentStatus == Stop || currentStatus == Pause) {
                newStatus = JogRev; //do jog backward
            }
            break;

        case Pause:
             //when not stopped or paused we can pause, remebering what were doing
            if(currentStatus != Stop || currentStatus == Pause) {
                lastTransportStatus = currentStatus;
                newStatus = Pause;
            }
            break;

        case PlayPause:
             //PlayPause toggles between stop and playing, and paused and playing. It
             //remembers the direction and speed were going
             if(currentStatus == Stop || currentStatus == Pause) {

                 if(lastTransportStatus == Stop)
                     newStatus = Fwd1;
                 else
                     newStatus = lastTransportStatus;

             } else {
                 lastTransportStatus = currentStatus;
                 newStatus = Pause;
             }
             break;

         //we can change from any state to a 'shuttle' or play
         case Fwd100:
         case Rev100:
             newSpeed = 100;
             newStatus = in;
             break;

         case Fwd50:
         case Rev50:
              newSpeed = 50;
             newStatus = in;
             break;

         case Fwd20:
         case Rev20:
              newSpeed = 20;
             newStatus = in;
             break;

         case Fwd10:
         case Rev10:
              newSpeed = 10;
             newStatus = in;
             break;

         case Fwd5:
         case Rev5:
              newSpeed = 5;
             newStatus = in;
             break;

         case Fwd2:
         case Rev2:
              newSpeed = 2;
             newStatus = in;
             break;

         case Fwd1:
         case Rev1:
            newSpeed = 1;
             newStatus = in;
             break;

         default:
             //oh-no!
            break;
    }

    //if we have determined a new transport status, set it
    if(newStatus != Unknown) {
        transportMutex.lock();
        transportStatus = newStatus;
        transportSpeed = newSpeed;
        transportMutex.unlock();
        if(DEBUG) printf("Changed transport state to %d, speed %d\n", (int)transportStatus, transportSpeed);
    }

}
