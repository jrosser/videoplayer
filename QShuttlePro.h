/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: QShuttlePro.h,v 1.8 2008-02-05 00:12:06 asuraparaju Exp $
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

#ifndef QSHUTTLEPRO_H
#define QSHUTTLEPRO_H

#include <QtGui>

#include "pthread.h"

class QShuttlePro : public QThread
{
    Q_OBJECT

public:

    QShuttlePro();
    void stop();

signals:
    void jogForward();
    void jogBackward();

    void shuttleChanged(int);
    void shuttleCenter();
    void shuttleLeft1();
    void shuttleLeft2();
    void shuttleLeft3();
    void shuttleLeft4();
    void shuttleLeft5();
    void shuttleLeft6();
    void shuttleLeft7();
    void shuttleRight1();
    void shuttleRight2();
    void shuttleRight3();
    void shuttleRight4();
    void shuttleRight5();
    void shuttleRight6();
    void shuttleRight7();

    void keyPressed(int);
    void keyReleased(int);
    void keyChanged(int, int);

    void key256Pressed();
    void key257Pressed();
    void key258Pressed();
    void key259Pressed();
    void key260Pressed();
    void key261Pressed();
    void key262Pressed();
    void key263Pressed();
    void key264Pressed();
    void key265Pressed();
    void key266Pressed();
    void key267Pressed();
    void key268Pressed();
    void key269Pressed();
    void key270Pressed();

private:

    typedef struct
    {
        short vendor;
        short product;
        char* name;
    } ShuttleData;

    int fd;
    struct timeval lastshuttle;
    int need_shuttle_center;
    unsigned int jogvalue;
    int shuttlevalue;

    void run();
    bool running;
    pthread_t self;

    int openShuttle();
    void process_event(struct input_event);
    void jog(unsigned int value);
    void shuttle(int value);
    void key(unsigned int value, unsigned int code);
    void check_shuttle_center();
};

#endif
