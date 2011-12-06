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

#include <stdio.h>

#include "videoData.h"
#include "videoTransport.h"
#include "frameQueue.h"

#define DEBUG 0

VideoTransport::VideoTransport(FrameQueue *fq) :
	frameQueue(fq), current_frame(0)
{
	lastTransportStatus = Fwd1;
	looping=true;
	transportFwd1();

	current_repeats_todo = 0;
	current_frame_field_num = 0;
	repeats = 1;
	repeats_rem = 0;
	ignore_interlaced_when_stepping = 0;
}

void VideoTransport::setLooping(bool l)
{
	looping = l;
}

int VideoTransport::getSpeed()
{
	TransportControls ts = transportStatus;

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
	TransportControls ts = transportStatus;

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

/* advance the transport.  This may or maynot move to a new frame period.
 * Returns true if moving to a new frame period */
bool VideoTransport::advance()
{
	int currentSpeed = getSpeed();
	int currentDirection = getDirection();

	bool do_interlace = 1;

	//stop after each jog
	TransportControls ts = transportStatus;
	if (ts == JogFwd || ts == JogRev) {
		transportStop();
		/* a jog is a request to move to the next frame/field immediately
		 * discard any repeats in this case */
		current_repeats_todo = 0;
		/* if using an interlaced source but not deinterlacing,
		 * don't display each frame twice when stepping */
		do_interlace = !ignore_interlaced_when_stepping;
	}

	if (!current_frame) {
		/* getting a first frame is more important than waiting for nothing */
		current_frame = frameQueue->getNextFrame(currentSpeed, currentDirection);
		if (!current_frame)
			return false;
	} else {
		/* soak up any repeats */
		if (current_repeats_todo > 1) {
			current_repeats_todo--;
			return false;
		}

		/* if there are insufficient repeats to do interlace, don't do it */
		do_interlace &= repeats > 1;

		/* advance by speed, counting in fields/frames */
		int field_delta = current_frame_field_num + currentSpeed*currentDirection;
		int frame_delta = field_delta >> (do_interlace & current_frame->isInterlaced);
		/* if we wanted to have a notional target frame number, it would be:
		 *    frame_num += frame_delta;
		 */
		if (frame_delta)
			current_frame = frameQueue->getNextFrame(currentSpeed, currentDirection);

		/* recalculate field position taking into account that any nextframe
		 * may be interlaced differently to the previous */
		current_frame_field_num =
		current_frame->fieldNum = field_delta
		                        & (int) (do_interlace & current_frame->isInterlaced);
	}

	bool is_new_frame_period = true;

	/* update the number of times to repeat this new frame/field */
	if (repeats_rem) {
		/* use any repeats left over from the previous iteration (field) */
		current_repeats_todo = repeats_rem;
		repeats_rem = 0;
		/* this is the second filed in the frame period */
		is_new_frame_period = false;
	}
	else if (current_frame->isInterlaced) {
		/* this is a new frame period with an interlaced frame
		 * share the number of repeats for this frame period
		 * between two field periods */
		/* interlaced handling is done by effectively stealing
		 * a factor of 2 from repeats, and using it to display two fields */
		current_repeats_todo = repeats / 2;
		repeats_rem = repeats - current_repeats_todo;
	}
	else {
		/* this is a new frame period with a progressive frame */
		current_repeats_todo = repeats;
	}

	//stop at first or last frame if there is no looping - will break at speeds other than 1x
	if (current_frame) {
		if ((looping == false) && (current_frame->isLastFrame == true
		        || current_frame->isFirstFrame == true)) {
			transportStop();
			emit(endOfFile());
		}
	}

	//wake the reader thread - done each time as jogging will move the frame number
	frameQueue->wake();

	return is_new_frame_period;
}

VideoData *VideoTransport::getFrame()
{
	return current_frame;
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

	currentStatus = transportStatus;

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
		transportStatus = newStatus;
		if (DEBUG)
			printf("Changed transport state to %d\n", (int)transportStatus);
	}

}
