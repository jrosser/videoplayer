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

#ifndef VIDEOTRANSPORT_H
#define VIDEOTRANSPORT_H

#include <list>
#include <vector>

#include <QSemaphore>
#include <QWaitCondition>

struct VideoData;
class FrameQueue;
class ReaderInterface;

class FrameNumCounter {
public:
	FrameNumCounter()
		: fps_out(1.)
		, fps_src(1.)
		, fps_accum(0.)
		, is_interlaced(0)
		, current_frame_number(0)
		, current_field(0)
		, current_repeats_todo(0)
		, current_repeats_field1(0)
		, transport_speed(1)
		, steady_state(0)
	{}

	void setFrameNum(int num) { current_frame_number = num; steady_state = false; }
	void setInterlaced(bool interlaced) { is_interlaced = interlaced; }
	void setSpeed(int speed) { transport_speed = speed; }
	void setRepeats(int r) { fps_out = 1.; fps_src = 1./r; }
	void setOutFPS(double fps) { fps_out = fps; }
	void setSrcFPS(double fps) { fps_src = fps; }
	bool advance();

	int getCurrentFrameNum() { return current_frame_number; }
	int getCurrentField() { return current_field; }
	double getSrcFPS() { return fps_src; }

private:
	/* number of times to repeat each frame */
	double fps_out;
	double fps_src;
	double fps_accum;

	/* current frame is interlaced: causes any repeats to be
	 * divided into two field periods */
	bool is_interlaced;

	/* frame number for the current state machine point */
	int current_frame_number;
	int current_field;

	/* number of times left to repeat the current frame */
	int current_repeats_todo;

	/* when interlaced, number of leftover repeats after previous field */
	int current_repeats_field1;

	/* speed at wich frames should be advanced */
	int transport_speed;

	bool steady_state;
};

class VideoTransport
{
public:
	VideoTransport(const std::list<ReaderInterface*>& rs, int read_ahead=16, int lru_cache_len=16);
	~VideoTransport();

	void setLooping(bool l) { looping = l; }
	void setRepeats(unsigned r) { current_frame_num.setRepeats(r); }
	void setSpeed(int s, bool jog = 0);
	void setVDUfps(double fps) { current_frame_num.setOutFPS(fps); }
	/* if using an interlaced source but not deinterlacing, set the following */
	void setSteppingIgnoreInterlace(bool b) { ignore_interlaced_when_stepping = b; }

	/* access must be serialised with respect to advance() */
	VideoData* getFrame(int queue); //the frame that is currently being displayed
	ReaderInterface* getReader(int queue);

	int getNumFutureFrames();

	bool advance(); //advance to the next frame (in the correct directon @ speed).

	void transportPlay(int speed);
	void transportStop();
	void transportJogFwd();
	void transportJogRev();
	void transportPlayPause();

protected:
	virtual void notifyEndOfFile() {};

public:
	std::vector<FrameQueue*> frame_queues;
	std::vector<VideoData*> output_frames;

	/* modify behaviour of advance() when single stepping interlaced content
	 * (but no deinterlacing is occuring) */
	bool ignore_interlaced_when_stepping;

	bool looping;

	/**
	 * A circular list of frame numbers that FrameQueues should load in the
	 * future.
	 * NB: the list shall always be fully populated.
	 * NB: Access to the head_idx boundary variable must be accessed with a
	 *     semaphore.  This is required so that the VideoTransport can purge
	 *     the list when the playback speed or direction changes
	 */
	/* internally, the head of the list is the next frame that we wish to display */
	std::vector<int> future_frame_num_list;

	/* serial number of the future_frame_num_list, updated whenever it is rewriten */
	int future_frame_num_list_serial;

	QWaitCondition frame_queue_wake;

	/**
	 * The start/end point of the list.  Accesses to this variable must occur
	 * when holding the semaphore.
	 */
	int future_frame_num_list_head_idx;

	/**
	 * Semaphore to guard access to future_frame_num_list_boundary
	 * Initialized with num_listeners = num_frame_queues + 1
	 */
	QSemaphore future_frame_num_list_head_idx_semaphore;

	/* must be kept synchronised */
	FrameNumCounter future_frame_num_counter;
	FrameNumCounter current_frame_num;

	/**
	 * playback speed of the video transport.
	 * In future this will be fractional to allow framerate matching
	 */
	int transport_speed;
	int transport_speed_prev;
	bool transport_jog;

	/**
	 * do not advance the transport automatically if set
	 */
	bool transport_pause;

	/**
	 * sufficient frames have been cached at startup to provide a
	 * reasonable guarantee of not stuttering */
	bool transport_startup_done;

	int num_listeners;
	bool advance_ok;

	double prev_fps;

	/**
	 * direction of playback
	 */
#if 0
	enum Direction {
		Forward = 1,
		Paused = 0,
		None = 0,
		Reverse = -1,
	};
	Direction transport_direction;
#endif
};

#endif
