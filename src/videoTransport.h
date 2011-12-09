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

#include <vector>

#include <QObject>
#include <QSemaphore>
#include <QWaitCondition>

class VideoData;
class FrameQueue;
class ReaderInterface;

class FrameNumCounter {
public:
	FrameNumCounter()
		: repeats(1)
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
	void setRepeats(int r) { repeats = r; }
	bool advance();

	int getCurrentFrameNum() { return current_frame_number; }
	int getCurrentField() { return current_field; }

private:
	/* number of times to repeat each frame */
	int repeats;

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

class VideoTransport : public QObject
{
	Q_OBJECT

public:
	VideoTransport(ReaderInterface *r, int read_ahead=16, int lru_cache_len=16);
	void setLooping(bool l) { looping = l; }
	void setRepeats(unsigned r) { current_frame_num.setRepeats(r); }
	void setSpeed(int s);
	/* if using an interlaced source but not deinterlacing, set the following */
	void setSteppingIgnoreInterlace(bool b) { ignore_interlaced_when_stepping = b; }

	/* access must be serialised with respect to advance() */
	VideoData* getFrame(); //the frame that is currently being displayed
	bool advance(); //advance to the next frame (in the correct directon @ speed).

protected:

	signals:
	void endOfFile(void);

public slots:
	//slots to connect keys / buttons to play and scrub controls
	void transportFwd100();
	void transportFwd50();
	void transportFwd20();
	void transportFwd10();
	void transportFwd5();
	void transportFwd2();
	void transportFwd1();
	void transportStop();
	void transportRev1();
	void transportRev2();
	void transportRev5();
	void transportRev10();
	void transportRev20();
	void transportRev50();
	void transportRev100();

	//frame jog controls
	void transportJogFwd();
	void transportJogRev();

	//play / pause
	void transportPlayPause();
	void transportPause();

public:
	FrameQueue *frame_queue;

	VideoData* output_frame;

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

	int num_listeners;
	bool advance_ok;

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
