/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: readThread.cpp,v 1.16 2008-02-05 00:12:06 asuraparaju Exp $
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

#include <QtGui>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

#ifdef Q_OS_WIN32
#include <io.h>
#define lseek64 _lseeki64
#define read _read
#define open _open
#define off64_t qint64
#endif

#ifdef Q_OS_MAC
#define lseek64 lseek
#define off64_t off_t
#endif

#include "readThread.h"
#include "videoRead.h"

#define DEBUG 0
#define LISTLEN 10
#define MAXREADS 10
#define MINUSED ((LISTLEN * 2) + 5)

ReadThread::ReadThread(VideoRead &v)
      : QThread(), vr(v)
{
    m_doReading = true;
    bandwidth_count = 0;
}

void ReadThread::stop()
{
    m_doReading = false;
}

void ReadThread::addPastFrames()
{
    int stop = MAXREADS;

    //add extra frames to the past frames list
    while(stop-- && numPastFrames < LISTLEN) {

        int lastPastFrame;
        QMutexLocker listLocker(&vr.listMutex);

        if(vr.transportSpeed != speed) {                                //the playback speed has changed - bail out early
            if(DEBUG) printf("Transport speed changed - aborting read\n");
            break;
        }

        if(vr.pastFrames.size() > 0)
            lastPastFrame = vr.pastFrames.last()->frameNum;                //get the number of the last future frame
        else
            lastPastFrame = vr.currentFrameNum;

        listLocker.unlock();

        int wantedFrame = lastPastFrame - speed;
        if(wantedFrame < 0) wantedFrame += (lastFrame + 1);
        wantedFrame %= lastFrame + 1;

        if(DEBUG) printf("Getting past frame %d\n", wantedFrame);

        VideoData *newFrame = NULL;

        if(usedFrames.size()) {
            if(DEBUG) printf("Recycling used frame for new past frame\n");
            newFrame = usedFrames.takeLast();
        }
        else {
            if(DEBUG) printf("Allocating frame for new past frame\n");
            newFrame = new VideoData(videoWidth, videoHeight, dataFormat);
        }
        newFrame->frameNum = wantedFrame;
        newFrame->renderFormat = newFrame->diskFormat;

        off64_t offset = (off_t)newFrame->dataSize * (off_t)wantedFrame;
        off64_t sret = lseek64(fd, offset, SEEK_SET);
        if(!sret && offset != 0) perror("LSEEK");

        int rret = read(fd, newFrame->data, newFrame->dataSize);
        bandwidth_count += newFrame->dataSize;
        if(!rret) perror("READ");

        listLocker.relock();
        vr.pastFrames.append(newFrame);
        numPastFrames = vr.pastFrames.size();
        listLocker.unlock();
    }
}

void ReadThread::addFutureFrames()
{
    int stop = MAXREADS;

    while(stop-- && numFutureFrames < LISTLEN) {

        int lastFutureFrame;
        QMutexLocker listLocker(&vr.listMutex);

        if(vr.transportSpeed != speed) {                                //the playback speed has changed - bail out early
            if(DEBUG) printf("Transport speed changed - aborting read\n");
            break;
        }

        if(vr.futureFrames.size() > 0)
            lastFutureFrame = vr.futureFrames.last()->frameNum;        //get the number of the last future frame
        else
            lastFutureFrame = vr.currentFrameNum - 1;

        listLocker.unlock();

        int wantedFrame = lastFutureFrame + speed;            //work out the number of the next future frame
        wantedFrame %= lastFrame + 1;                        //wrap from the end of the file to the start

        if(DEBUG) printf("Getting future frame %d\n", wantedFrame);

        VideoData *newFrame = NULL;

        if(usedFrames.size()) {
            if(DEBUG) printf("Recycling used frame for new future frame\n");
            newFrame = usedFrames.takeLast();
        }
        else {
            if(DEBUG) printf("Allocating frame for new past frame\n");
            newFrame = new VideoData(videoWidth, videoHeight, dataFormat);
        }

        newFrame->frameNum = wantedFrame;
        newFrame->renderFormat = newFrame->diskFormat;

        off64_t offset = (off_t)newFrame->dataSize * (off_t)wantedFrame;    //seek to the wanted frame
        off64_t sret = lseek64(fd, offset, SEEK_SET);
        if(!sret && offset != 0) perror("LSEEK");

        int rret = read(fd, newFrame->data, newFrame->dataSize);
        bandwidth_count += newFrame->dataSize;
        if(!rret) perror("READ");

        listLocker.relock();
        vr.futureFrames.append(newFrame);
        numFutureFrames = vr.futureFrames.size();
        listLocker.unlock();
    }
}

