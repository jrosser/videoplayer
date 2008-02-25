/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: readThread.cpp,v 1.17 2008-02-25 15:08:05 jrosser Exp $
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

ReadThread::ReadThread() : QThread()
{
    m_doReading = true;
    bandwidth_count = 0;
    currentFrameNum = 0;
    displayFrame = NULL;
    start();
}

void ReadThread::stop()
{
    m_doReading = false;
    wake();
    wait();
}

void ReadThread::setForceFileType(bool f)
{
	forceFileType = f;
}

void ReadThread::setFileType(const QString &t)
{
	fileType = t;
}

void ReadThread::setVideoWidth(int w)
{
	videoWidth = w;
}

void ReadThread::setVideoHeight(int h)
{
	videoHeight = h;
}

int ReadThread::getIOLoad()
{
    QMutexLocker perfLocker(&perfMutex);
    return ioLoad;
}

int ReadThread::getIOBandwidth()
{
    QMutexLocker perfLocker(&perfMutex);
    return ioBandwidth;
}

void ReadThread::setFileName(const QString &fn)
{
	fileName = fn;
    QFileInfo info(fileName);

	printf("Readthread file name set to %s\n", fileName.toAscii().data());    
    
    QString type = forceFileType ? fileType.toLower() : info.suffix().toLower();

    printf("Playing file with type %s\n", type.toLatin1().data());

    if(type == "16p0") {
        videoFormat = VideoData::V16P0;
        lastFrameNum = info.size() / ((videoWidth * videoHeight * 3 * 2) / 2);
        lastFrameNum--;
    }

    if(type == "16p2") {
    	videoFormat = VideoData::V16P2;
        lastFrameNum = info.size() / (videoWidth * videoHeight * 2 * 2);
        lastFrameNum--;
    }

    if(type == "16p4") {
    	videoFormat = VideoData::V16P4;
        lastFrameNum = info.size() / (videoWidth * videoHeight * 3 * 2);
        lastFrameNum--;
    }
	
    if(type == "420p") {
	    videoFormat = VideoData::V8P0;
        lastFrameNum = info.size() / ((videoWidth * videoHeight * 3) / 2);
        lastFrameNum--;
    }
	    
	if(type == "422p") {
	    videoFormat = VideoData::V8P2;
        lastFrameNum = info.size() / (videoWidth * videoHeight * 2);
        lastFrameNum--;
    }
	    
	if(type == "444p") {
	    videoFormat = VideoData::V8P4;
        lastFrameNum = info.size() / (videoWidth * videoHeight * 3);
        lastFrameNum--;
    }

    if(type == "i420") {
    	videoFormat = VideoData::V8P0;
        lastFrameNum = info.size() / ((videoWidth * videoHeight * 3) / 2);
        lastFrameNum--;
    }

    if(type == "yv12") {
    	videoFormat = VideoData::YV12;
        lastFrameNum = info.size() / ((videoWidth * videoHeight * 3) / 2);
        lastFrameNum--;
    }
	
    if(type == "uyvy") {
	   	videoFormat = VideoData::UYVY;
	    lastFrameNum = info.size() / (videoWidth * videoHeight * 2);
	    lastFrameNum--;
	}

	if(type == "v216") {
	 	videoFormat = VideoData::V216;
	    lastFrameNum = info.size() / (videoWidth * videoHeight * 4);
	    lastFrameNum--;
	}

	if(type == "v210") {
	  	videoFormat = VideoData::V210;
	    lastFrameNum = info.size() / ((videoWidth * videoHeight * 2 * 4) / 3);
	    lastFrameNum--;
	}
}


void ReadThread::addPastFrames()
{
    int stop = MAXREADS;

    QMutexLocker listLocker(&listMutex);
    int numPastFrames = pastFrames.size();
    listLocker.unlock();
    
    //add extra frames to the past frames list
    while(stop-- && numPastFrames < LISTLEN) {
    	
        listLocker.relock();
        int lastPastFrame;        
        if(pastFrames.size() > 0)
            lastPastFrame = pastFrames.last()->frameNum;                //get the number of the last future frame
        else
            lastPastFrame = currentFrameNum;
        listLocker.unlock();

        int wantedFrame = lastPastFrame - speed;
        if(wantedFrame < 0) wantedFrame += (lastFrameNum + 1);
        wantedFrame %= lastFrameNum + 1;

        if(DEBUG) printf("Getting past frame %d, speed=%d\n", wantedFrame, speed);

        VideoData *newFrame = NULL;

        if(usedFrames.size()) {
            if(DEBUG) printf("Recycling used frame for new past frame\n");
            newFrame = usedFrames.takeLast();
        }
        else {
            if(DEBUG) printf("Allocating frame for new past frame\n");
            newFrame = new VideoData(videoWidth, videoHeight, videoFormat);
        }
        
        newFrame->frameNum = wantedFrame;
        newFrame->isFirstFrame = (wantedFrame == firstFrameNum);
        newFrame->isLastFrame  = (wantedFrame == lastFrameNum);        
        newFrame->renderFormat = newFrame->diskFormat;

        off64_t offset = (off_t)newFrame->dataSize * (off_t)wantedFrame;
        off64_t sret = lseek64(fd, offset, SEEK_SET);
        if(!sret && offset != 0) perror("LSEEK");

        int rret = read(fd, newFrame->data, newFrame->dataSize);
        bandwidth_count += newFrame->dataSize;
        if(!rret) perror("READ");

        listLocker.relock();
        pastFrames.append(newFrame);
        numPastFrames = pastFrames.size();
        listLocker.unlock();
    }
}

