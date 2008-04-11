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

#include "videoTransport.h"

#define DEBUG 0

VideoTransport::VideoTransport(FrameQueue *fq) :
	frameQueue(fq)
{
	lastTransportStatus = Fwd1;
	looping=true;
	transportFwd1();
}

void VideoTransport::setLooping(bool l)
{
	looping = l;
}

int VideoTransport::getSpeed()
{
	QMutexLocker transportLocker(&transportMutex);
	TransportControls ts = transportStatus;
	transportLocker.unlock();

	int speed = 1;

	switch (ts) {

	case Fwd2:
	case Rev2:
		speed = 2;
		break;

	case Fwd5:
	case Rev5:
		speed = 5;
		break;

	case Fwd10:
	case Rev10:
		speed = 10;
		break;

	case Fwd20:
	case Rev20:
		speed = 20;
		break;

	case Fwd50:
	case Rev50:
		speed = 50;
		break;

	case Fwd100:
	case Rev100:
		speed = 100;
		break;

	default:
		break;
	}

	return speed;
}

int VideoTransport::getDirection()
{
	QMutexLocker transportLocker(&transportMutex);
	TransportControls ts = transportStatus;
	transportLocker.unlock();

	//stopped or paused
	int direction = 0;

	//any forward speed
	if (ts == Fwd1 || ts == Fwd2 || ts == Fwd5 || ts == Fwd10 || ts == Fwd20
	        || ts == Fwd50 || ts == Fwd100 || ts == JogFwd)
		direction = 1;

	//any reverse speed
	if (ts == Rev1 || ts == Rev2 || ts == Rev5 || ts == Rev10 || ts == Rev20
	        || ts == Rev50 || ts == Rev100 || ts == JogRev)
		direction = -1;

	return direction;
}

VideoData *VideoTransport::getNextFrame()
{
	int currentSpeed = getSpeed();
	int currentDirection = getDirection();

	QMutexLocker transportLocker(&transportMutex);
	TransportControls ts = transportStatus;
	transportLocker.unlock();

	VideoData *frame = frameQueue->getNextFrame(currentSpeed, currentDirection);

	//stop after each jog
	if (ts == JogFwd || ts == JogRev)
		transportStop();

	//stop at first or last frame if there is no looping - will break at speeds other than 1x
	if (frame) {
		if ((looping == false) && (frame->isLastFrame == true
		        || frame->isFirstFrame == true)) {
			transportStop();
			emit(endOfFile());
		}
	}

	//wake the reader thread - done each time as jogging will move the frame number
	frameQueue->wake();

	return frame;
}

void VideoTransport::transportFwd100()
{
	transportController(Fwd100);
}
void VideoTransport::transportFwd50()
{
	transportController(Fwd50);
}
void VideoTransport::transportFwd20()
{
	transportController(Fwd20);
}
void VideoTransport::transportFwd10()
{
	transportController(Fwd10);
}
void VideoTransport::transportFwd5()
{
	transportController(Fwd5);
}
void VideoTransport::transportFwd2()
{
	transportController(Fwd2);
}
void VideoTransport::transportFwd1()
{
	transportController(Fwd1);
}
void VideoTransport::transportStop()
{
	transportController(Stop);
}
void VideoTransport::transportRev1()
{
	transportController(Rev1);
}
void VideoTransport::transportRev2()
{
	transportController(Rev2);
}
void VideoTransport::transportRev5()
{
	transportController(Rev5);
}
void VideoTransport::transportRev10()
{
	transportController(Rev10);
}
void VideoTransport::transportRev20()
{
	transportController(Rev20);
}
void VideoTransport::transportRev50()
{
	transportController(Rev50);
}
void VideoTransport::transportRev100()
{
	transportController(Rev100);
}
void VideoTransport::transportJogFwd()
{
	transportController(JogFwd);
}
void VideoTransport::transportJogRev()
{
	transportController(JogRev);
}
void VideoTransport::transportPlayPause()
{
	transportController(PlayPause);
}
void VideoTransport::transportPause()
{
	transportController(Pause);
}

void VideoTransport::transportController(TransportControls in)
{
	TransportControls newStatus = Unknown;
	TransportControls currentStatus;

	transportMutex.lock();
	currentStatus = transportStatus;
	transportMutex.unlock();

	switch (in) {
	case Stop:
		newStatus = Stop;
		lastTransportStatus = Fwd1; //after 'Stop', unpausing makes us play forwards
		break;

	case JogFwd:
		if (currentStatus == Stop || currentStatus == Pause) {
			newStatus = JogFwd; //do jog forward
		}
		break;

	case JogRev:
		if (currentStatus == Stop || currentStatus == Pause) {
			newStatus = JogRev; //do jog backward
		}
		break;

	case Pause:
		//when not stopped or paused we can pause, remebering what were doing
		if (currentStatus != Stop || currentStatus == Pause) {
			lastTransportStatus = currentStatus;
			newStatus = Pause;
		}
		break;

	case PlayPause:
		//PlayPause toggles between stop and playing, and paused and playing. It
		//remembers the direction and speed were going
		if (currentStatus == Stop || currentStatus == Pause) {

			if (lastTransportStatus == Stop)
				newStatus = Fwd1;
			else
				newStatus = lastTransportStatus;

		}
		else {
			lastTransportStatus = currentStatus;
			newStatus = Pause;
		}
		break;

		//we can change from any state to a 'shuttle' or play
	case Fwd100:
	case Rev100:
		newStatus = in;
		break;

	case Fwd50:
	case Rev50:
		newStatus = in;
		break;

	case Fwd20:
	case Rev20:
		newStatus = in;
		break;

	case Fwd10:
	case Rev10:
		newStatus = in;
		break;

	case Fwd5:
	case Rev5:
		newStatus = in;
		break;

	case Fwd2:
	case Rev2:
		newStatus = in;
		break;

	case Fwd1:
	case Rev1:
		newStatus = in;
		break;

	default:
		//oh-no!
		break;
	}

	//if we have determined a new transport status, set it
	if (newStatus != Unknown) {
		transportMutex.lock();
		transportStatus = newStatus;
		transportMutex.unlock();
		if (DEBUG)
			printf("Changed transport state to %d\n", (int)transportStatus);
	}

}
