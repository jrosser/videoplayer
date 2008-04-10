/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: videoData.h,v 1.16 2008-03-28 17:04:36 jrosser Exp $
*
* The MIT License
*
* Copyright (c) 2008 BBC Research
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
* ***** END LICENSE BLOCK ***** */

#ifndef VIDEODATA_H_
#define VIDEODATA_H_

#include <GL/glew.h>

class VideoData
{
public:

    enum DataFmt { V8P0, V8P2, V8P4, YV12, V16P0, V16P2, V16P4, UYVY, V216, V210, Unknown};    //how is the data packed?

    VideoData();
    void resize(int w, int h, DataFmt f);
    ~VideoData();

    void convertV210();
    void convertPlanar16();

    unsigned long frameNum;
    bool isLastFrame;		 //mark the first and last frames in a sequence
    bool isFirstFrame;
    
    DataFmt diskFormat;      //enumerated format read from the disk
    DataFmt renderFormat;    //pixel packing used for openGL rendering

    bool isPlanar;           //flag to indicate that we have a seperate texture for each component to copy to the GPU

    GLenum glInternalFormat; //format used to describe which components of the texture are applied to the texels
    GLenum glFormat;         //format of the texture data, GL_LUMINANCE or GL_RGBA in these applications
    GLenum glType;           //the data type for the texture, GL_UNSIGNED_BYTE, GL_INT etc...
    int glMinMaxFilter;      //filter type for scaling the pixel data GL_LINEAR for planar, or GL_NEAREST for packed
    int glYTextureWidth;     //the width of the Y texture, which may actually be UYVY data packed as RGBA. There
                             //will be half the number of RGBA quads as Y pixels across the image as each quad
                             //stores two luminance samples

    int Ywidth;              //luminance dimensions in pixels
    int Yheight;

    int Cwidth;              //chrominance dimensions in pixels
    int Cheight;

    int dataSize;            //total allocation at *data
    int YdataSize;           //number of bytes for each component
    int UdataSize;
    int VdataSize;

    unsigned char *data;              //pointer to allocated block
    unsigned char *Ydata;             //pointer to luminance, or multiplexed YCbCr
    unsigned char *Udata;             //pointers to planar chrominance components, if format is planar
    unsigned char *Vdata;
    
private:
	void allocate(int w, int h, DataFmt f);
};

#endif
