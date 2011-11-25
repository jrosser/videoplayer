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
#include <list>
#include <vector>
#include <stdio.h>

#include "videoData.h"
#include "videoTransport.h"
#include "frameQueue.h"
#include "readerInterface.h"

#define DEBUG 0

using namespace std;

#if 0
class SomeEOF : public exception {
public:
	explicit SomeEOF(const string& msg);
	const char* what() const throw();
};
#endif

bool
FrameNumCounter::advance() {
	/* if there are insufficient repeats to do interlace, don't do it */
	bool do_interlace = is_interlaced & (fps_out/fps_src >= 2.);

	if (!steady_state) {
		/* initial frame */
		steady_state = true;
		return true;
	}

	fps_accum += fps_src * transport_speed;
	int num_frames_to_advance = fps_accum / fps_out;
	fps_accum -= num_frames_to_advance * fps_out;
	current_frame_number += num_frames_to_advance;

	/* field polarity for interlaced systems: first half of frame interval = field 0 */
	current_field = 0;
	if (do_interlace && 2.0 * fps_accum >= fps_out) {
		current_field = 1;
	}

	return num_frames_to_advance != 0;
}

bool VideoTransport::advance()
{
	bool paused = transport_pause && !transport_jog;
	if (paused) {
		return false;
	}

	future_frame_num_list_head_idx_semaphore.acquire();

	current_frame_num.setSrcFPS(frame_queue->getReader()->getFPS(current_frame_num.getCurrentFrameNum()));

	if (!advance_ok) {
		/* don't advance the frame counter: we have unfinished business from
		 * the previous frame period */
	}
	else if (transport_jog) {
		transport_jog = 0; /* only jog once */
		/* skip to new frames only (ignoring repeats) when jogging */
		while (!current_frame_num.advance());
	}
	else if (!current_frame_num.advance()) {
		/* no need to load a new frame, still reusing the old one */
		for (int i = 0; i < 1 /* num_frame_queues */; i++) {
			if (!output_frame)
				continue;
			/* update the field polarity of the current output frame to
			 * handle the case where we jump from one field to the next
			 * of the same frame period */
			/* todo: this isn't exactly nice, it would be better to
			 * return a new field period and create a slave picture
			 * with the new field information */
			output_frame->fieldNum = current_frame_num.getCurrentField();
		}
		future_frame_num_list_head_idx_semaphore.release();
		return false;
	}

	int current_frame = future_frame_num_list[future_frame_num_list_head_idx];

	/* sanity check: verify that current_frame is expected */
	assert(current_frame == current_frame_num.getCurrentFrameNum());
	VideoData *next_output[1];

	/* extract next frame from the FrameQueue.  If the FrameQueue has
	 * not been able to load the required frame, it will return NULL.
	 * In such cases, the frame period is not advanced */
	advance_ok = true; /* set to false if a frame queue failed to provide a frame */
	bool eof = true; /* set to false if any frame_queue is not at eof */
	for (int i = 0; i < 1 /*num_frame_queues*/; i++) {
		VideoData *frame = NULL;
		try {
			frame = frame_queue->getFrame(current_frame);
			if (!frame) {
				advance_ok = false;
			} else {
				frame->fieldNum = current_frame_num.getCurrentField();
				eof &= frame->isLastFrame;
			}
		} catch (...) {
			/* eof: advancing in this case is ok, this video will just
			 * be NULL until all queues reach eof */
		}
		next_output[i] = frame;
	}

	if (advance_ok && eof && !looping) {
		/* only handle the eof case when all input files have reached eof */
		if (looping) {
			//reset frame count
		} else {
			//transportStop();
			transport_pause = 1; // fixme: transportStop() causes things to hang.
			emit(endOfFile());
		}
		/* todo: tail recurse, so that frames can be returned after this
		 * advance call */
		advance_ok = false;
	}

	if (advance_ok) {
		/* this frame has been completed, time to move to the next */
		future_frame_num_list_head_idx_semaphore.acquire(num_listeners-1);

		/* advance the read-ahead frame num counter, to the next frame to load */
		while (!future_frame_num_counter.advance());
		int next_frame_num  = future_frame_num_counter.getCurrentFrameNum();
		future_frame_num_list[future_frame_num_list_head_idx] = next_frame_num;

		future_frame_num_list_head_idx += 1;
		future_frame_num_list_head_idx %= future_frame_num_list.size();

		future_frame_num_list_head_idx_semaphore.release(num_listeners-1);

		/* copy the frames that we have fetched from the queues to the
		 * output list.  this ensures that the output list always refers
		 * to the same frame period. */
		for (int i = 0; i < 1 /*um_frame_queues*/; i++) {
			output_frame = next_output[i];
		}
	}
	future_frame_num_list_head_idx_semaphore.release();
	/* wake the reader threads in case any are blocked on a full list */
	frame_queue_wake.wakeAll();

	return advance_ok;
}