void ReadThread::run()
{
    if(DEBUG) printf("Starting readThread\n");
    fd = 0;
    unsigned int preallocate = 20;
    QTime activeTimer;
    QTime idleTimer;

    int averaging=10;
    int activeTime=0;
    int idleTime=0;

    //get the last transport status to avoid deleting the frame lists first time round
    QMutexLocker transportLocker(&vr.transportMutex);
    lastSpeed = vr.transportSpeed;
    transportLocker.unlock();

    while(m_doReading) {

        activeTimer.restart();

        //get the transport status
        transportLocker.relock();
        VideoRead::TransportControls ts = vr.transportStatus;
        speed = vr.transportSpeed;
        transportLocker.unlock();

        QMutexLocker fileInfoLocker(&vr.fileInfoMutex);

        if(fd <= 0) {
            if(DEBUG) printf("Opening file %s\n", vr.fileName.toLatin1().data());
            fd = open(vr.fileName.toLatin1().data(), O_RDONLY);

            if(fd < 0 && DEBUG == 1) {
                printf("[%s]\n", vr.fileName.toLatin1().data());
                perror("OPEN");
            }
        }

        videoHeight = vr.videoHeight;
        videoWidth  = vr.videoWidth;
        dataFormat  = vr.dataFormat;

        fileInfoLocker.unlock();

        if(fd > 0) {

            //------------------------------------------------------------------------------------------------
            //make sure that the used frame list is at least MINUSED long, so there are some spare frames sloshing
            //around to read into
            for(; preallocate>0; preallocate--) {
                if(DEBUG) printf("Preallocating used frame, width=%d, height=%d, format=%d\n", videoWidth, videoHeight, dataFormat);
                VideoData *newFrame = new VideoData(videoWidth, videoHeight, dataFormat);
                usedFrames.prepend(newFrame);
            }

            //------------------------------------------------------------------------------------------------
            //trash the contents of the frame lists when we change speed
            //this could be made MUCH cleverer - to keep past and future frames that we need when changing speed
            if(speed != lastSpeed) {

                if(DEBUG) printf("Trashing frame lists - ts=%d speed=%d, last=%d\n", ts, speed, lastSpeed);

                QMutexLocker listLocker(&vr.listMutex);

                while(vr.futureFrames.size()) {
                    VideoData *frame = vr.futureFrames.takeLast();
                    usedFrames.prepend(frame);
                    //delete frame;
                }

                while(vr.pastFrames.size()) {
                    VideoData *frame = vr.pastFrames.takeLast();
                    usedFrames.prepend(frame);
                    //delete frame;
                }

                listLocker.unlock();

            }

            lastSpeed = speed;

            //------------------------------------------------------------------------------------------------
            //get information about the length of the lists
            {
                QMutexLocker listLocker(&vr.listMutex);

                //info about the lists of frames
                numFutureFrames = vr.futureFrames.size();
                numPastFrames = vr.pastFrames.size();

                //the extents of the sequence
                firstFrame = vr.firstFrameNum;
                lastFrame = vr.lastFrameNum;

                //the format and dimensions of the sequence  - which mutex should protect theses properly???
                dataFormat = vr.dataFormat;
                videoWidth = vr.videoWidth;
                videoHeight = vr.videoHeight;

                listLocker.unlock();
            }

            //------------------------------------------------------------------------------------------------
            //make sure the lists are long enough

            //add extra frames to the future frames list when playing forward
            if(ts == VideoRead::Fwd100 || ts == VideoRead::Fwd50 || ts == VideoRead::Fwd20 || ts == VideoRead::Fwd10 ||
               ts == VideoRead::Fwd5   || ts == VideoRead::Fwd2  || ts == VideoRead::Fwd1) {
                addFutureFrames();
            }

            //add extra frames to the past frame list when playing backward
            if(ts == VideoRead::Rev100 || ts == VideoRead::Rev50 || ts == VideoRead::Rev20 || ts == VideoRead::Rev10 ||
               ts == VideoRead::Rev5   || ts == VideoRead::Rev2  || ts == VideoRead::Rev1) {
                addPastFrames();
            }

            //when pasued or stopped fill both lists for nice jog-wheel response
            if(ts == VideoRead::Pause || ts == VideoRead::Stop) {
                addFutureFrames();
                addPastFrames();
            }

            //------------------------------------------------------------------------------------------------
            //make sure the lists are not too long
            //remove excess frames from the future list, which accumulate when playing backwards
            while(numFutureFrames > LISTLEN) {

                QMutexLocker listLocker(&vr.listMutex);
                VideoData *oldFrame = vr.futureFrames.takeLast();
                numFutureFrames = vr.futureFrames.size();

                //delete oldFrame;
                usedFrames.prepend(oldFrame);    //recycle

                listLocker.unlock();
            }

            //remove excess frames from the past list, which accumulate when playing forward
            while(numPastFrames > LISTLEN) {

                QMutexLocker listLocker(&vr.listMutex);
                VideoData *oldFrame = vr.pastFrames.takeLast();
                numPastFrames = vr.pastFrames.size();

                //delete oldFrame;
                usedFrames.prepend(oldFrame);    //recycle

                listLocker.unlock();
            }
        }

        activeTime += activeTimer.elapsed();
        idleTimer.restart();

#ifdef Q_OS_UNIX
/* Broken under win32? */
        vr.frameMutex.lock();
        vr.frameConsumed.wait(&(vr.frameMutex));
        vr.frameMutex.unlock();
#endif

        idleTime += idleTimer.elapsed();

        averaging--;
        if(averaging==0) {
            vr.perfMutex.lock();

            if(idleTime != 0)
                vr.ioLoad = (100 * activeTime) / (idleTime + activeTime);
            else
                vr.ioLoad = 100;

            vr.ioBandwidth = bandwidth_count / ((idleTime + activeTime) * 1000);

            idleTime = 0;
            activeTime = 0;
            bandwidth_count = 0;

            vr.perfMutex.unlock();
            averaging=10;
        }

    }
}
