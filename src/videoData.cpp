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

#include <QtGlobal>

#include <stdlib.h>

#ifdef Q_OS_WIN32
#include <malloc.h>
#define valloc malloc
#endif

#include "videoData.h"

VideoData::VideoData()
{
	//make a bogus 1x1 4:4:4 8 bit image
	allocate(1, 1, V8P4);
	data[0] = 16;
	data[1] = 128;
	data[2] = 128;
}

void VideoData::resize(int w, int h, DataFmt f)
{
	if (w != Ywidth || h != Yheight || f != diskFormat) {
		if (data)
			free(data);
		allocate(w, h, f);
	}

	/* Some type conversions don't need to reallocate but
	 * do change the render format, so there is diskformat
	 * space avaliable but only renderformat used. */
	renderFormat = f;
}

void VideoData::allocate(int w, int h, DataFmt f)
{
	Ywidth = w;
	Yheight = h;
	diskFormat = f;

	switch (diskFormat) {

	case V8P4: //8 bit 444 planar
		Cwidth = Ywidth;
		Cheight = Yheight;
		YdataSize = Ywidth * Yheight;
		UdataSize = YdataSize;
		VdataSize = YdataSize;
		break;

	case V16P4: //16 bit 444 planar
		Cwidth = Ywidth;
		Cheight = Yheight;
		YdataSize = Ywidth * Yheight * 2;
		UdataSize = YdataSize;
		VdataSize = YdataSize;
		break;

	case V8P2: //8 bit 422 planar
		Cwidth = Ywidth / 2;
		Cheight = Yheight;
		YdataSize = Ywidth * Yheight;
		UdataSize = YdataSize / 2;
		VdataSize = YdataSize / 2;
		break;

	case V16P2: //16 bit 422 planar
		Cwidth = Ywidth / 2;
		Cheight = Yheight;
		YdataSize = Ywidth * Yheight * 2;
		UdataSize = YdataSize / 2;
		VdataSize = YdataSize / 2;
		break;

	case V16P0: //16 bit 420
		Cwidth = Ywidth / 2;
		Cheight = Yheight / 2;
		YdataSize = Ywidth * Yheight * 2;
		UdataSize = YdataSize / 4;
		VdataSize = YdataSize / 4;
		break;

	case V8P0: //8 bit 420, YUV
	case YV12: //8 bit 420, YVU
		Cwidth = Ywidth / 2;
		Cheight = Yheight / 2;
		YdataSize = Ywidth * Yheight;
		UdataSize = YdataSize / 4;
		VdataSize = YdataSize / 4;
		break;

	case UYVY:
		Cwidth = Ywidth / 2;
		Cheight = Yheight;
		YdataSize = Ywidth * Yheight * 2;
		UdataSize = 0;
		VdataSize = 0;
		break;

	case V216:
		Cwidth = Ywidth / 2;
		Cheight = Yheight;
		YdataSize = Ywidth * Yheight * 4;
		UdataSize = 0;
		VdataSize = 0;
		break;

	case V210:
		//3 10 bit components packed into 4 bytes
		Cwidth = Ywidth / 2;
		Cheight = Yheight;
		YdataSize = (Ywidth * Yheight * 2 * 4) / 3;
		UdataSize = 0;
		VdataSize = 0;
		break;

	default:
		//oh-no!
		break;
	}

	//total size of the frame
	dataSize = YdataSize + UdataSize + VdataSize;

	//get some nicely page aligned memory
	data = (unsigned char *)valloc(dataSize);
	Ydata = data;
	isPlanar = false;
	glMinMaxFilter = GL_LINEAR; //linear interpolation
	Udata = Ydata;
	Vdata = Ydata;
	renderFormat = diskFormat;

	switch (diskFormat) {
	//planar formats YUV
	case V16P4:
	case V16P2:
	case V16P0:
	case V8P4:
	case V8P2:
	case V8P0:
		glYTextureWidth = Ywidth;
		glInternalFormat = GL_LUMINANCE;
		glFormat = GL_LUMINANCE;
		glType = GL_UNSIGNED_BYTE;
		isPlanar = true;
		Udata = Ydata + YdataSize;
		Vdata = Udata + UdataSize;
		break;

		//planar format, YVU
	case YV12:
		isPlanar = true;
		glYTextureWidth = Ywidth;
		glInternalFormat = GL_LUMINANCE;
		glFormat = GL_LUMINANCE;
		glType = GL_UNSIGNED_BYTE;
		//U&V components are exchanged
		Vdata = Ydata + (Ywidth * Yheight);
		Udata = Vdata + ((Ywidth * Yheight) / 4);
		break;

	case UYVY:
		//muxed 8 bit format
		glYTextureWidth = Ywidth / 2; //2 Y samples per RGBA quad
		glInternalFormat = GL_RGBA;
		glFormat = GL_RGBA;
		glType = GL_UNSIGNED_BYTE;
		break;

	case V210: //gets converted to UYVY
		glYTextureWidth = Ywidth / 2; //2 Y samples per RGBA quad
		glInternalFormat = GL_RGBA;
		glFormat = GL_RGBA;
		glType = GL_UNSIGNED_BYTE;
		break;

	case V216:
		glYTextureWidth = Ywidth / 2; //2 Y samples per RGBA quad
		glInternalFormat = GL_RGBA;
		glFormat = GL_RGBA;
		glType = GL_UNSIGNED_SHORT;
		break;

	default:
		//oh-no!
		break;
	}
}

