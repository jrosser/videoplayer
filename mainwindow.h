/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: mainwindow.h,v 1.12 2007-05-18 14:29:05 jrosser Exp $
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtAssistant/QAssistantClient>
#include <QtGui>

#include "GLvideo_mt.h"
#include "videoRead.h"

#ifdef Q_OS_LINUX
#include "QShuttlePro.h"
#endif

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
	bool allParsed;		//flag to say that all commandline parameters were parsed OK.
	    
protected:

private slots:
	void toggleFullScreen();
	void escapeFullScreen(); 
	void quit();
	        
private:
	void createActions(void);
	void parseCommandLine();
	void usage();

#ifdef Q_OS_LINUX	
	QShuttlePro *shuttle;
#endif

	QAction *quitAct;

	//display actions
	QAction *toggleOSDAct;
	QAction *toggleAspectLockAct;		
	QAction *viewFullScreenAct;
	QAction *escapeFullScreenAct;
	QAction *toggleLuminanceAct;
	QAction *toggleChrominanceAct;
	
	//transport control actions
	QAction *transportFwd100Act;
	QAction *transportFwd50Act;
	QAction *transportFwd20Act;
	QAction *transportFwd10Act;
	QAction *transportFwd5Act;
	QAction *transportFwd2Act;
	QAction *transportFwd1Act;
	QAction *transportStopAct;
	QAction *transportRev1Act;
	QAction *transportRev2Act;
	QAction *transportRev5Act;
	QAction *transportRev10Act;				
	QAction *transportRev20Act;
	QAction *transportRev50Act;
	QAction *transportRev100Act;								

	QAction *transportPlayPauseAct;
	QAction *transportJogFwdAct;
	QAction *transportJogRevAct;
	
	void setFullScreen(bool);
	VideoRead *videoRead;
	GLvideo_mt *glvideo_mt;

	//data from the command line parameters
	int videoWidth;
	int videoHeight;
	int frameRepeats;
	int framePolarity;
	QString fileName;
	QString fontFile;
	
	bool forceFileType;
	QString fileType;
};

#endif