void ReadThread::addFutureFrames()
{
    int stop = MAXREADS;

    QMutexLocker listLocker(&listMutex);
    int numFutureFrames = futureFrames.size();
    listLocker.unlock();    
    
    while(stop-- && numFutureFrames < LISTLEN) {

        listLocker.relock();
        int lastFutureFrame;        
        if(futureFrames.size() > 0)
            lastFutureFrame = futureFrames.last()->frameNum;        //get the number of the last future frame
        else
            lastFutureFrame = currentFrameNum - 1;
        listLocker.unlock();

        int wantedFrame = lastFutureFrame + speed;            //work out the number of the next future frame
        wantedFrame %= lastFrameNum + 1;                        //wrap from the end of the file to the start

        if(DEBUG) printf("Getting future frame %d, speed=%d\n", wantedFrame, speed);

        VideoData *newFrame = NULL;

        if(usedFrames.size()) {
            newFrame = usedFrames.takeLast();
        }
        else {
            if(DEBUG) printf("Allocating frame for new past frame\n");
            newFrame = new VideoData(videoWidth, videoHeight, videoFormat);
        }

        newFrame->frameNum = wantedFrame;
        newFrame->isFirstFrame = (wantedFrame == firstFrameNum);
        newFrame->isLastFrame  = (wantedFrame == lastFrameNum);
        newFrame->renderFormat = newFrame->diskFormat;

        off64_t offset = (off_t)newFrame->dataSize * (off_t)wantedFrame;    //seek to the wanted frame
        off64_t sret = lseek64(fd, offset, SEEK_SET);
        if(!sret && offset != 0) perror("LSEEK");

        int rret = read(fd, newFrame->data, newFrame->dataSize);
        bandwidth_count += newFrame->dataSize;
        if(!rret) perror("READ");

        listLocker.relock();
        futureFrames.append(newFrame);
        numFutureFrames = futureFrames.size();
        listLocker.unlock();
    }
}

int ReadThread::getFutureQueueLen()
{
	QMutexLocker listLocker(&listMutex);
	return futureFrames.size();
}

int ReadThread::getPastQueueLen()
{
	QMutexLocker listLocker(&listMutex);
	return pastFrames.size();	
}

//called from the transport controller to get frame data for display
VideoData* ReadThread::getNextFrame(int transportSpeed, int transportDirection)
{
	QMutexLocker playStatusLocker(&playStatusMutex);
	speed = transportSpeed;
	direction = transportDirection;
	playStatusLocker.unlock();
	
    
    //stopped or paused with no frame displayed, or forwards
    if((direction == 0 && displayFrame == NULL) || direction == 1) {

        if(futureFrames.size() == 0)
            printf("Dropped frame - no future frame available\n");
        else
        {
            QMutexLocker listLocker(&listMutex);

            //playing forward
            if(displayFrame) pastFrames.prepend(displayFrame);

            displayFrame = futureFrames.takeFirst();
            
            if(displayFrame) currentFrameNum = displayFrame->frameNum;
        	if(DEBUG) printf("Getting next future frame %ld\n", currentFrameNum);            
        }
    
    }

    //backwards
    if(direction == -1) {

        if(pastFrames.size() == 0)
            printf("Dropped frame - no past frame available\n");
        else 
        {
        	QMutexLocker listLocker(&listMutex);

            //playing backward
            if(displayFrame) futureFrames.prepend(displayFrame);

            displayFrame = pastFrames.takeFirst();
            
            if(displayFrame) currentFrameNum = displayFrame->frameNum;
        	if(DEBUG) printf("Getting next past frame %ld\n", currentFrameNum);            
        }        
    }

    return displayFrame;
}

