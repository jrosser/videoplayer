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

#include <QtGui>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

#ifdef Q_OS_WIN32
#include <io.h>
#define lseek64 _lseeki64
#define read _read
#define open _open
#define off64_t qint64
#endif

#ifdef Q_OS_MAC
#define lseek64 lseek
#define off64_t off_t
#endif

#include "yuvReader.h"

#define DEBUG 0

YUVReader::YUVReader(FrameQueue& frameQ) :
	ReaderInterface(frameQ)
{
	randomAccess = true;
}

void YUVReader::setForceFileType(bool f)
{
	forceFileType = f;
}

void YUVReader::setFileType(const QString &t)
{
	fileType = t;
}

void YUVReader::setVideoWidth(int w)
{
	videoWidth = w;
}

void YUVReader::setVideoHeight(int h)
{
	videoHeight = h;
}

void YUVReader::setFileName(const QString &fn)
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

	QString type = forceFileType ? fileType.toLower() : info.suffix().toLower();
	printf("Playing file with type %s\n", type.toLatin1().data());

	firstFrameNum = 0;

	if (type == "16p0") {
		videoFormat = VideoData::V16P0;
		lastFrameNum = info.size() / ((videoWidth * videoHeight * 3 * 2) / 2);
		lastFrameNum--;
	}

	if (type == "16p2") {
		videoFormat = VideoData::V16P2;
		lastFrameNum = info.size() / (videoWidth * videoHeight * 2 * 2);
		lastFrameNum--;
	}

	if (type == "16p4") {
		videoFormat = VideoData::V16P4;
		lastFrameNum = info.size() / (videoWidth * videoHeight * 3 * 2);
		lastFrameNum--;
	}

	if (type == "420p") {
		videoFormat = VideoData::V8P0;
		lastFrameNum = info.size() / ((videoWidth * videoHeight * 3) / 2);
		lastFrameNum--;
	}

	if (type == "422p") {
		videoFormat = VideoData::V8P2;
		lastFrameNum = info.size() / (videoWidth * videoHeight * 2);
		lastFrameNum--;
	}

	if (type == "444p") {
		videoFormat = VideoData::V8P4;
		lastFrameNum = info.size() / (videoWidth * videoHeight * 3);
		lastFrameNum--;
	}

	if (type == "i420") {
		videoFormat = VideoData::V8P0;
		lastFrameNum = info.size() / ((videoWidth * videoHeight * 3) / 2);
		lastFrameNum--;
	}

	if (type == "yv12") {
		videoFormat = VideoData::YV12;
		lastFrameNum = info.size() / ((videoWidth * videoHeight * 3) / 2);
		lastFrameNum--;
	}

	if (type == "uyvy") {
		videoFormat = VideoData::UYVY;
		lastFrameNum = info.size() / (videoWidth * videoHeight * 2);
		lastFrameNum--;
	}

	if (type == "v216") {
		videoFormat = VideoData::V216;
		lastFrameNum = info.size() / (videoWidth * videoHeight * 4);
		lastFrameNum--;
	}

	if (type == "v210") {
		videoFormat = VideoData::V210;
		lastFrameNum = info.size() / ((videoWidth * videoHeight * 2 * 4) / 3);
		lastFrameNum--;
	}
}

//called from the frame queue controller to get frame data for display
void YUVReader::pullFrame(int frameNumber, VideoData*& dst)
{
	VideoData* frame = frameQueue.allocateFrame();
	frame->resize(videoWidth, videoHeight, videoFormat);

	if (DEBUG)
		printf("Getting frame number %d\n", frameNumber);

	//deal with frame number wrapping
	if (frameNumber < 0) {
		frameNumber *= -1;
		frameNumber %= lastFrameNum;
		frameNumber = lastFrameNum - frameNumber;
	}

	if (frameNumber > lastFrameNum)
		frameNumber %= lastFrameNum;

	//set frame number and first/last flags
	frame->frameNum = frameNumber;
	frame->isFirstFrame = (frameNumber == firstFrameNum);
	frame->isLastFrame = (frameNumber == lastFrameNum);
	dst = frame;

	//seek
	off64_t offset = (off_t)dst->dataSize * (off_t)frameNumber; //seek to the wanted frame
	off64_t sret = lseek64(fd, offset, SEEK_SET);
	if (!sret && offset != 0)
		perror("LSEEK");

	//read
	int rret = read(fd, dst->data, dst->dataSize);
	if (!rret)
		perror("READ");
}
