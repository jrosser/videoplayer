/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: mainwindow.cpp,v 1.7 2007-04-10 15:24:02 jrosser Exp $
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

#include "mainwindow.h"

#include <QtGui>

MainWindow::MainWindow()
{
	setWindowTitle("VideoPlayer");
	
	shuttle = new QShuttlePro();
	
	//threaded video reader
	videoRead = new VideoRead();
	
	//central widget is the threaded openGL video widget
	//which pulls video from the reader
	glvideo_mt = new GLvideo_mt(*videoRead);
	
	if(qApp->arguments().count() > 1) {
		const QStringList &args = qApp->arguments();	
		videoRead->setFileName(args[1]);
	}
	else
		exit(1);
		
  	setCentralWidget(glvideo_mt);
  				
	//set up menus etc
 	createActions();
        
    //shuttlepro jog wheel
	connect(shuttle, SIGNAL(jogForward()), videoRead, SLOT(transportJogFwd()));
	connect(shuttle, SIGNAL(jogBackward()), videoRead, SLOT(transportJogRev()));
	
	//shuttlepro shuttle dial	    
	connect(shuttle, SIGNAL(shuttleRight7()), videoRead, SLOT(transportFwd100()));
	connect(shuttle, SIGNAL(shuttleRight6()), videoRead, SLOT(transportFwd50()));
	connect(shuttle, SIGNAL(shuttleRight5()), videoRead, SLOT(transportFwd20()));
	connect(shuttle, SIGNAL(shuttleRight4()), videoRead, SLOT(transportFwd10()));
	connect(shuttle, SIGNAL(shuttleRight3()), videoRead, SLOT(transportFwd5()));
	connect(shuttle, SIGNAL(shuttleRight2()), videoRead, SLOT(transportFwd2()));
	connect(shuttle, SIGNAL(shuttleRight1()), videoRead, SLOT(transportFwd1()));
	
	connect(shuttle, SIGNAL(shuttleLeft7()), videoRead, SLOT(transportRev100()));
	connect(shuttle, SIGNAL(shuttleLeft6()), videoRead, SLOT(transportRev50()));
	connect(shuttle, SIGNAL(shuttleLeft5()), videoRead, SLOT(transportRev20()));
	connect(shuttle, SIGNAL(shuttleLeft4()), videoRead, SLOT(transportRev10()));
	connect(shuttle, SIGNAL(shuttleLeft3()), videoRead, SLOT(transportRev5()));			
	connect(shuttle, SIGNAL(shuttleLeft2()), videoRead, SLOT(transportRev2()));
	connect(shuttle, SIGNAL(shuttleLeft1()), videoRead, SLOT(transportRev1()));	
	connect(shuttle, SIGNAL(shuttleCenter()), videoRead, SLOT(transportStop()));			
	
	//shuttlepro buttons
	connect(shuttle, SIGNAL(key267Pressed()), videoRead, SLOT(transportPlayPause()));
	connect(shuttle, SIGNAL(key265Pressed()), videoRead, SLOT(transportFwd1()));
	connect(shuttle, SIGNAL(key259Pressed()), this, SLOT(toggleFullScreen()));
		
	//key 269, press=previous mark, hold=start of file
	//key 270, press=next mark, hold=end of file
	//key 256, cycle OSD status
	//key 257, cycle displayed timecode type
	//key 258, press=lock controls, hold=unlock controls
	//key 259, press=toggle fullscreen, hold=quit
	
    //createMenus();
    //createStatusBar();
    //createToolBar();
  	//setStatusBar(myStatusBar);
  	//addToolBar(Qt::TopToolBarArea, myToolBar);
  	//initializeAssistant();
  			            
    //signal connections for main window

	//some defaults in the abscence of any settings	
	
	//load the settings for this application
}


