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

#include <QtCore>
#include <iostream>

#include "videoData.h"
#include "frameQueue.h"
#include "videoTransport.h"
#include "readerInterface.h"

using namespace std;

FrameQueue::~FrameQueue()
{
	stop = true;
	vt.frame_queue_wake.wakeAll();
	wait();
}

void FrameQueue::run()
{
	stop = false;

	while (!stop) {
		/* todo: clean up */
		vt.future_frame_num_list_head_idx_semaphore.acquire();

		QMutexLocker locker(&frame_map_mutex);
		/* find the next frame in the list to be loaded */
		bool do_load = false;
		int future_list_len = vt.future_frame_num_list.size();
		int frame_num;
		int last_idx = 0;
		for (int i = 0; i < future_list_len; i++) {
			last_idx = i;
			frame_num = vt.future_frame_num_list[(vt.future_frame_num_list_head_idx+i) % future_list_len];
			if (frame_map.find(frame_num) == frame_map.end()) {
				do_load = 1;
				break;
			}
		}
		/* remove anything that is in the future list from the LRU list */
		for (int i = 0; i < future_list_len; i++) {
			int future_frame_num = vt.future_frame_num_list[(vt.future_frame_num_list_head_idx+i) % future_list_len];
			frame_map_lru.remove(future_frame_num);
		}
		std::stringstream ss; ss << last_idx << "/" << future_list_len << " (" << frame_map.size() << ")";
		stats->addStat("Future", ss.str());
		locker.unlock();

		vt.future_frame_num_list_head_idx_semaphore.release();

		if (!do_load) {
			/* sleep */
			QMutex m; m.lock();
			vt.frame_queue_wake.wait(&m);
			m.unlock();
			continue;
		}
		/* load the frame, append frame num to LRU list for culling later */
		VideoData *v = reader->pullFrame(frame_num);
		locker.relock();
		frame_map[frame_num] = v;
		frame_map_lru.push_back(frame_num);

		/* purge excess frames from the queue */
		int num_frames_to_discard = frame_map_lru.size() - frame_map_lru_cache_maxlen;
		ss.str(""); ss << num_frames_to_discard << "/" << frame_map_lru.size();
		stats->addStat("LRU discard", ss.str());
		for (int i = 0; i < num_frames_to_discard; i++) {
			int frame_to_delete = frame_map_lru.front();
			frame_map_lru.pop_front();
			frame_map_t::iterator it = frame_map.find(frame_to_delete);
			delete it->second;
			frame_map.erase(it);
		}

		locker.unlock();
	}
}

VideoData* FrameQueue::getFrame(int frame_num)
{
	QMutexLocker locker(&frame_map_mutex);

	frame_map_t::iterator it = frame_map.find(frame_num);
	if (it == frame_map.end()) {
		/* not found */
		return NULL;
	}

	/* update the LRU status */
	frame_map_lru.remove(frame_num);
	frame_map_lru.push_back(frame_num);

	return it->second;
	/* todo: handle EOF */
}
