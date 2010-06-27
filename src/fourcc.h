
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

#ifndef FOURCC_H_
#define FOURCC_H_

#include <stdint.h>
typedef uint32_t FourCC;

#define FOURCC(a,b,c,d) \
	( ((uint32_t) a << 24) | \
	  ((uint32_t) b << 16) | \
	  ((uint32_t) c <<  8) | \
	  d )

#define FOURCC_8p0  FOURCC(' ','8','p','0')
#define FOURCC_420p FOURCC('4','2','0','p')
#define FOURCC_i420 FOURCC('i','4','2','0')
#define FOURCC_yv12 FOURCC('y','v','1','2')
#define FOURCC_8p2  FOURCC(' ','8','p','2')
#define FOURCC_422p FOURCC('4','2','2','p')
#define FOURCC_8p4  FOURCC(' ','8','p','4')
#define FOURCC_444p FOURCC('4','4','4','p')
#define FOURCC_16p0 FOURCC('1','6','p','0')
#define FOURCC_16p2 FOURCC('1','6','p','2')
#define FOURCC_16p4 FOURCC('1','6','p','4')
#define FOURCC_uyvy FOURCC('u','y','v','y')
#define FOURCC_v216 FOURCC('v','2','1','6')
#define FOURCC_v210 FOURCC('v','2','1','0')

#endif
