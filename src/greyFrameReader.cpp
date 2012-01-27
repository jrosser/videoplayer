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


#include "greyFrameReader.h"
#include "videoData.h"

#include <memory>

using namespace std::tr1;

VideoData *GreyFrameReader::getGreyFrame()
{
	VideoData* frame = new VideoData();
	setPlaneDimensions(*(PictureData<void>*)&frame->data, GLvideo::V8P, GLvideo::Cr444, 1, 1);
	static uint8_t data[] = { 0x80u };
	frame->data.packing_format = GLvideo::V8P;
	frame->data.chroma_format = GLvideo::Cr444;
	frame->data.plane[0].data = shared_ptr<DataPtr>(new DataPtr_alias(data));
	frame->data.plane[1].data = shared_ptr<DataPtr>(new DataPtr_alias(data));
	frame->data.plane[2].data = shared_ptr<DataPtr>(new DataPtr_alias(data));

	frame->data.sar = 1.0f;
	frame->frame_number = 0;
	frame->is_first_frame = 0;
	frame->is_last_frame = 0;
	frame->is_interlaced = 0;

	return frame;
}

GreyFrameReader::GreyFrameReader()
{
	randomAccess = true;
}

VideoData* GreyFrameReader::pullFrame(int frameNumber)
{
	return getGreyFrame();
}
