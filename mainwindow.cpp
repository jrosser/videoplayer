/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: mainwindow.cpp,v 1.3 2007-03-28 13:58:25 jrosser Exp $
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
	//central widget is the 'view'
	glvideo_mt = new GLvideo_mt();
	
	if(qApp->arguments().count() > 1) {
		const QStringList &args = qApp->arguments();	
		glvideo_mt->setFileName(args[1]);
	}
	else
		exit(1);
		
  	setCentralWidget(glvideo_mt);
  		
    //glvideo = new GLvideo();            
  	//setCentralWidget(glvideo);
	
	setWindowTitle("VideoPlayer");
	
	QSizePolicy newPolicy(sizePolicy());
	newPolicy.setHeightForWidth(true); 
	setSizePolicy(newPolicy);	
	
	//set up menus etc
 	//createActions();
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

int MainWindow::heightForWidth ( int w ) const
{
	return( (w * 9) / 16);	
}
