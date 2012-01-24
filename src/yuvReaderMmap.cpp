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

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/mman.h>
#endif

#ifdef Q_OS_MAC
#define lseek64 lseek
#define off64_t off_t
#define mmap64 mmap
#endif

#include "videoData.h"
#include "yuvReaderMmap.h"
#include "stats.h"
#include "util.h"

#define DEBUG 0

struct DataPtr_mmap : public DataPtr {
	DataPtr_mmap(void *data, unsigned length)
		: length(length)
	{
		ptr = data;
		void *aligned_ptr = (void*)((unsigned long)ptr & ~4095);
		size_t aligned_length = length + ((unsigned long)ptr - (unsigned long)aligned_ptr);
		/* inform the OS that we do need the pages */
		madvise(aligned_ptr, aligned_length, MADV_WILLNEED);
	};

	~DataPtr_mmap() {
		/* only free entire pages that we nolonger cover */
		/* todo: handle overlapping pages */

		void *first_complete_page_ptr = (void*) (((unsigned long)ptr + 4095) & ~4095);
		size_t compete_pages_len = (length - ((unsigned long)first_complete_page_ptr - (unsigned long)ptr)) & ~4095;
		/* inform the OS that we don't need the pages anymore */
		madvise(first_complete_page_ptr, compete_pages_len, MADV_DONTNEED);
	};

private:
	unsigned length;
};

YUVReaderMmap::YUVReaderMmap()
{
	randomAccess = true;
	interlacedSource = false;
}

void YUVReaderMmap::setFileName(const QString &fn)
{
	fileName = fn;
	QFileInfo info(fileName);

	if (DEBUG)
		printf("Opening file %s\n", fileName.toLatin1().data());
	fd = open(fileName.toLatin1().data(), O_RDONLY);

	if (fd < 0 && DEBUG == 1) {
		printf("[%s]\n", fileName.toLatin1().data());
		perror("OPEN");
	}

	base_ptr = (char *)mmap64(0, info.size(), PROT_READ, MAP_PRIVATE, fd, 0);

	fileType = forceFileType ? fileType.toLower() : info.suffix().toLower();
	printf("Playing file with type %s\n", fileType.toLatin1().data());

	FourCC fourcc = qstringToFourCC(fileType);
	packing_format = fourccToPacking(fourcc);
	chroma_format = fourccToChroma(fourcc);
	frame_size = sizeofFrame(packing_format, chroma_format, videoWidth, videoHeight);

	firstFrameNum = 0;
	lastFrameNum = info.size() / frame_size;
	lastFrameNum--;
}

//called from the frame queue controller to get frame data for display
VideoData* YUVReaderMmap::pullFrame(int frameNumber)
{
	if (DEBUG)
		printf("Getting frame number %d\n", frameNumber);

	//deal with frame number wrapping
	if (frameNumber < 0) {
		frameNumber *= -1;
		frameNumber %= (lastFrameNum + 1);
		frameNumber = lastFrameNum - frameNumber;
	}

	if (frameNumber > lastFrameNum)
		frameNumber %= (lastFrameNum + 1);

	/* allocate new storage:
	 *  1) work out dimensions for the planes
	 *  2) create contiguous storage (we already know the frame_size)
	 *  3) create aliases for any other planes
	 */
	VideoData* frame = new VideoData();
	frame->data.packing_format = packing_format;
	frame->data.chroma_format = chroma_format;
	setPlaneDimensions(*(PictureData<void>*)&frame->data, packing_format, chroma_format, videoWidth, videoHeight);

	//calculate offset
	off64_t offset = (off_t)frame_size * (off_t)frameNumber; //seek to the wanted frame
	void* const data = base_ptr + offset;

	/* the virtual read */
	int idletime = timer.restart();
	using namespace std::tr1;
	frame->data.plane[0].data = shared_ptr<DataPtr>(new DataPtr_mmap(data, frame_size));
	int readtime = timer.restart();

	uint8_t* ptr = (uint8_t*) data;
	for (unsigned i = 1; i < frame->data.plane.size(); i++) {
		ptr += frame->data.plane[i-1].length;
		frame->data.plane[i].data = shared_ptr<DataPtr>(new DataPtr_alias(ptr));
	}
	/* xxx: fixup plane numbering if required (eg YV12 vs I420) */

	//set frame number and first/last flags
	frame->frame_number = frameNumber;
	frame->is_first_frame = (frameNumber == firstFrameNum);
	frame->is_last_frame = (frameNumber == lastFrameNum);
	frame->is_interlaced = interlacedSource;

	frame->sample_aspect_ratio_numerator = sample_aspect_ratio_numerator;
	frame->sample_aspect_ratio_denominator = sample_aspect_ratio_denominator;
	frame->data.sar = (float)sample_aspect_ratio_numerator / (float)sample_aspect_ratio_denominator;

	Stats::Section& stats = Stats::getSection("YUV Reader(mmap)");
	addStat(stats, "Read", readtime, "ms");
	addStat(stats, "VideoWidth", videoWidth);
	addStat(stats, "VideoHeight", videoHeight);
	addStat(stats, "FirstFrame", firstFrameNum);
	addStat(stats, "LastFrame", lastFrameNum);
	addStat(stats, "VideoFormat", fileType.toLatin1().data());
	if (0) {
		addStat(stats, "Peak Rate", frame_size / (readtime * 1024), "MiB/s");
		addStat(stats, "Mean Rate", frame_size / ((readtime + idletime) * 1024), "MiB/s");
	}

	return frame;
}
