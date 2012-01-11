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

#ifndef FRAMEQUEUE_H_
#define FRAMEQUEUE_H_

#include <QtCore>
#include <map>
#include <list>

class ReaderInterface;
class VideoTransport;
class VideoData;

class FrameQueue : public QThread {
public:
	FrameQueue(VideoTransport& vt, int lru_cache_len=16)
		: QThread()
		, vt(vt)
		, frame_map_lru_cache_maxlen(lru_cache_len)
	{ }

	~FrameQueue();

	void run();

	VideoData *allocateFrame(void);

	void setReader(ReaderInterface *r)
	{
		reader = r;
	}

	VideoData *getFrame(int frame_num);

private:
	bool stop;

	//the reader
	ReaderInterface *reader;

	/* the video transport we are a queue for */
	VideoTransport& vt;

	/* */
	typedef std::map<int, VideoData*> frame_map_t;
	frame_map_t frame_map;
	std::list<int> frame_map_lru;
	int frame_map_lru_cache_maxlen;

	QMutex frame_map_mutex;
};

#endif
