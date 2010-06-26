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

#ifndef YUVREADERMMAP_H_
#define YUVREADERMMAP_H_

#include "readerInterface.h"
#include "videoData.h"

class YUVReaderMmap : public ReaderInterface {
public:
	//from ReaderInterface
	YUVReaderMmap(FrameQueue& frameQ);
	virtual VideoData* pullFrame(int wantedFrame);

public:
	//specific to yuvReader
	void setForceFileType(bool f) { forceFileType = f; }
	void setFileType(const QString &t) { fileType = t; }
	void setVideoWidth(int w) { videoWidth = w; }
	void setVideoHeight(int h) { videoHeight = h; }
	void setFileName(const QString &fn);
	void setInterlacedSource(bool x) { interlacedSource = x; }

private:

	//information about the video data
	VideoData::DataFmt videoFormat;
	QString fileType;
	QString fileName;
	bool forceFileType;
	bool interlacedSource;
	int videoWidth;
	int videoHeight;

	//video file
	int fd;
	char *base_ptr;
	int lastFrameNum;
	int firstFrameNum;

	//bandwidth counter
	QTime timer;
};

#endif
