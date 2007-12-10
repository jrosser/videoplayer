/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: GLvideo.h,v 1.3 2007-12-10 10:24:47 jrosser Exp $
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

#ifndef GLVIDEO_H
#define GLVIDEO_H

#define GL_GLEXT_PROTOTYPES 1

#include <QtGui>
#include <QGLWidget>

#ifdef Q_OS_UNIX
#include <sys/time.h>
#endif

#ifdef Q_OS_MACX
#include <sys/time.h>
#endif

class GLvideo : public QGLWidget
{
    Q_OBJECT

public:
	GLvideo(QWidget *parent = 0);

protected:
	void initialiseGL();
	void resizeGL(int width, int height);
	void paintGL();
	void compileFragmentShader();
	
private slots:
        
private:
	
	timeval last;
	int framenum;
	int srcwidth, srcheight;	//dimensions of input pictures

	bool shaderCompiled;
	GLuint shader, program;	//handles for the shader and program
   	GLint compiled, linked;	//flags for success
   	
   	GLubyte *Ytex, *Utex, *Vtex;	//Y,U,V textures
   	
   	FILE *fp;

};

#endif
