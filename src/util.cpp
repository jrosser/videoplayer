/* ***** BEGIN LICENSE BLOCK *****
 *
 * The MIT License
 *
 * Copyright (c) 2010 BBC Research
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
#include <QString>
#include "fourcc.h"

FourCC
qstringToFourCC(QString& s)
{
	if (s == "16p0")
		return FOURCC_16p0;
	if (s == "16p2")
		return FOURCC_16p2;
	if (s == "16p4")
		return FOURCC_16p4;
    if (s == "10p0")
        return FOURCC_10p0;
    if (s == "10p2")
        return FOURCC_10p2;
    if (s == "10p4")
        return FOURCC_10p4;
	if (s == "420p")
		return FOURCC_420p;
	if (s == "422p")
		return FOURCC_422p;
	if (s == "444p")
		return FOURCC_444p;
	if (s == "i420")
		return FOURCC_i420;
	if (s == "yv12")
		return FOURCC_yv12;
	if (s == "uyvy")
		return FOURCC_uyvy;
	if (s == "v216")
		return FOURCC_v216;
	if (s == "v210")
		return FOURCC_v210;

	assert(!"unknown fourcc");
	return 0;
}
