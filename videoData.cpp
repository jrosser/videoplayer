/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: videoData.cpp,v 1.17 2007-12-13 12:12:59 jrosser Exp $
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

#include "videoData.h"

#include <stdlib.h>

#ifdef Q_OS_WIN32
#define valloc malloc
#endif

VideoData::VideoData(int w, int h, DataFmt f)
	: diskFormat(f)
	, Ywidth(w)
	, Yheight(h)
{
	switch(diskFormat) {
	
		case V8P4:				//8 bit 444 planar
			Cwidth  = Ywidth;
			Cheight = Yheight;
			YdataSize = Ywidth * Yheight;
			UdataSize = YdataSize;
			VdataSize = YdataSize;						
			break;

		case V16P4:				//16 bit 444 planar
			Cwidth  = Ywidth;
			Cheight = Yheight;
			YdataSize = Ywidth * Yheight * 2;
			UdataSize = YdataSize;
			VdataSize = YdataSize;						
			break;

		case V8P2:				//8 bit 422 planar
			Cwidth  = Ywidth / 2;
			Cheight = Yheight;		
			YdataSize = Ywidth * Yheight;
			UdataSize = YdataSize / 2;
			VdataSize = YdataSize / 2;								
			break;

		case V16P2:				//16 bit 422 planar
			Cwidth  = Ywidth / 2;
			Cheight = Yheight;		
			YdataSize = Ywidth * Yheight * 2;
			UdataSize = YdataSize / 2;
			VdataSize = YdataSize / 2;								
			break;
			
		case V16P0:			//16 bit 420
			Cwidth  = Ywidth / 2;
			Cheight = Yheight / 2;		
			YdataSize = Ywidth * Yheight * 2;
			UdataSize = YdataSize / 4;
			VdataSize = YdataSize / 4;								
			break;
		
		case V8P0:			//8 bit 420, YUV		
		case YV12:			//8 bit 420, YVU
			Cwidth  = Ywidth / 2;
			Cheight = Yheight / 2;		
			YdataSize = Ywidth * Yheight;
			UdataSize = YdataSize / 4;
			VdataSize = YdataSize / 4;								
			break;
			
		case UYVY:
			Cwidth  = Ywidth / 2;
			Cheight = Yheight;		
			YdataSize = Ywidth * Yheight * 2;
			UdataSize = 0;
			VdataSize = 0;								
			break;
			
		case V216:
			Cwidth  = Ywidth / 2;
			Cheight = Yheight;	
			YdataSize = Ywidth * Yheight * 4;
			UdataSize = 0;
			VdataSize = 0;
			break;
			
		case V210:
			//3 10 bit components packed into 4 bytes
			Cwidth  = Ywidth / 2;
			Cheight = Yheight;			
			YdataSize = (Ywidth * Yheight * 2 * 4) / 3;
			UdataSize = 0;
			VdataSize = 0;			
			break;
			
		default:
			//oh-no!
			break;
	}
	
	//total size of the frame
	dataSize = YdataSize + UdataSize + VdataSize;
	
	//get some nicely page aligned memory 
	data = (char *)valloc(dataSize);
	Ydata = data;
	isPlanar = false;
	glMinMaxFilter = GL_LINEAR;	//linear interpolation
	Udata = Ydata;
	Vdata = Ydata;
	renderFormat = diskFormat;
	
	switch(diskFormat) {
		//planar formats YUV
		case V16P4:
		case V16P2:
		case V16P0:
		case V8P4:
		case V8P2:
		case V8P0:
			glYTextureWidth = Ywidth;		
			glInternalFormat = GL_LUMINANCE;
			glFormat = GL_LUMINANCE;
			glType = GL_UNSIGNED_BYTE;
			isPlanar = true;
			Udata = Ydata + YdataSize;
			Vdata = Udata + UdataSize;
			break;
		
		//planar format, YVU
		case YV12:
			isPlanar = true;
			glYTextureWidth = Ywidth;			
			glInternalFormat = GL_LUMINANCE;
			glFormat = GL_LUMINANCE;
			glType = GL_UNSIGNED_BYTE;			
			//U&V components are exchanged
			Vdata = Ydata + (Ywidth * Yheight);
			Udata = Vdata + ((Ywidth * Yheight) / 4);
			break;
			
		case UYVY:
			//muxed 8 bit format
			glYTextureWidth = Ywidth / 2;	//2 Y samples per RGBA quad			
			glInternalFormat = GL_RGBA;
			glFormat = GL_RGBA;
			glType = GL_UNSIGNED_BYTE;
			glMinMaxFilter = GL_NEAREST;					
			break;
					
		case V210:							//gets converted to UYVY
			glYTextureWidth = Ywidth / 2;	//2 Y samples per RGBA quad			
			glInternalFormat = GL_RGBA;
			glFormat = GL_RGBA;
			glType = GL_UNSIGNED_BYTE;
			glMinMaxFilter = GL_NEAREST;
			break;
			
		case V216:
			glYTextureWidth = Ywidth / 2;	//2 Y samples per RGBA quad
			glInternalFormat = GL_RGBA;
			glFormat = GL_RGBA;
			glType = GL_UNSIGNED_SHORT;			
			break;
			
		default:
			//oh-no!
			break;
	}			
}
	
VideoData::~VideoData()
{
	if(data)
		free(data);	
};

typedef unsigned int uint_t;
typedef unsigned char uint8_t;

