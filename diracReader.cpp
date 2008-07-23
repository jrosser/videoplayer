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
#include <sstream>

#include <schroedinger/schro.h>

#include "diracReader.h"
#include "stats.h"

#include "schro_parse.c"

#define DEBUG 0

/* Read a chunk of data from file @fd@, using the sync state to hint
 * at the size of the chunk required */
SchroBuffer*
readChunk(FILE* fd, SchroSyncState* s)
{
	unsigned char pi[13];
	int next_du_size = 512;
	int tmp_len = 0; /* either 0 or 13 (if we've tried to read a PI) */

	if (SYNCED == s->state) {
		/* If synced, there is a possibility that we are stream aligned,
		 * so try and read an aligned PI and work out the DU length; */
		tmp_len = fread(pi, 13, 1, fd);
		if (tmp_len && pi[0] == 'B' && pi[1] == 'B' && pi[2] == 'C' && pi[3] == 'D')
			next_du_size = pi[5] << 24 | pi[6] << 16 | pi[7] << 8 | pi[8];
		tmp_len *= 13;
	}
	if (SYNCED_INCOMPLETEDU == s->state) {
		/* We are synced and not stream aligned (haven't read enough data
		 * to finish this DU), read enough data to finish this DU */
		next_du_size = s->offset - schro_buflist_length(s->bufs);
	}
	SchroBuffer* b = schro_buffer_new_and_alloc(next_du_size);
	memcpy(b->data, pi, tmp_len);
	b->length = fread(b->data + tmp_len, 1, next_du_size - tmp_len, fd) + tmp_len;
	if (!b->length) {
		schro_buffer_unref(b);
		return 0;
	}
	return b;
}

/* Allocate storage for a frame of data (using the framequeue to
 * allocate suitably aligned data */
SchroFrame*
DiracReader::allocFrame()
{
	SchroFrame *f = NULL;
	VideoData* v = frameQueue.allocateFrame();
	assert(v);
	assert(vidfmt);

	static const VideoData::DataFmt chromafmt_to_datafmt[] = {
		VideoData::V8P4,
		VideoData::V8P2,
		VideoData::V8P0,
	};

	typedef SchroFrame* (*SchroFrameNewFromData_t)(void*,int,int);
	static const SchroFrameNewFromData_t schro_frame_new_from_data[] = {
		schro_frame_new_from_data_u8_planar444,
		schro_frame_new_from_data_u8_planar422,
		schro_frame_new_from_data_I420,
	};

	v->resize(vidfmt->width, vidfmt->height
	         ,chromafmt_to_datafmt[vidfmt->chroma_format]);

	f = schro_frame_new_from_data[vidfmt->chroma_format](v->data, vidfmt->width, vidfmt->height);

	assert(f);
	f->priv = v;
	return f;
}

/* Add frame @video@ to the output queue.
 * Attempting to add a frame to a full queue blocks this function
 * (and thus regulates the speed of decoding) */
#define MAX_QUEUE_LEN 10
void
DiracReader::queueFrame(VideoData* video)
{
	Stats &stat = Stats::getInstance();
	std::stringstream ss;

	//wait for space in the list
	frameMutex.lock();
	int size = frameList.size();
	ss.str("");
	ss << size << "/" << MAX_QUEUE_LEN;
	//stat.addStat("DiracReader", "OutQueueLen", ss.str());

	if (size == MAX_QUEUE_LEN) {
		bufferNotFull.wait(&frameMutex);
	}

	frameList.prepend(video);
	bufferNotEmpty.wakeAll();
	frameMutex.unlock();
}

/* called from the transport controller to get frame data for display
 * frame number is bogus, as the dirac wrapper cannot seek by frame number */
void
DiracReader::pullFrame(int /*frameNumber*/, VideoData*& dst)
{
	//see if there is a frame on the list
	frameMutex.lock();
	if (frameList.empty()) {
		bufferNotEmpty.wait(&frameMutex);
	}

	dst = frameList.takeLast();
	//dst->isLastFrame = 0;
	dst->isFirstFrame = 0;
	//dst->frameNum = frameNumber;

	bufferNotFull.wakeAll();
	frameMutex.unlock();
}

DiracReader::DiracReader(FrameQueue& frameQ) :
	ReaderInterface(frameQ),
	vidfmt(0)
{
	randomAccess = false;
}

void
DiracReader::setFileName(const QString &fn)
{
	fileName = fn;
}

void
DiracReader::stop()
{
	go=false;
	wait();
}

void
DiracReader::run()
{
	Stats &stat = Stats::getInstance();

	schro_init();
	SchroDecoder* schro = schro_decoder_new();
	SchroSyncState sync_state = {NOT_SYNCED, 0, -1, };

	stat.addStat("DiracReader", "SchroState", "STARTED");

	go = true;
	while (fileName.isEmpty()) sleep(1);

	FILE* fd = NULL;
	fd = fopen(fileName.toAscii().constData(), "rb");
	bool last_frame = 0;

	static const char* decoderstate_to_str[] = {
		"SCHRO_DECODER_OK", "SCHRO_DECODER_ERROR", "SCHRO_DECODER_EOS",
		"SCHRO_DECODER_FIRST_ACCESS_UNIT", "SCHRO_DECODER_NEED_BITS",
		"SCHRO_DECODER_NEED_FRAME", "SCHRO_DECODER_WAIT",
		"SCHRO_DECODER_STALLED",
	};

	do {
		if (feof(fd))
			/* just in case the file didn't have an end of stream */
			schro_decoder_push_end_of_stream(schro);

		int state = schro_decoder_wait(schro);

		//stat.addStat("DiracReader", "SchroState", decoderstate_to_str[state]);
		//stat.addStat("DiracReader", "SyncState", syncstate_to_str[sync_state.state]);

		switch (state) {
		case SCHRO_DECODER_NEED_BITS: {
			SchroBuffer *b = readChunk(fd, &sync_state);
			b = schro_parse_getnextdu(&sync_state, b);
			if (!b) break;
			state = schro_decoder_push(schro, b);
			if (state == SCHRO_DECODER_FIRST_ACCESS_UNIT) {
				if (vidfmt)
					free(vidfmt);
				vidfmt = schro_decoder_get_video_format(schro);
			}
			break;
		}
		case SCHRO_DECODER_NEED_FRAME: {
			SchroFrame *f = allocFrame();
			schro_decoder_add_output_picture(schro, f);
			break;
		}
		case SCHRO_DECODER_OK: {
			int pnum = schro_decoder_get_picture_number(schro);
			SchroFrame *f = schro_decoder_pull(schro);
			((VideoData*)f->priv)->frameNum = pnum;
			((VideoData*)f->priv)->isLastFrame = last_frame;
			last_frame = 0;
			queueFrame((VideoData*)f->priv);
			f->priv = NULL;
			schro_frame_unref(f);
			break;
		}
		case SCHRO_DECODER_EOS:
			if(feof(fd)) {
				rewind(fd);
				last_frame = 1;
			}
			schro_decoder_reset(schro);
			break;
		case SCHRO_DECODER_ERROR:
			fprintf(stderr, "some error\n");
			return;
		}
	} while(go);

	stat.addStat("DiracReader", "SchroState", "-ETERM");

	schro_decoder_free(schro);
	if (vidfmt)
		free(vidfmt);
}
