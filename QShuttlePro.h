/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: QShuttlePro.h,v 1.2 2007-04-10 15:24:02 jrosser Exp $
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
* Contributor(s): Jonathan Rosser 
* Inspired by IngexPlayer
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

#ifndef QSHUTTLEPRO_H
#define QSHUTTLEPRO_H

#include <QtGui>

class QShuttlePro : public QThread
{
    Q_OBJECT

public:

	QShuttlePro();
			
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
	int need_synthetic_shuttle;
	unsigned int jogvalue;
	int shuttlevalue;

	void run();
	int openShuttle();
	void process_event(struct input_event);
	void jog(unsigned int value);
	void shuttle(int value);
	void key(unsigned int value, unsigned int code);	
	void check_shuttle_center();
};

#endif
