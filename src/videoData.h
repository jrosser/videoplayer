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

#ifndef VIDEODATA_H_
#define VIDEODATA_H_

#include <vector>
#include <memory>
#if __cplusplus <= 199711L
# if !defined(_MSC_VER) || _MSC_VER < 1600
#  include <tr1/memory>
# endif
#endif


#include <GL/glew.h>

#include "fourcc.h"

/* virtual base */
struct DataPtr {
	virtual ~DataPtr() {} ;
	void* ptr;
};

struct DataPtr_valloc : public DataPtr {
	DataPtr_valloc(unsigned length);
	~DataPtr_valloc();
};

/* alias some other data. */
struct DataPtr_alias : public DataPtr {
	DataPtr_alias(void* in_ptr);
	/* No destructor -- this is an alias */
};

namespace GLvideo {
	enum PackingFmt {
		V8P,  /*  8bit planar */
		V16P, /* 16bit planar */
		UYVY, /*  8bit packed [Cb,Y',Cr,Y'] */
		V216, /* 16bit packed [Cb,Y',Cr,Y'] */
		V210, /* 10bit packed [special] */
	};

	enum ChromaFmt {
		/* Todo, this should be more like h.264 annex E, fig 1.
		 * Ie, represent chroma sample positions */
		Cr420,
		Cr422,
		Cr444,
	};
};

template<typename T>
struct PictureData {
	GLvideo::PackingFmt packing_format;
	GLvideo::ChromaFmt chroma_format;
	template<typename T1>
	struct Plane {
		std::tr1::shared_ptr<T1> data;
		unsigned width; /* notional width of data */
		unsigned height;
		unsigned length; /* meaning of length is dependant upon T1 */
	};
	std::vector<Plane<T> > plane;
};

struct GLvideo_data {
	PictureData<DataPtr> data;
	PictureData<GLuint> gl_data;

	bool is_interlaced;
	bool is_top_field_first;
	bool is_field1;
};

struct VideoData : public GLvideo_data {
	unsigned long frame_number;
	bool is_first_frame;
	bool is_last_frame;
};

GLvideo::PackingFmt fourccToPacking(FourCC fourcc);
GLvideo::ChromaFmt fourccToChroma(FourCC fourcc);

unsigned sizeofFrame(GLvideo::PackingFmt packing, GLvideo::ChromaFmt chroma ,unsigned width, unsigned height);

PictureData<void>&
setPlaneDimensions(PictureData<void>& pd, GLvideo::PackingFmt packing, GLvideo::ChromaFmt chroma, unsigned width, unsigned height);

void convertPlanar16toPlanar8(PictureData<DataPtr>& pd);
void convertV210toUYVY(PictureData<DataPtr>& pd);
void convertV216toUYVY(PictureData<DataPtr>& pd);
#endif
