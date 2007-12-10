/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: videoData.h,v 1.9 2007-12-10 10:51:13 jrosser Exp $
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

#ifndef VIDEODATA_H_
#define VIDEODATA_H_

#include "GLvideo_rt.h"

class VideoData
{
public:

	enum DataFmt { Planar444, Planar422, I420, YV12, Planar411, UYVY, V216, V210 };	//how is the data packed?
		
	VideoData(int width, int height, DataFmt f);
	~VideoData();
	
	void convertV210();
		
	unsigned long frameNum;
	
	DataFmt    diskFormat;		//enumerated format read from the disk
	DataFmt    renderFormat;	//pixel packing used for openGL rendering
		
	bool 	   isPlanar;	//flag to indicate that we have a seperate texture for each component to copy to the GPU
	
	GLenum		glInternalFormat;	//format used to describe which components of the texture are applied to the texels
	GLenum		glFormat;			//format of the texture data, GL_LUMINANCE or GL_RGBA in these applications
	GLenum		glType;				//the data type for the texture, GL_UNSIGNED_BYTE, GL_INT etc...
	int			glMinMaxFilter;		//filter type for scaling the pixel data GL_LINEAR for filtering, GL_NEAREST for V210 specifically
	int 		glYTextureWidth;	//the width of the Y texture, which may actually be UYVY data packed as RGBA. There
									//will be half the number of RGBA quads as Y pixels across the image as each quad
									//stores two luminance samples	
		
	int Ywidth;			//luminance dimensions in pixels
	int Yheight;
	
	int Cwidth;			//chrominance dimensions in pixels
	int Cheight;
	
	int dataSize;		//total allocation at *data
	int YdataSize;		//number of bytes for each component 
	int UdataSize;
	int VdataSize;
		
	char *data;			//pointer to allocated block
	char *Ydata;		//pointer to luminance, or multiplexed YCbCr
	char *Udata;		//pointers to planar chrominance components, if format is planar
	char *Vdata;        
};

#endif