//generate actions for menus and keypress handlers
void MainWindow::createActions()
{	
	viewFullScreenAct = new QAction("View full screen", this);
	viewFullScreenAct->setShortcut(tr("Ctrl+F"));		
	addAction(viewFullScreenAct);		
	connect(viewFullScreenAct, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));	
    
    escapeFullScreenAct = new QAction("Escape full screen", this);
	escapeFullScreenAct->setShortcut(tr("Escape"));    
    addAction(escapeFullScreenAct);
    connect(escapeFullScreenAct, SIGNAL(triggered()), this, SLOT(escapeFullScreen()));
    
	transportFwd100Act = new QAction("Forward 100x", this);
	transportFwd100Act->setShortcut(tr("7"));
	addAction(transportFwd100Act);
	connect(transportFwd100Act, SIGNAL(triggered()), videoRead, SLOT(transportFwd100()));
	
	transportFwd50Act = new QAction("Forward 50x", this);
	transportFwd50Act->setShortcut(tr("6"));
	addAction(transportFwd50Act);
	connect(transportFwd50Act, SIGNAL(triggered()), videoRead, SLOT(transportFwd50()));
	
	transportFwd20Act = new QAction("Forward 20x", this);
	transportFwd20Act->setShortcut(tr("5"));
	addAction(transportFwd20Act);
	connect(transportFwd20Act, SIGNAL(triggered()), videoRead, SLOT(transportFwd20()));
	
	transportFwd10Act = new QAction("Forward 10x", this);
	transportFwd10Act->setShortcut(tr("4"));
	addAction(transportFwd10Act);
	connect(transportFwd10Act, SIGNAL(triggered()), videoRead, SLOT(transportFwd10()));
	
	transportFwd5Act = new QAction("Forward 5x", this);
	transportFwd5Act->setShortcut(tr("3"));
	addAction(transportFwd5Act);
	connect(transportFwd5Act, SIGNAL(triggered()), videoRead, SLOT(transportFwd5()));
	
	transportFwd2Act = new QAction("Forward 2x", this);
	transportFwd2Act->setShortcut(tr("2"));
	addAction(transportFwd2Act);
	connect(transportFwd2Act, SIGNAL(triggered()), videoRead, SLOT(transportFwd2()));
	
	transportFwd1Act = new QAction("Forward 1x", this);
	transportFwd1Act->setShortcut(tr("f"));
	addAction(transportFwd1Act);
	connect(transportFwd1Act, SIGNAL(triggered()), videoRead, SLOT(transportFwd1()));
	
	transportStopAct = new QAction("Stop", this);
	transportStopAct->setShortcut(tr("s"));
	addAction(transportStopAct);
	connect(transportStopAct, SIGNAL(triggered()), videoRead, SLOT(transportStop()));
	
	transportRev1Act = new QAction("Reverse 1x", this);
	transportRev1Act->setShortcut(tr("r"));
	addAction(transportRev1Act);
	connect(transportRev1Act, SIGNAL(triggered()), videoRead, SLOT(transportRev1()));
	
	transportRev2Act = new QAction("Reverse 2x", this);
	transportRev2Act->setShortcut(tr(""));
	addAction(transportRev2Act);
	connect(transportRev2Act, SIGNAL(triggered()), videoRead, SLOT(transportRev2()));
	
	transportRev5Act = new QAction("Reverse 5x", this);
	transportRev5Act->setShortcut(tr(""));
	addAction(transportRev5Act);
	connect(transportRev5Act, SIGNAL(triggered()), videoRead, SLOT(transportRev5()));
	
	transportRev10Act = new QAction("Reverse 10x", this);
	transportRev10Act->setShortcut(tr(""));
	addAction(transportRev10Act);
	connect(transportRev10Act, SIGNAL(triggered()), videoRead, SLOT(transportRev10()));
					
	transportRev20Act = new QAction("Reverse 20x", this);
	transportRev20Act->setShortcut(tr(""));
	addAction(transportRev20Act);
	connect(transportRev20Act, SIGNAL(triggered()), videoRead, SLOT(transportRev20()));
	
	transportRev50Act = new QAction("Reverse 50x", this);
	transportRev50Act->setShortcut(tr(""));
	addAction(transportFwd100Act);
	connect(transportFwd100Act, SIGNAL(triggered()), videoRead, SLOT(transportFwd100()));
	
	transportRev100Act = new QAction("Reverse 100x", this);								
	transportRev100Act->setShortcut(tr(""));
	addAction(transportRev100Act);
	connect(transportRev100Act, SIGNAL(triggered()), videoRead, SLOT(transportRev100()));

	transportPlayPauseAct = new QAction("Play/Pause", this);
	transportPlayPauseAct->setShortcut(tr("Space"));
	addAction(transportPlayPauseAct);
	connect(transportPlayPauseAct, SIGNAL(triggered()), videoRead, SLOT(transportPlayPause()));
	
	transportJogFwdAct = new QAction("Jog Forward", this);
	transportJogFwdAct->setShortcut(tr("."));
	addAction(transportJogFwdAct);
	connect(transportJogFwdAct, SIGNAL(triggered()), videoRead, SLOT(transportJogFwd()));
	
	transportJogRevAct = new QAction("Jog Reverse", this);
	transportJogRevAct->setShortcut(tr(","));
	addAction(transportJogRevAct);
	connect(transportJogRevAct, SIGNAL(triggered()), videoRead, SLOT(transportJogRev()));    
}

//slot to receive full screen toggle command
void MainWindow::toggleFullScreen()
{
	setFullScreen(!isFullScreen());	
}

void MainWindow::escapeFullScreen()
{
	if(isFullScreen())
		setFullScreen(false);	
}

//full screen toggle worker
void MainWindow::setFullScreen(bool fullscreen)
{
	if(fullscreen) {
		showFullScreen();				
	}
	else {
		showNormal();
	}	
}