VideoData::~VideoData()
{
	if (data) {
		free(data);
		data = NULL;
	}
}
;

typedef unsigned int uint_t;
typedef unsigned char uint8_t;

//read the first 10 bit sample from the least significant bits of the 4 bytes pointed to by 'data'
inline uint_t readv210sample_pos2of3(uint8_t* data)
{
	const uint_t lsb = (data[2] & 0xf0) >> 4;
	const uint_t msb = (data[3] & 0x3f) << 4;
	return msb | lsb;
}

//read the second 10 bit sample from the middle bits of the 4 bytes pointed to by 'data'
inline uint_t readv210sample_pos1of3(uint8_t* data)
{
	const uint_t lsb = (data[1] & 0xfc) >> 2;
	const uint_t msb = (data[2] & 0x0f) << 6;
	return msb | lsb;
}

//read the third 10 bit sample from the more significant bits of the 4 bytes pointed to by 'data'
inline uint_t readv210sample_pos0of3(uint8_t* data)
{
	const uint_t lsb = (data[0] & 0xff);
	const uint_t msb = (data[1] & 0x03) << 8;
	return msb | lsb;
}

void unpackv210line(uint8_t* dst, uint8_t* src, uint_t luma_width)
{
	/* number of blocks completely filled with active samples (6 per block) */
	const uint_t num_firstpass_samples = 2*(luma_width/6)*6;
	uint_t x;
	for (x=0; x < num_firstpass_samples; src += 16) {
		dst[x++] = readv210sample_pos0of3(src + 0) >> 2; /* Cb */
		dst[x++] = readv210sample_pos1of3(src + 0) >> 2; /* Y' */
		dst[x++] = readv210sample_pos2of3(src + 0) >> 2; /* Cr */
		dst[x++] = readv210sample_pos0of3(src + 4) >> 2; /* Y' */

		dst[x++] = readv210sample_pos1of3(src + 4) >> 2;
		dst[x++] = readv210sample_pos2of3(src + 4) >> 2;
		dst[x++] = readv210sample_pos0of3(src + 8) >> 2;
		dst[x++] = readv210sample_pos1of3(src + 8) >> 2;

		dst[x++] = readv210sample_pos2of3(src + 8) >> 2;
		dst[x++] = readv210sample_pos0of3(src + 12) >> 2;
		dst[x++] = readv210sample_pos1of3(src + 12) >> 2;
		dst[x++] = readv210sample_pos2of3(src + 12) >> 2;
	}
	/* mop up last subblock (4 or 2 active samples) */

	if (x < 2*luma_width) {
		dst[x++] = readv210sample_pos0of3(src + 0) >> 2; /* Cb */
		dst[x++] = readv210sample_pos1of3(src + 0) >> 2; /* Y' */
		dst[x++] = readv210sample_pos2of3(src + 0) >> 2; /* Cr */
		dst[x++] = readv210sample_pos0of3(src + 4) >> 2; /* Y' */
	}

	if (x < 2*luma_width) {
		dst[x++] = readv210sample_pos1of3(src + 4) >> 2;
		dst[x++] = readv210sample_pos2of3(src + 4) >> 2;
		dst[x++] = readv210sample_pos0of3(src + 8) >> 2;
		dst[x++] = readv210sample_pos1of3(src + 8) >> 2;
	}
}

//utility function to convert V210 data into something easier to display
//it is too difficult to do this in the openGL shader
void VideoData::convertV210()
{
	/* pad to a multiple of 48 luma samples */
	/* nb, 48 luma samples becomes 128 bytes */
	const uint_t padded_w = ((Ywidth + 47)/48) * 48; //number of luma samples, padded
	const uint_t padded_line_length = (2*padded_w*4) / 3; //number of bytes on each line in the file, including padding data

	uint8_t *dst = (uint8_t*) data; /* destination for 8bit uyvy, may = src */
	uint8_t *src = (uint8_t*) data; /* source of v210 data */

	for (int y=0; y<Yheight; y++) {
		unpackv210line(dst, src, Ywidth);
		src += padded_line_length;
		dst += Ywidth*2;
	}

	//change the description of the data to match UYVY
	//note that the dataSize stays the same
	renderFormat = UYVY;
	glYTextureWidth = Ywidth / 2; //2 Y samples per RGBA quad
	YdataSize = Ywidth * Yheight * 2;
	UdataSize = 0;
	VdataSize = 0;
}

void VideoData::convertPlanar16()
{
	uint8_t *src=(uint8_t *)Ydata;
	uint8_t *dst=(uint8_t *)Ydata;
	const uint8_t *end=(uint8_t *)(Ydata+dataSize);

	//convert the 16 bit source to 8 bit destination, in place
	//just throw away the LSBs
	while (src < end) {
		*dst++ = *src;
		src+=2;
	}

	//new data size
	YdataSize = Ywidth * Yheight;

	//tell the renderer that this is now 8 bit data
	switch (renderFormat) {
	case V16P0:
		renderFormat = V8P0;
		UdataSize = YdataSize/4;
		VdataSize = YdataSize/4;
		break;

	case V16P2:
		renderFormat = V8P2;
		UdataSize = YdataSize/2;
		VdataSize = YdataSize/2;
		break;

	case V16P4:
		renderFormat = V8P4;
		UdataSize = YdataSize;
		VdataSize = YdataSize;
		break;

	default:
		//doh!
		break;
	}

	//update pointers to the start of each plane
	Udata = Ydata + YdataSize;
	Vdata = Udata + UdataSize;
}