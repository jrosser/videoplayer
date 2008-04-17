/* ***** BEGIN LICENSE BLOCK *****
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

#include "frameQueue.h"
#include "videoTransport.h"
#include "readerInterface.h"

#define DEBUG 0
#define LISTLEN 50
#define MAXREADS 10

FrameQueue::FrameQueue() :
	QThread()
{
	m_doReading = true;
	displayFrame = NULL;
	displayFrameNum = 0;
	speed=0;
	direction=0;
}

void FrameQueue::stop()
{
	m_doReading = false;
	wake();
	wait();
}

int FrameQueue::wantedFrameNum(bool future)
{
	//if there are no frames in the list, then the last displayed frame number is the
	//starting point for working out what the next needed frame is
	int lastFrame=displayFrameNum;
	int wantedFrame;

	//if the reader cannot do random access then it will always return the next frame
	//we cannot seek so frame numbers are a bit meaningless here
	if (reader->randomAccess == false)
		return 0;

	//if we can seek then get the number of the last frame in the queue
	if (future == true) {
		if (futureFrames.size())
			lastFrame = futureFrames.last()->frameNum;
	}
	else {
		if (pastFrames.size())
			lastFrame = pastFrames.last()->frameNum;
	}

	//frame offset from the last one
	int offset = speed;
	if (direction == -1)
		offset = offset * -1;
	wantedFrame = lastFrame + offset;

	return wantedFrame;
}

void FrameQueue::addFrames(bool future)
{
	QList<VideoData *> *list = (future == true) ? &futureFrames : &pastFrames;
	int stop= MAXREADS;

	QMutexLocker listLocker(&listMutex);
	int numFrames = list->size();
	listLocker.unlock();

	//add extra frames to the past frames list
	while (stop-- && numFrames < LISTLEN) {

		int wantedFrame = wantedFrameNum(future);

		VideoData *v;
		reader->pullFrame(wantedFrame, v);

		listLocker.relock();
		list->append(v);
		numFrames = list->size();
		listLocker.unlock();
	}
}

int FrameQueue::getFutureQueueLen()
{
	QMutexLocker listLocker(&listMutex);
	return futureFrames.size();
}

int FrameQueue::getPastQueueLen()
{
	QMutexLocker listLocker(&listMutex);
	return pastFrames.size();
}

//called from the transport controller to get frame data for display
VideoData* FrameQueue::getNextFrame(int transportSpeed, int transportDirection)
{
	speed = transportSpeed;
	direction = transportDirection;

	//stopped or paused with no frame displayed, or forwards
	if ((direction == 0 && displayFrame == NULL) || direction == 1) {

		if (futureFrames.size() == 0) {
			//printf("Dropped frame - no future frame available\n");
		}
		else {
			QMutexLocker listLocker(&listMutex);

			//playing forward
			if (displayFrame)
				pastFrames.prepend(displayFrame);

			displayFrame = futureFrames.takeFirst();
			displayFrameNum = displayFrame->frameNum;
			listLocker.unlock();
		}
	}

	//backwards
	if (direction == -1) {

		if (pastFrames.size() == 0) {
			//printf("Dropped frame - no past frame available\n");
		}
		else {
			QMutexLocker listLocker(&listMutex);

			//playing backward
			if (displayFrame)
				futureFrames.prepend(displayFrame);

			displayFrame = pastFrames.takeFirst();
			displayFrameNum = displayFrame->frameNum;
			listLocker.unlock();
		}
	}

	return displayFrame;
}

void FrameQueue::wake()
{
	frameMutex.lock();
	frameConsumed.wakeOne();
	frameMutex.unlock();
}

VideoData *FrameQueue::allocateFrame(void)
{
	if (usedFrames.empty()) {
		return new VideoData;
	}
	else {
		QMutexLocker listlocker(&listMutex);
		return usedFrames.takeLast();
	}
}

void FrameQueue::releaseFrame(VideoData *v)
{
	QMutexLocker listlocker(&listMutex);
	usedFrames.append(v);
}

void FrameQueue::run()
{
	if (DEBUG)
		printf("Starting FrameQueue\n");

	//set to play forward to avoid deleting the frame lists first time round
	int lastSpeed = speed;

	while (m_doReading) {

		//------------------------------------------------------------------------------------------------
		//trash the contents of the frame lists when we change speed
		//this could be made MUCH cleverer - to keep past and future frames that we need when changing speed
		if (speed != lastSpeed) {

			if (DEBUG)
				printf("Trashing frame lists - speed=%d, last=%d\n", speed,
				       lastSpeed);

			QMutexLocker listLocker(&listMutex);

			while (futureFrames.size()) {
				VideoData *frame = futureFrames.takeLast();
				usedFrames.prepend(frame);
			}

			while (pastFrames.size()) {
				VideoData *frame = pastFrames.takeLast();
				usedFrames.prepend(frame);
			}
		}

		lastSpeed = speed;

		//------------------------------------------------------------------------------------------------
		//make sure the lists are long enough
		switch (direction) {
		case 1:
			if (DEBUG)
				printf("Adding future frames\n");
			addFrames(true);
			break;

		case -1:
			if (DEBUG)
				printf("Adding past frames\n");
			if (reader->randomAccess == true)
				addFrames(false);
			break;

		case 0:
			//when pasued or stopped fill both lists for nice jog-wheel response
			addFrames(true);
			if (reader->randomAccess == true)
				addFrames(false);
			break;

		default:
			break;

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
			while (numFutureFrames > LISTLEN) {
				listLocker.relock();
				VideoData *oldFrame = futureFrames.takeLast();
				if (DEBUG)
					printf("Retiring future frame %ld\n", oldFrame->frameNum);
				usedFrames.prepend(oldFrame);
				numFutureFrames = futureFrames.size();
				listLocker.unlock();
			}

			//remove excess past frames
			while (numPastFrames > LISTLEN) {
				listLocker.relock();
				VideoData *oldFrame = pastFrames.takeLast();
				if (DEBUG)
					printf("Retiring past frame %ld\n", oldFrame->frameNum);
				usedFrames.prepend(oldFrame);
				numPastFrames = pastFrames.size();
				listLocker.unlock();
			}
		}

		//wait for display thread to display a new frame
		frameMutex.lock();
		frameConsumed.wait(&frameMutex);
		frameMutex.unlock();

	}
}
