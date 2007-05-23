/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: videoRead.h,v 1.8 2007-05-23 13:41:06 jrosser Exp $
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

#ifndef VIDEOREAD_H
#define VIDEOREAD_H

#include <QtGui>

#include "readThread.h"
#include "videoData.h"
#include "GLvideo_rt.h"
class VideoRead : public QObject
{
    Q_OBJECT

public:

	enum TransportControls { Rev100, Rev50, Rev20, Rev10, Rev5, Rev2, Rev1, PlayPause, Pause, Stop, Fwd1, Fwd2, Fwd5, Fwd10, Fwd20, Fwd50, Fwd100, JogFwd, JogRev, Unknown }; //transport control status

	VideoRead();
	void setFileName(const QString &fn);
	void setVideoWidth(int width);
	void setVideoHeight(int height);
	void setForceFileType(bool flag);
	void setFileType(const QString &t);
	void stop();
	
	QList<VideoData *> pastFrames;
	QList<VideoData *> futureFrames;
	QMutex listMutex;					//protect the lists of frames
		
	int currentFrameNum;
	int firstFrameNum;
	int lastFrameNum;
	
	int videoWidth;					//input file
	int videoHeight;	
	QString fileName;
	VideoData::DataFmt dataFormat;	//data format type
	QMutex fileInfoMutex;			//protect the file information
				
	QWaitCondition frameConsumed;	//condition variable to pause/wake reading thread, synchronising display rate and data reading thread	
	QMutex frameMutex;		

	TransportControls transportStatus;	//what we are doing now - shared with readThread
	int transportSpeed;
	QMutex transportMutex;				//protect the transport status

	VideoData* getNextFrame(void);	//the frame that is currently being displayed, this pointer is only manipulated from the read thread
	int getDirection();	            //return the current transport direction -1 reverse, 0 stopped/paused, 1 forward.
			
protected:

private slots:
	void transportController(TransportControls c);

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
	ReadThread readThread;
	VideoData *displayFrame;				//a pointer to the video data for the frame being displayed
	
	bool forceFileType;
	QString fileType;
		
	TransportControls lastTransportStatus;	//what we were doing before 'pause'
};

#endif
