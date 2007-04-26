/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: videoRead.cpp,v 1.10 2007-04-26 16:09:51 davidf Exp $
*
* Version: MPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Mozilla Public License
* Version 1.1 (the "License"); you may not use this file except in compliance
* with the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
* the specific language governing rights and limitations under the License.
*
* The Original Code is BBC Research and Development code.
*
* The Initial Developer of the Original Code is the British Broadcasting
* Corporation.
* Portions created by the Initial Developer are Copyright (C) 2004.
* All Rights Reserved.
*
* Contributor(s): Jonathan Rosser (Original Author)
*
* Alternatively, the contents of this file may be used under the terms of
* the GNU General Public License Version 2 (the "GPL"), or the GNU Lesser
* Public License Version 2.1 (the "LGPL"), in which case the provisions of
* the GPL or the LGPL are applicable instead of those above. If you wish to
* allow use of your version of this file only under the terms of the either
* the GPL or LGPL and not to allow others to use your version of this file
* under the MPL, indicate your decision by deleting the provisions above
* and replace them with the notice and other provisions required by the GPL
* or LGPL. If you do not delete the provisions above, a recipient may use
* your version of this file under the terms of any one of the MPL, the GPL
* or the LGPL.
* ***** END LICENSE BLOCK ***** */

#include "videoRead.h"

#define DEBUG 0

VideoRead::VideoRead() 
	: readThread(*this)
{	
	lastTransportStatus = Fwd1;
	currentFrameNum=0;
	firstFrameNum=0;
	lastFrameNum=0;
	
	displayFrame = NULL;
	frameMutex.lock();	//required for QWaitCondition
	readThread.start();
	
	transportMutex.lock();
	transportStatus = Fwd1;
	transportSpeed = 1;
	transportMutex.unlock();	
}

void VideoRead::stop()
{
	readThread.stop();
	frameMutex.unlock();
	frameConsumed.wakeOne();
	
	readThread.wait();
}

void VideoRead::setVideoWidth(int width)
{
	videoWidth = width;
}

void VideoRead::setVideoHeight(int height)
{
	videoHeight = height;
}

void VideoRead::setFileName(const QString &fn)
{
    QMutexLocker fileInfoLocker(&fileInfoMutex);
	fileName = fn;
	
	QFileInfo info(fn);
	
	//fixme - this is a nasty assumption that .yuv is i420
	if(info.suffix() == "YUV" || info.suffix() == "yuv") {
		dataFormat = VideoData::I420;
		
		lastFrameNum = info.size() / ((videoWidth * videoHeight * 3) / 2);
		lastFrameNum--; 	
	}

	if(info.suffix() == "I420" || info.suffix() == "i420") {
		dataFormat = VideoData::I420;

		lastFrameNum = info.size() / ((videoWidth * videoHeight * 3) / 2);
		lastFrameNum--; 						
	}

	if(info.suffix() == "YV12" || info.suffix() == "yv12") {
		dataFormat = VideoData::YV12;

		lastFrameNum = info.size() / ((videoWidth * videoHeight * 3) / 2);
		lastFrameNum--; 			
	}

	if(info.suffix() == "UYVY" || info.suffix() == "uyvy") {
		dataFormat = VideoData::UYVY;

		lastFrameNum = info.size() / (videoWidth * videoHeight * 2);
		lastFrameNum--; 			
	}

	if(info.suffix() == "V216" || info.suffix() == "v216") {
		dataFormat = VideoData::V216;
		
		lastFrameNum = info.size() / (videoWidth * videoHeight * 4);
		lastFrameNum--; 			
	}

	if(info.suffix() == "V210" || info.suffix() == "v210") {
		dataFormat = VideoData::V210;

		lastFrameNum = info.size() / ((videoWidth * videoHeight * 2 * 4) / 3);
		lastFrameNum--; 			
	}
	
}

//called from the openGL display widget to get frame data for display
VideoData* VideoRead::getNextFrame()
{
	//get the status of the transport
	QMutexLocker transportLocker(&transportMutex);
	TransportControls ts = transportStatus;
	transportLocker.unlock();
	
	//paused or stopped - get a frame if there is none
	if((ts == Pause || ts == Stop) && displayFrame == NULL) {
		ts = JogFwd;
	}
	
	//forwards
	if(ts == Fwd1 || ts == Fwd2 || ts == Fwd5 || ts == Fwd10 || ts == Fwd20 || ts == Fwd50 || ts == Fwd100 || ts == JogFwd) {
						
		if(futureFrames.size() == 0)
			printf("Dropped frame - no future frame available\n");
		else
		{				
			QMutexLocker listLocker(&listMutex);
			
			//playing forward
			if(displayFrame != NULL)
				pastFrames.prepend(displayFrame);
			
			displayFrame = futureFrames.takeFirst();				
			currentFrameNum = displayFrame->frameNum;				
		}
		
		if(ts == JogFwd) {
			transportLocker.relock();
			transportStatus = Stop;
			transportLocker.unlock();				
		}		
	}
	
	//backwards	
	if(ts == Rev1 || ts == Rev2 || ts == Rev5 || ts == Rev10 || ts == Rev20 || ts == Rev50 || ts == Rev100 || ts == JogRev) {
											
		if(pastFrames.size() == 0)
			printf("Dropped frame - no past frame available\n");
		else {
				QMutexLocker listLocker(&listMutex);
			
			//playing backward
			if(displayFrame != NULL)
				futureFrames.prepend(displayFrame);
							
			displayFrame = pastFrames.takeFirst();				
			currentFrameNum = displayFrame->frameNum;								
		}

		if(ts == JogRev) {
			transportLocker.relock();
			transportStatus = Stop;
			transportLocker.unlock();				
		}		
	}
	
	//wake the reader thread - done each time as jogging may move the frame number
	frameConsumed.wakeOne();
		
	return displayFrame;
}

