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

#include <cassert>
#include <QtCore>
#include <sstream>

#include "videoData.h"
#include "frameQueue.h"
#include "videoTransport.h"
#include "readerInterface.h"
#include "stats.h"

#define DEBUG 0
#define LISTLEN 50
#define MAXREADS 10

FrameQueue::FrameQueue() :
	QThread()
{
	m_doReading = true;
	displayFrame = NULL;
	displayFrameNum = 0;
	speed=1;
	direction=0;
}

FrameQueue::~FrameQueue()
{
	m_doReading = false;
	wake();
	wait();

	if (displayFrame)
		delete displayFrame;
	displayFrame = NULL;

	while (!pastFrames.isEmpty())
		delete pastFrames.takeFirst();

	while (!futureFrames.isEmpty())
		delete futureFrames.takeFirst();

	while (!usedFrames.isEmpty())
		delete usedFrames.takeFirst();
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
		QMutexLocker listLocker(&listMutex);
		if (futureFrames.size())
			lastFrame = futureFrames.last()->frameNum;
	}
	else {
		QMutexLocker listLocker(&listMutex);
		if (pastFrames.size())
			lastFrame = pastFrames.last()->frameNum;
	}

	//frame offset from the last one
	int offset = future ? speed : -speed;
	wantedFrame = lastFrame + offset;

	return wantedFrame;
}

void FrameQueue::addFrames(bool future)
{
	QLinkedList<VideoData *> *list = (future == true) ? &futureFrames : &pastFrames;
	int stop= MAXREADS;

	QMutexLocker listLocker(&listMutex);
	int numFrames = list->size();
	listLocker.unlock();

	//add extra frames to the past frames list
	while (stop-- && numFrames < LISTLEN) {

		int wantedFrame = wantedFrameNum(future);

		VideoData *v = reader->pullFrame(wantedFrame);

		listLocker.relock();
		list->append(v);
		numFrames = list->size();
		listLocker.unlock();
	}
}

//called from the transport controller to get frame data for display
VideoData* FrameQueue::getNextFrame(int transportSpeed, int transportDirection)
{
	speed = transportSpeed;
	direction = transportDirection;

	assert(speed > 0);

	//stopped or paused with no frame displayed, or forwards
	if ((direction == 0 && displayFrame == NULL) || direction == 1) {

		if (futureFrames.isEmpty()) {
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

		if (pastFrames.isEmpty()) {
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

	//stats
	{
		QMutexLocker listLocker(&listMutex);

		Stats &stat = Stats::getInstance();
		std::stringstream ss;

		ss << futureFrames.size();
		stat.addStat("FrameQueue", "QueueFuture", ss.str());

		ss.str("");
		ss << pastFrames.size();
		stat.addStat("FrameQueue", "QueuePast", ss.str());
	}

	return displayFrame;
}

void FrameQueue::wake()
{
	frameConsumed.wakeOne();
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

void FrameQueue::run()
{
	if (DEBUG)
		printf("Starting FrameQueue\n");

	//set to play forward to avoid deleting the frame lists first time round
	int lastSpeed = speed;

	//performance timer
	QTime timer;
	timer.restart();
	int addtime=0;
	int prunetime=0;
	int sleeptime=0;

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

		addtime = timer.restart();

		//------------------------------------------------------------------------------------------------
		//make sure the lists are not too long
		//remove excess frames from the future list, which accumulate when playing backwards
		//remove excess future frames
		while (1) {
			QMutexLocker listLocker(&listMutex);
			if (futureFrames.size() <= LISTLEN)
				break;
			VideoData *oldFrame = futureFrames.takeLast();
			if (DEBUG)
				printf("Retiring future frame %ld\n", oldFrame->frameNum);
			usedFrames.prepend(oldFrame);
		}

		//remove excess past frames
		while (1) {
			QMutexLocker listLocker(&listMutex);
			if (pastFrames.size() <= LISTLEN)
				break;
			VideoData *oldFrame = pastFrames.takeLast();
			if (DEBUG)
				printf("Retiring past frame %ld\n", oldFrame->frameNum);
			usedFrames.prepend(oldFrame);
		}

		prunetime = timer.restart();

		//stats
		{
			int busytime = addtime + prunetime;
			timer.restart();

			int totaltime = busytime + sleeptime;

			if(totaltime) {
				int load = (busytime * 100) / totaltime;

				Stats &stat = Stats::getInstance();
				std::stringstream ss;
				ss << load << "%";
				stat.addStat("FrameQueue", "Load", ss.str());

				ss.str("");
				ss << addtime << " ms";
				stat.addStat("FrameQueue", "AddTime", ss.str());

				ss.str("");
				ss << prunetime << "ms";
				stat.addStat("FrameQueue", "RemTime", ss.str());
			}
		}

		//wait for display thread to display a new frame
		frameMutex.lock();
		if(m_doReading)
			frameConsumed.wait(&frameMutex);
		frameMutex.unlock();

		sleeptime = timer.elapsed();
		timer.restart();
	}
}