//read the first 10 bit sample from the least significant bits of the 4 bytes pointed to by 'data'
inline uint_t
readv210sample_pos2of3(uint8_t* data)
{
	const uint_t lsb = (data[2] & 0xf0) >> 4;
	const uint_t msb = (data[3] & 0x3f) << 4;
	return msb | lsb;
}

//read the second 10 bit sample from the middle bits of the 4 bytes pointed to by 'data'
inline uint_t
readv210sample_pos1of3(uint8_t* data)
{
	const uint_t lsb = (data[1] & 0xfc) >> 2;
	const uint_t msb = (data[2] & 0x0f) << 6;
	return msb | lsb;
}

//read the third 10 bit sample from the more significant bits of the 4 bytes pointed to by 'data'
inline uint_t
readv210sample_pos0of3(uint8_t* data)
{
	const uint_t lsb = (data[0] & 0xff);
	const uint_t msb = (data[1] & 0x03) << 8;
	return msb | lsb;
}

void unpackv210line(uint8_t* dst, uint8_t* src, uint_t luma_width)
{
	/* number of blocks completely filled with active samples (6 per block) */
	const int num_firstpass_samples = 2*(luma_width/6)*6;
	int x;
	for (x=0; x < num_firstpass_samples; src += 16) {
		dst[x++] = readv210sample_pos0of3(src + 0) >> 2; /* Cb */
		dst[x++] = readv210sample_pos1of3(src + 0) >> 2; /* Y' */
		dst[x++] = readv210sample_pos2of3(src + 0) >> 2; /* Cr */
		dst[x++] = readv210sample_pos0of3(src + 4) >> 2; /* Y' */

		dst[x++] = readv210sample_pos1of3(src + 4) >> 2;
		dst[x++] = readv210sample_pos2of3(src + 4) >> 2;
		dst[x++] = readv210sample_pos0of3(src + 8) >> 2;
		dst[x++] = readv210sample_pos1of3(src + 8) >> 2;

		dst[x++] = readv210sample_pos2of3(src + 8) >> 2;
		dst[x++] = readv210sample_pos0of3(src + 12) >> 2;
		dst[x++] = readv210sample_pos1of3(src + 12) >> 2;
		dst[x++] = readv210sample_pos2of3(src + 12) >> 2;
	}
	/* mop up last subblock (4 or 2 active samples) */

	if (x < 2*luma_width) {
		dst[x++] = readv210sample_pos0of3(src + 0) >> 2; /* Cb */
		dst[x++] = readv210sample_pos1of3(src + 0) >> 2; /* Y' */
		dst[x++] = readv210sample_pos2of3(src + 0) >> 2; /* Cr */
		dst[x++] = readv210sample_pos0of3(src + 4) >> 2; /* Y' */
	}

	if (x < 2*luma_width) {
		dst[x++] = readv210sample_pos1of3(src + 4) >> 2;
		dst[x++] = readv210sample_pos2of3(src + 4) >> 2;
		dst[x++] = readv210sample_pos0of3(src + 8) >> 2;
		dst[x++] = readv210sample_pos1of3(src + 8) >> 2;
	}
}


//utility function to convert V210 data into something easier to display
//it is too difficult to do this in the openGL shader
void VideoData::convertV210()
{
	/* pad to a multiple of 48 luma samples */
	/* nb, 48 luma samples becomes 128 bytes */
	const uint_t padded_w = ((Ywidth + 47)/48) * 48; //number of luma samples, padded
	const uint_t padded_line_length =  (2*padded_w*4) / 3; //number of bytes on each line in the file, includin padding data

	uint8_t *dst = (uint8_t*) data; /* destination for 8bit uyvy, may = src */
	uint8_t *src = (uint8_t*) data; /* source of v210 data */

    for (int y=0; y<Yheight; y++) {
		unpackv210line(dst, src, Ywidth);
		src += padded_line_length;
		dst += Ywidth*2;
	}

	//change the description of the data to match UYVY
	//note that the dataSize stays the same
	renderFormat = UYVY;
	glYTextureWidth = Ywidth / 2;	//2 Y samples per RGBA quad
	YdataSize = Ywidth * Yheight * 2;
	UdataSize = 0;
	VdataSize = 0;
}

void VideoData::convertPlanar16()
{
	uint8_t *src=(uint8_t *)Ydata;
	uint8_t *dst=(uint8_t *)Ydata;
	const uint8_t *end=(uint8_t *)(Ydata+dataSize);
		
	//convert the 16 bit source to 8 bit destination, in place
	//just throw away the LSBs
	while(src < end) {
		*dst++ = *src;
		src+=2;	
	}

	//new data size
	YdataSize = Ywidth * Yheight;
		
	//tell the renderer that this is now 8 bit data
	switch(renderFormat) {
		case V16P0:
			renderFormat = V8P0;
			UdataSize = YdataSize/4;
			VdataSize = YdataSize/4;
			break;
			
		case V16P2:
			renderFormat = V8P2;
			UdataSize = YdataSize/2;
			VdataSize = YdataSize/2;			
			break;
			
		case V16P4:
			renderFormat = V8P4;
			UdataSize = YdataSize;
			VdataSize = YdataSize;			
			break;
			
		default:
			//doh!
			break;
	}
		
	//update pointers to the start of each plane
	Udata = Ydata + YdataSize;
	Vdata = Udata + UdataSize;
}
