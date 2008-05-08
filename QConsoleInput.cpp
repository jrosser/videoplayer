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
#include <QApplication>
#include <QObject>
#include <QKeyEvent>

#ifdef Q_OS_UNIX
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#endif

#include "QConsoleInput.h"

QConsoleInput::QConsoleInput(QObject* in_event_handler)
{
	event_handler = in_event_handler;
	start();
}

int
asciiToKey(char c)
{
	if (!isprint(c))
		return 0;
	return toupper(c);
}

Qt::KeyboardModifiers
asciiToMod(char c)
{
	Qt::KeyboardModifiers mod = Qt::NoModifier;
	if (isupper(c))
		mod |= Qt::ShiftModifier;
	return mod;
}

void
QConsoleInput::run()
{
	if (!isatty(STDIN_FILENO))
		/* we can only do this if we are on a tty */
		return;

	/* Protect against reading while backgrounded
	 * (causes read to return -EIO) */
	signal(SIGTTIN, SIG_IGN);
	/* hack: flag set when we get -EIO on read */
	/* No signal is generated to inform a process if it suddenly
	 * becomes the foreground process, so some hack is required */
	bool hack_got_eio;

	/* put the terminal into cbreak mode, aka character mode, so that read()
	 * doesn't block waiting for a newline before returning */
	struct termios tio;
	tio_orig = new termios;
	tcgetattr(STDIN_FILENO, tio_orig);
	tcgetattr(STDIN_FILENO, &tio);
	tio.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &tio);

	while (1) {
		if (hack_got_eio && tcgetpgrp(STDIN_FILENO) == getpid()) {
			tcgetattr(STDIN_FILENO, &tio);
			tio.c_lflag &= ~(ICANON | ECHO);
			tcsetattr(STDIN_FILENO, TCSANOW, &tio);
			hack_got_eio = 0;
		}

		char c = 'a';
		if (read(STDIN_FILENO, &c, 1) <= 0) {
			sleep(1);
			hack_got_eio = 1;
			continue;
		}

		Qt::KeyboardModifiers mods = asciiToMod(c);
		int key = asciiToKey(c);

		QKeyEvent *ev = new QKeyEvent(QEvent::KeyPress, key, mods);
#if QT_VERSION >= 0x040400
		if (event_handler->isWidgetType())
			QApplication::setActiveWindow(reinterpret_cast<QWidget*>(event_handler));
#endif
		QCoreApplication::postEvent(event_handler, ev);
	}
}

QConsoleInput::~QConsoleInput()
{
	terminate();
	wait();

	signal(SIGTTIN, SIG_DFL);

	/* restore tty mode at exit */
	if (!isatty(STDIN_FILENO))
		return;

	tcsetattr(STDIN_FILENO, TCSANOW, tio_orig);

	delete tio_orig;
}
