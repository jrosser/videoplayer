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

#ifndef READERINTERFACE_H_
#define READERINTERFACE_H_

#include <QString>

struct VideoData;

class ReaderInterface {
public:
	ReaderInterface()
	: fps(1.0f)
	, sample_aspect_ratio_numerator(1)
	, sample_aspect_ratio_denominator(1)
	{}

	virtual ~ReaderInterface()
	{
	}

	virtual VideoData* pullFrame(int wantedFrame) = 0;

	virtual void setSAR(int n, int d) { sample_aspect_ratio_numerator = n; sample_aspect_ratio_denominator = d; }
	virtual void setFPS(double x) { fps = x; }
	virtual double getFPS(int) { return fps; }

	virtual QString getCaption(int) { return ""; }

	bool randomAccess;

private:
	ReaderInterface(ReaderInterface const&);
	ReaderInterface& operator=(ReaderInterface const&);

protected:
	double fps;
	int sample_aspect_ratio_numerator;
	int sample_aspect_ratio_denominator;
};

#endif
