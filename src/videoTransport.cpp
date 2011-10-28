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
	bool do_interlace = is_interlaced & (repeats > 1);
	bool is_new_frame_period = true;

	if (steady_state) {
		/* soak up any repeats */
		if (current_repeats_todo > 1) {
			current_repeats_todo--;
			return false;
		}

		/* advance by speed, counting in fields/frames */
		int field_delta = current_field + transport_speed;
		int frame_delta = field_delta >> do_interlace;

		current_frame_number += frame_delta;
		current_field = field_delta & do_interlace;

		is_new_frame_period = frame_delta;
	}
	steady_state = true;

	/* update the number of times to repeat this new frame/field */
	if (current_repeats_field1) {
		/* use any repeats left over from the previous iteration (field) */
		/* this is the second field in the frame period */
		current_repeats_todo = current_repeats_field1;
		current_repeats_field1 = 0;
	}
	else if (do_interlace) {
		/* this is a new frame period with an interlaced frame
		 * share the number of repeats for this frame period
		 * between two field periods */
		/* interlaced handling is done by effectively stealing
		 * a factor of 2 from repeats, and using it to display two fields */
		current_repeats_todo = repeats / 2;
		current_repeats_field1 = repeats - current_repeats_todo;
	}
	else {
		/* this is a new frame period with a progressive frame */
		current_repeats_todo = repeats;
	}

	return is_new_frame_period;
}

bool VideoTransport::advance()
{
#if 0
	/* modify state if we have been asked to jog a single frame */
	if (transport_jog != None) {
		/* a jog is a request to move to the next frame/field immediately
		 * discard any repeats in this case */
		current_repeats_todo = 0;
		/* if using an interlaced source but not deinterlacing,
		 * don't display each frame twice when stepping */
		do_interlace = !ignore_interlaced_when_stepping;
	}
#endif

	bool paused = transport_pause && !transport_jog;
	if (paused) {
		return false;
	}

	future_frame_num_list_head_idx_semaphore.acquire();

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
		/* todo: set interlace flag in output */
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
			eof = false;
			if (!frame)
				advance_ok = false;
		} catch (...) {
			/* eof: advancing in this case is ok, this video will just
			 * be NULL until all queues reach eof */
		}
		next_output[i] = frame;
	}

	if (eof) {
		/* only handle the eof case when all input files have reached eof */
		if (looping) {
			//reset frame count
		} else {
			transportStop();
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
		future_frame_num_list[future_frame_num_list_head_idx] = future_frame_num_counter.getCurrentFrameNum();

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