void ReadThread::wake()
{
    frameMutex.lock();
    frameConsumed.wakeOne();
    frameMutex.unlock();	
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

    //set to play forward to avoid deleting the frame lists first time round
    QMutexLocker playStatusLocker(&playStatusMutex);
    int currentSpeed = speed;
    int lastSpeed = speed;
    int currentDirection = direction;
    playStatusLocker.unlock();
    
    while(m_doReading) {

        activeTimer.restart();

        //open the source file if it not already open
        if(fd <= 0) {
            if(DEBUG) printf("Opening file %s\n", fileName.toLatin1().data());
            fd = open(fileName.toLatin1().data(), O_RDONLY);

            if(fd < 0 && DEBUG == 1) {
                printf("[%s]\n", fileName.toLatin1().data());
                perror("OPEN");
            }
        }

        if(fd > 0) {

        	
            //------------------------------------------------------------------------------------------------
            //make sure that the used frame list is at least MINUSED long, so there are some spare frames sloshing
            //around to read into
            for(; preallocate>0; preallocate--) {
                if(DEBUG) printf("Preallocating used frame, width=%d, height=%d, format=%d\n", videoWidth, videoHeight, videoFormat);
                VideoData *newFrame = new VideoData(videoWidth, videoHeight, videoFormat);
                usedFrames.prepend(newFrame);
            }

            //------------------------------------------------------------------------------------------------
            //trash the contents of the frame lists when we change speed
            //this could be made MUCH cleverer - to keep past and future frames that we need when changing speed
            playStatusLocker.relock();
            currentSpeed = speed;
            playStatusLocker.unlock();            
            
            if(currentSpeed != lastSpeed) {

                if(DEBUG) printf("Trashing frame lists - speed=%d, last=%d\n", currentSpeed, lastSpeed);

                QMutexLocker listLocker(&listMutex);

                while(futureFrames.size()) {
                    VideoData *frame = futureFrames.takeLast();
                    usedFrames.prepend(frame);
                    //delete frame;
                }

                while(pastFrames.size()) {
                    VideoData *frame = pastFrames.takeLast();
                    usedFrames.prepend(frame);
                    //delete frame;
                }

                listLocker.unlock();

            }

            lastSpeed = currentSpeed;
            
            //------------------------------------------------------------------------------------------------
            //make sure the lists are long enough
            playStatusLocker.relock();
            currentDirection = direction;
            playStatusLocker.unlock();            

            //add extra frames to the future frames list when playing forward
            if(currentDirection == 1) {
            	if(DEBUG) printf("Adding future frames\n");
                addFutureFrames();
            }

            //add extra frames to the past frame list when playing backward
            if(currentDirection == -1) {
            	if(DEBUG) printf("Adding past frames\n");            	
                addPastFrames();
            }

            //when pasued or stopped fill both lists for nice jog-wheel response
            if(currentDirection == 0) {
                addFutureFrames();
                addPastFrames();
            }

            //------------------------------------------------------------------------------------------------
            //make sure the lists are not too long
            //remove excess frames from the future list, which accumulate when playing backwards
            {
                QMutexLocker listLocker(&listMutex);
                int numFutureFrames = futureFrames.size();
                int numPastFrames = pastFrames.size();
                listLocker.unlock();

                //remove excess future frames
                while(numFutureFrames > LISTLEN) {                	
                	listLocker.relock();
                	VideoData *oldFrame = futureFrames.takeLast();
                	if(DEBUG) printf("Retiring future frame %ld\n", oldFrame->frameNum);                	
                	usedFrames.prepend(oldFrame);
                	numFutureFrames = futureFrames.size();                	
                	listLocker.unlock();
                }
                
                //remove excess past frames
                while(numPastFrames > LISTLEN) {                	
                    listLocker.relock();
                    VideoData *oldFrame = pastFrames.takeLast();
                	if(DEBUG) printf("Retiring past frame %ld\n", oldFrame->frameNum);                    
                    usedFrames.prepend(oldFrame);
                    numPastFrames = pastFrames.size();                    
                    listLocker.unlock();
                }            	
            }

        }

        //grab performance timers
        activeTime += activeTimer.elapsed();
        idleTimer.restart();

        //wait for display thread to display a new frame
        frameMutex.lock();
        frameConsumed.wait(&frameMutex);
        frameMutex.unlock();

        idleTime += idleTimer.elapsed();

        //process performance timer info
        averaging--;
        if(averaging==0) {
        	QMutexLocker perfMutexLocker(&perfMutex);

            int totalTime = idleTime + activeTime;
            
            if(totalTime != 0) {
            	ioLoad = (100 * activeTime) / (totalTime);
                ioBandwidth = bandwidth_count / (totalTime * 1000);
            }
            else {            	
                ioLoad = 100;
                ioBandwidth = 0;
            }

            idleTime = 0;
            activeTime = 0;
            bandwidth_count = 0;

            averaging=10;
        }

    }
}