void VideoTransport::setSpeed(int new_speed, bool jog)
{
	transport_speed = new_speed;

	/* 1. grab the semaphore */
	future_frame_num_list_head_idx_semaphore.acquire(num_listeners);

	/* 2. reset the future frame num counter to "now", update with the
	 * new speed and repopulate the future framenum list */
	current_frame_num.setSpeed(new_speed);
	future_frame_num_counter = current_frame_num;

	//future_frame_num_list_serial++; /* allow frame queues to spot a change */
	future_frame_num_list_head_idx = 0;
	for (unsigned i = 0; i < future_frame_num_list.size(); i++) {
		if (advance_ok || i > 0)
			/* on the first iteration of the loop, don't advance the counter
			 * if the current frame failed to load -- we need to cause it to reload */
			while (!future_frame_num_counter.advance());
		future_frame_num_list[i] = future_frame_num_counter.getCurrentFrameNum();
	}

	/* propagate the jog flag */
	transport_jog = jog;

	/* release the semaphore for FrameQueues to continue */
	future_frame_num_list_head_idx_semaphore.release(num_listeners);
}


VideoTransport::VideoTransport(ReaderInterface *r, int read_ahead, int lru_cache_len)
	: output_frame(0)
	, future_frame_num_list(read_ahead, 0)
	, future_frame_num_list_head_idx_semaphore(2)
	, transport_pause(0)
	, transport_jog(0)
{
	num_listeners = 2; /* 1 framequeue + this */

	current_frame_num.setInterlaced(true);

	advance_ok =1;
	looping=true;
	/* preload the initial future frame list with an obvious default */
	setSpeed(1);

	frame_queue = new FrameQueue(*this, lru_cache_len);
	frame_queue->setReader(r);
	frame_queue->start();
}

VideoData *VideoTransport::getFrame()
{
	return output_frame;
}

#define TRANSPORT_MODE(name, speed) \
	void VideoTransport::transport ## name () { transport_jog = transport_pause = 0; setSpeed(speed); }
TRANSPORT_MODE(Fwd100, 100);
TRANSPORT_MODE(Fwd50, 50);
TRANSPORT_MODE(Fwd20, 20);
TRANSPORT_MODE(Fwd10, 10);
TRANSPORT_MODE(Fwd5, 5);
TRANSPORT_MODE(Fwd2, 2);
TRANSPORT_MODE(Fwd1, 1);
TRANSPORT_MODE(Rev1, -1);
TRANSPORT_MODE(Rev2, -2);
TRANSPORT_MODE(Rev5, -5);
TRANSPORT_MODE(Rev10, -10);
TRANSPORT_MODE(Rev20, -20);
TRANSPORT_MODE(Rev50, -50);
TRANSPORT_MODE(Rev100, -100);

void VideoTransport::transportStop()
{
	transport_speed_prev = 1; /* in case pause/play is used to resume */
	transport_pause = 1;
	setSpeed(1);
}

void VideoTransport::transportJogFwd()
{
	if (!transport_pause)
		return;
	setSpeed(1, true);
}

void VideoTransport::transportJogRev()
{
	if (!transport_pause)
		return;
	setSpeed(-1, true);
}

void VideoTransport::transportPlayPause()
{
	if (!transport_pause) {
		transport_pause = 1;
		transport_speed_prev = transport_speed;
	} else {
		transport_pause = 0;
		setSpeed(transport_speed_prev);
	}
}
