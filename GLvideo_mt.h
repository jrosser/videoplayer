/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: GLvideo_mt.h,v 1.2 2007-03-28 13:58:25 jrosser Exp $
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

#ifndef GLVIDEO_MT_H
#define GLVIDEO_MT_H

#define GL_GLEXT_PROTOTYPES 1

#include <QtGui>
#include <QGLWidget>
#include "GLvideo_rt.h"

#include <sys/time.h>


class GLvideo_mt : public QGLWidget
{
    Q_OBJECT

public:
	GLvideo_mt(QWidget *parent = 0);
	void setFileName(const QString &fileName);
	void lockMutex();
	void unlockMutex();
		
protected:

private slots:
        
private:
	void initializeGL();
	void resizeGL(int w, int h);
	void paintGL();
	void paintEvent(QPaintEvent * event);
	GLvideo_rt renderThread;
	QMutex mutex;	
	
};

#endif
