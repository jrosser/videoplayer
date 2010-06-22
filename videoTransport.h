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

#ifndef VIDEOTRANSPORT_H
#define VIDEOTRANSPORT_H

#include <QtGui>

#include "videoData.h"

class FrameQueue;

class VideoTransport : public QObject
{
	Q_OBJECT

public:

	enum TransportControls {
		Rev100 = 0,
		Rev50,
		Rev20,
		Rev10,
		Rev5,
		Rev2,
		Rev1,
		PlayPause,
		Pause,
		Stop,
		Fwd1,
		Fwd2,
		Fwd5,
		Fwd10,
		Fwd20,
		Fwd50,
		Fwd100,
		JogFwd,
		JogRev,
		Unknown}; //transport control status

	VideoTransport(FrameQueue *fq);
	void setLooping(bool l);
	void setRepeats(unsigned) {}

	VideoData* getFrame(); //the frame that is currently being displayed
	void advance(); //advance to the next frame (in the correct directon @ speed).
	int getDirection(); //return the current transport direction -1 reverse, 0 stopped/paused, 1 forward.
	int getSpeed();

protected:

	signals:
	void endOfFile(void);

public slots:
	//slots to connect keys / buttons to play and scrub controls
	void transportFwd100();
	void transportFwd50();
	void transportFwd20();
	void transportFwd10();
	void transportFwd5();
	void transportFwd2();
	void transportFwd1();
	void transportStop();
	void transportRev1();
	void transportRev2();
	void transportRev5();
	void transportRev10();
	void transportRev20();
	void transportRev50();
	void transportRev100();

	//frame jog controls
	void transportJogFwd();
	void transportJogRev();

	//play / pause
	void transportPlayPause();
	void transportPause();

private:
	void transportController(TransportControls c);
	FrameQueue *frameQueue;
	VideoData *current_frame;
	bool looping;

	TransportControls transportStatus; //what we are doing now - shared with readThread
	TransportControls lastTransportStatus; //what we were doing last
};

#endif