void VideoRead::transportFwd100() {transportController(Fwd100);}
void VideoRead::transportFwd50() {transportController(Fwd50);}
void VideoRead::transportFwd20() {transportController(Fwd20);}
void VideoRead::transportFwd10() {transportController(Fwd10);}
void VideoRead::transportFwd5() {transportController(Fwd5);}
void VideoRead::transportFwd2() {transportController(Fwd2);}
void VideoRead::transportFwd1() {transportController(Fwd1);}
void VideoRead::transportStop() {transportController(Stop);}
void VideoRead::transportRev1() {transportController(Rev1);}
void VideoRead::transportRev2() {transportController(Rev2);}
void VideoRead::transportRev5() {transportController(Rev5);}
void VideoRead::transportRev10() {transportController(Rev10);}
void VideoRead::transportRev20() {transportController(Rev20);}
void VideoRead::transportRev50() {transportController(Rev50);}
void VideoRead::transportRev100() {transportController(Rev100);}
void VideoRead::transportJogFwd() {transportController(JogFwd);}
void VideoRead::transportJogRev() {transportController(JogRev);}
void VideoRead::transportPlayPause() {transportController(PlayPause);}
void VideoRead::transportPause() {transportController(Pause);}

void VideoRead::transportController(TransportControls in)
{
	TransportControls newStatus = Unknown;
	TransportControls currentStatus;
	
	transportMutex.lock();
	currentStatus = transportStatus;
	int newSpeed  = transportSpeed;
	transportMutex.unlock();
	
	switch(in) {
		case Stop:
			newStatus = Stop;
			lastTransportStatus = Fwd1;	//after 'Stop', unpausing makes us play forwards
			newSpeed = 1;				//at 1x speed
			break;
			
		case JogFwd:
			if(currentStatus == Stop || currentStatus == Pause) {
				newStatus = JogFwd; //do jog forward	
			}
			break;
			
		case JogRev:
			if(currentStatus == Stop || currentStatus == Pause) {
				newStatus = JogRev; //do jog backward	
			}
			break;
			
		case Pause:
 			//when not stopped or paused we can pause, remebering what were doing		
			if(currentStatus != Stop || currentStatus == Pause) {
				lastTransportStatus = currentStatus;
				newStatus = Pause;	
			}
			break;
			
		case PlayPause:
		 	//PlayPause toggles between stop and playing, and paused and playing. It
 			//remembers the direction and speed were going
 			if(currentStatus == Stop || currentStatus == Pause) {
 								
 				if(lastTransportStatus == Stop)
 					newStatus = Fwd1;
 				else
 					newStatus = lastTransportStatus;
 					
 			} else { 				
 				lastTransportStatus = currentStatus;
 				newStatus = Pause;	
 			}
 			break;

 		//we can change from any state to a 'shuttle' or play 			
 		case Fwd100:
 		case Rev100:
 			newSpeed = 100;
 			newStatus = in;
 			break;	
 			 		
 		case Fwd50:
 		case Rev50:
 		 	newSpeed = 50;
 			newStatus = in;
 			break;	
 		 		
 		case Fwd20:
 		case Rev20:
 		 	newSpeed = 20;
 			newStatus = in;
 			break;	
 		 		
 		case Fwd10:
 		case Rev10:
 		 	newSpeed = 10;
 			newStatus = in;
 			break;	
 		 		
 		case Fwd5:
 		case Rev5:
 		 	newSpeed = 5;
 			newStatus = in;
 			break;	
 		 		
 		case Fwd2:
 		case Rev2:
 		 	newSpeed = 2;
 			newStatus = in;
 			break;	
 		 		
 		case Fwd1:
 		case Rev1:
			newSpeed = 1;
 			newStatus = in;
 			break;	
 			
 		default:
 			//oh-no!
			break;
	}
	
	//if we have determined a new transport status, set it
	if(newStatus != Unknown) {
		transportMutex.lock();
		transportStatus = newStatus;
		transportSpeed = newSpeed;
		transportMutex.unlock();
		if(DEBUG) printf("Changed transport state to %d, speed %d\n", (int)transportStatus, transportSpeed);		
	}
	

	
}
