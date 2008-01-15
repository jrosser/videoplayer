/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: GLvideo_rt.cpp,v 1.41 2008-01-15 14:25:22 jrosser Exp $
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

#include "GLvideo_rt.h"
#include "GLvideo_mt.h"

#ifdef HAVE_FTGL
#include "FTGL/FTGLPolygonFont.h"
#endif

#ifdef Q_OS_MACX
#include "agl_getproc.h"
#endif

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/time.h>
#else
#include <time.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

//FIXME - nasty global variables
//rendering performance monitoring
int perf_getParams;
int perf_readData;
int perf_convertFormat;
int perf_updateVars;
int perf_repeatWait;
int perf_upload;
int perf_renderVideo;
int perf_renderOSD;
int perf_swapBuffers;
int perf_interval;

//I/O performance
int perf_futureQueue;
int perf_pastQueue;
int perf_IOLoad;
int perf_IOBandwidth;

#define DEBUG 0

//------------------------------------------------------------------------------------------
//shader program for planar video formats
//should work with all planar chroma subsamplings
//glInternalFormat=GL_LUMINANCE glFormat=GL_LUMINANCE glType=GL_UNSIGNED_BYTE
static char *shaderPlanarSrc=
  "#extension GL_ARB_texture_rectangle : require\n"
  "uniform sampler2DRect Ytex;\n"
  "uniform sampler2DRect Utex,Vtex;\n"
  "uniform float Yheight, Ywidth;\n"   
  "uniform float CHsubsample, CVsubsample;\n"
  
  "uniform bool interlacedSource;\n"
  "uniform bool deinterlace;\n"  
  "uniform int  field;\n"
  "uniform int  direction;\n"  
  "uniform vec3 yuvOffset1;\n"
  "uniform vec3 yuvMul;\n"
  "uniform vec3 yuvOffset2;\n"
  "uniform mat3 colorMatrix;\n"
          
  "void main(void) {\n"
  " float nx,ny,a;\n"
  " int thisField;\n"
  " vec3 yuv;\n"
  " vec3 rgb;\n"
  
  " nx=gl_TexCoord[0].x;\n"
  " ny=Yheight-gl_TexCoord[0].y;\n"

  " thisField = field;\n"		//swap field order when playing interlaced pictures backwards
  " if(direction < 0) thisField = 1 - field;\n"		//swap field order when playing interlaced pictures backwards

  " if((interlacedSource == true) && (deinterlace == true) && (mod(floor(ny) + float(thisField), 2.0) > 0.5)) {\n"   
  "     //interpolated line in a field\n"
  "     yuv[0] = texture2DRect(Ytex,vec2(nx, (ny+1.0))).r + texture2DRect(Ytex,vec2(nx, (ny-1.0))).r;\n" 
  "     yuv[1] = texture2DRect(Utex,vec2(nx/CHsubsample, (ny+1.0)/CVsubsample)).r + texture2DRect(Utex,vec2(nx/CHsubsample, (ny-1.0)/CVsubsample)).r;\n"
  "     yuv[2] = texture2DRect(Vtex,vec2(nx/CHsubsample, (ny+1.0)/CVsubsample)).r + texture2DRect(Vtex,vec2(nx/CHsubsample, (ny-1.0)/CVsubsample)).r;\n"  
  "     yuv = yuv / 2.0;"
  " }\n"
  " else {\n"
  "     //non interpolated line in a field, or non interlaced\n"
  "     yuv[0]=texture2DRect(Ytex,vec2(nx,ny)).r;\n"
  "     yuv[1]=texture2DRect(Utex,vec2(nx/CHsubsample, ny/CVsubsample)).r;\n"
  "     yuv[2]=texture2DRect(Vtex,vec2(nx/CHsubsample, ny/CVsubsample)).r;\n"    
  " }\n"

  " yuv = yuv + yuvOffset1;\n"
  " yuv = yuv * yuvMul;\n"
  " yuv = yuv + yuvOffset2;\n"  
  " rgb = yuv * colorMatrix;\n"

  "	a=1.0;\n"
      
  " gl_FragColor=vec4(rgb ,a);\n"
  "}\n";

//------------------------------------------------------------------------------------------
//shader program for UYVY muxed 8 bit data with glInternalFormat=GL_RGBA glFormat=GL_RGBA glType=GL_UNSIGNED_BYTE
//shader program for v216 muxed 16 bit data with glInternalFormat=GL_RGBA glFormat=GL_RGBA glType=GL_UNSIGNED_SHORT
//there are 2 luminance samples per RGBA quad, so divide the horizontal location by two to use each RGBA value twice
static char *shaderUYVYSrc=
  "#extension GL_ARB_texture_rectangle : require\n"
  "uniform sampler2DRect Ytex;\n"
  "uniform sampler2DRect Utex,Vtex;\n"
  "uniform float Yheight, Ywidth;\n"  
  "uniform float CHsubsample, CVsubsample;\n"

  "uniform bool interlacedSource;\n"
  "uniform bool deinterlace;\n"  
  "uniform int  field;\n"
  "uniform int  direction;\n"  
  "uniform vec3 yuvOffset1;\n"
  "uniform vec3 yuvMul;\n"
  "uniform vec3 yuvOffset2;\n"
  "uniform mat3 colorMatrix;\n"
        
  "void main(void) {\n"
  " float nx,ny,a;\n"
  " vec3 yuv;\n"
  " vec4 rgba;\n"
  " vec4 above;\n"
  " vec4 below;\n"
  " vec3 rgb;\n"
  " int  thisField;\n"
    
  " nx=gl_TexCoord[0].x;\n"
  " ny=Yheight-gl_TexCoord[0].y;\n"
 
  " thisField = field;\n"	
  " if(direction < 0) thisField = 1 - field;\n"		//swap field order when playing interlaced pictures backwards	
 
  " if((interlacedSource == true) && (deinterlace == true) && (mod(floor(ny) + float(field), 2.0) > 0.5)) {\n"   
  "     //interpolated line in a field\n"
  "     above = texture2DRect(Ytex, vec2(floor(nx/2.0), ny+1.0));\n"
  "     below = texture2DRect(Ytex, vec2(floor(nx/2.0), ny-1.0));\n"

  "     yuv[0]  = (fract(nx/2.0) < 0.5) ? above.g : above.a;\n"
  "     yuv[0] += (fract(nx/2.0) < 0.5) ? below.g : below.a;\n"
  "     yuv[1] = above.r + below.r;\n"
  "     yuv[2] = above.b + below.b;\n"
  "     yuv = yuv / 2.0;"
  " }\n"
  " else {\n"
  "     //non interpolated line in a field, or non interlaced\n"
  "     rgba = texture2DRect(Ytex, vec2(floor(nx/2.0), ny));\n"	 //sample every other RGBA quad to get luminance
  "     yuv[0] = (fract(nx/2.0) < 0.5) ? rgba.g : rgba.a;\n"   //pick the correct luminance from G or A for this pixel
  "     yuv[1] = rgba.r;\n"
  "     yuv[2] = rgba.b;\n"  
  " }\n"

  " yuv = yuv + yuvOffset1;\n"
  " yuv = yuv * yuvMul;\n"
  " yuv = yuv + yuvOffset2;\n"  
  " rgb = yuv * colorMatrix;\n"

  " a=1.0;\n"
      
  " gl_FragColor=vec4(rgb, a);\n"
  "}\n";

GLvideo_rt::GLvideo_rt(GLvideo_mt &gl) 
      : QThread(), glw(gl)
{	
	m_frameRepeats = 0;	
	m_doRendering = true;
	m_aspectLock = true;
	m_osd = true;
	m_perf = false;
	m_changeFont = false;
	m_showLuminance = true;
	m_showChrominance = true;
	m_interlacedSource = false;
	m_deinterlace = false;
	m_matrixScaling = false;
	m_changeMatrix = true;			//always compute colour matrix at least once
	
	m_luminanceOffset1 = -0.0625;	//video luminance has black at 16/255
	m_chrominanceOffset1 = -0.5;    //video chrominace is centered at 128/255
	
	m_luminanceMultiplier = 1.0;	//unscaled
	m_chrominanceMultiplier = 1.0;
	
	m_luminanceOffset2 = 0.0;		//no offset
	m_chrominanceOffset2 = 0.0;
	
	m_matrixKr = 0.2126;	//colour matrix for HDTV
	m_matrixKg = 0.7152;
	m_matrixKb = 0.0722;
	
	m_osdScale = 1.0;
	
	memset(caption, 0, sizeof(caption));
	strcpy(caption, "Hello World");
}

void GLvideo_rt::buildColourMatrix(float *matrix, const float Kr, const float Kg, const float Kb, bool Yscale, bool Cscale)
{
	//R row
	matrix[0] = 1.0;				//Y	column
	matrix[1] = 0.0;				//Cb
	matrix[2] = 2.0 * (1-Kr);	//Cr	
	
	//G row
	matrix[3] = 1.0;
	matrix[4] = -1.0 * ( (2.0*(1.0-Kb)*Kb)/Kg );
	matrix[5] = -1.0 * ( (2.0*(1.0-Kr)*Kr)/Kg );
	
	//B row
	matrix[6] = 1.0;
	matrix[7] = 2.0 * (1.0-Kb);
	matrix[8] = 0.0;

	float Ys = Yscale ? (255.0/219.0) : 1.0;
	float Cs = Cscale ? (255.0/224.0) : 1.0;

	//apply any necessary scaling
	matrix[0] *= Ys;
	matrix[1] *= Cs;
	matrix[2] *= Cs;
	matrix[3] *= Ys;
	matrix[4] *= Cs;
	matrix[5] *= Cs;
	matrix[6] *= Ys;
	matrix[7] *= Cs;
	matrix[8] *= Cs;

	if(DEBUG) {
		printf("Building YUV->RGB matrix with Kr=%f Kg=%f, Kb=%f\n", Kr, Kg, Kb);
		printf("%f %f %f\n", matrix[0], matrix[1], matrix[2]);
		printf("%f %f %f\n", matrix[3], matrix[4], matrix[5]);
		printf("%f %f %f\n", matrix[6], matrix[7], matrix[8]);
		printf("\n");
	}			
}


void GLvideo_rt::compileFragmentShaders()
{
	compileFragmentShader((int)shaderPlanar, shaderPlanarSrc);
	compileFragmentShader((int)shaderUYVY,   shaderUYVYSrc);						
}

void GLvideo_rt::compileFragmentShader(int n, const char *src)
{
	GLint length = 0;			//log length
	
    shaders[n]=glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

	if(DEBUG) printf("Shader program is %s\n", src);

    glShaderSourceARB(shaders[n], 1, (const GLcharARB**)&src, NULL);    
	glCompileShaderARB(shaders[n]);
    
    glGetObjectParameterivARB(shaders[n], GL_OBJECT_COMPILE_STATUS_ARB, &compiled[n]);
    glGetObjectParameterivARB(shaders[n], GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);    
    char *s=(char *)malloc(length);
    glGetInfoLogARB(shaders[n], length, &length ,s);
    if(DEBUG)printf("Compile Status %d, Log: (%d) %s\n", compiled[n], length, length ? s : NULL);
    if(compiled[n] <= 0) {
		printf("Compile Failed: %s\n", gluErrorString(glGetError()));
    	throw /* */;
    }
    free(s);

   	/* Set up program objects. */
    programs[n]=glCreateProgramObjectARB();
    glAttachObjectARB(programs[n], shaders[n]);
    glLinkProgramARB(programs[n]);

    /* And print the link log. */
    if(compiled[n]) {
    	glGetObjectParameterivARB(programs[n], GL_OBJECT_LINK_STATUS_ARB, &linked[n]);
    	glGetObjectParameterivARB(shaders[n], GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);            
    	s=(char *)malloc(length);
    	glGetInfoLogARB(shaders[n], length, &length, s);
    	if(DEBUG) printf("Link Status %d, Log: (%d) %s\n", linked[n], length, s);
    	free(s);
    }
}

void GLvideo_rt::createTextures(VideoData *videoData, int currentShader)
{
	int i=0;
	
	if(videoData->isPlanar) {
    
    	/* Select texture unit 1 as the active unit and bind the U texture. */
    	glActiveTexture(GL_TEXTURE1);
    	i=glGetUniformLocationARB(programs[currentShader], "Utex");
    	glUniform1iARB(i,1);  /* Bind Utex to texture unit 1 */
    	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,1);

    	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, videoData->glMinMaxFilter);
    	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, videoData->glMinMaxFilter);
		glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, videoData->glInternalFormat, videoData->Cwidth , videoData->Cheight, 0, videoData->glFormat, videoData->glType, NULL);
    
    	/* Select texture unit 2 as the active unit and bind the V texture. */
    	glActiveTexture(GL_TEXTURE2);
    	i=glGetUniformLocationARB(programs[currentShader], "Vtex");
    	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,2);
    	glUniform1iARB(i,2);  /* Bind Vtext to texture unit 2 */

    	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, videoData->glMinMaxFilter);
    	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, videoData->glMinMaxFilter);
		glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, videoData->glInternalFormat, videoData->Cwidth , videoData->Cheight, 0, videoData->glFormat, videoData->glType, NULL);
	}
    
    /* Select texture unit 0 as the active unit and bind the Y texture. */
    glActiveTexture(GL_TEXTURE3);
    i=glGetUniformLocationARB(programs[currentShader], "Ytex");
    glUniform1iARB(i,3);  /* Bind Ytex to texture unit 3 */
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,3);

    glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, videoData->glMinMaxFilter);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, videoData->glMinMaxFilter);
	glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, videoData->glInternalFormat, videoData->glYTextureWidth, videoData->Yheight, 0, videoData->glFormat, videoData->glType, NULL);

	//create buffer objects that are used to transfer the texture data to the card
	//this is much faster than using glTexSubImage2D() later on to replace the texture data
    if(videoData->isPlanar) {
    	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, io_buf[0]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, videoData->UdataSize, NULL, GL_STREAM_DRAW);    

    	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, io_buf[1]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, videoData->VdataSize, NULL, GL_STREAM_DRAW);    
    }
    
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, io_buf[2]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, videoData->YdataSize, NULL, GL_STREAM_DRAW);    
}
    
void GLvideo_rt::stop()
{
	mutex.lock();
	m_doRendering = false;
	mutex.unlock();
}

void GLvideo_rt::setAspectLock(bool l)
{
	mutex.lock();

	if(m_aspectLock != l) {
		m_aspectLock = l;
		m_doResize = true;
	}
	
	mutex.unlock();	
}

void GLvideo_rt::setFrameRepeats(int repeats)
{
	mutex.lock();
	m_frameRepeats = repeats;
	mutex.unlock();		
}

void GLvideo_rt::setFontFile(const char *f)
{
	mutex.lock();
	strncpy(fontFile, f, 255);
	m_changeFont = true;
	mutex.unlock();	
}

void GLvideo_rt::setOsdScale(float s)
{
	mutex.lock();
	m_osdScale = s;
	mutex.unlock();	
}


void GLvideo_rt::setCaption(const char *c)
{
	mutex.lock();
	strncpy(caption, c, 255);
	mutex.unlock();	
}

void GLvideo_rt::setFramePolarity(int p)
{
	mutex.lock();
	m_framePolarity = p;
	mutex.unlock();		
}

void GLvideo_rt::toggleAspectLock(void)
{
	mutex.lock();
	m_aspectLock = !m_aspectLock;
	m_doResize = true;
	mutex.unlock();		
}

void GLvideo_rt::toggleOSD(void)
{
	mutex.lock();
	m_osd = (m_osd + 1) % MAX_OSD;	
	mutex.unlock();		
}

void GLvideo_rt::togglePerf(void)
{
	mutex.lock();
	m_perf = !m_perf;	
	mutex.unlock();		
}


void GLvideo_rt::setInterlacedSource(bool i)
{
	mutex.lock();
	
	m_interlacedSource = i;
	if(m_interlacedSource == false) m_deinterlace=false;
		
	mutex.unlock();		
}

void GLvideo_rt::setDeinterlace(bool d)
{
	mutex.lock();
	
	if(m_interlacedSource)
		m_deinterlace = d;
	else
		m_deinterlace = false;
			
	mutex.unlock();		
}

void GLvideo_rt::toggleDeinterlace(void)
{
	mutex.lock();
	
	if(m_interlacedSource)
		m_deinterlace = !m_deinterlace;
	else
		m_deinterlace = false;
			
	mutex.unlock();		
}

void GLvideo_rt::toggleLuminance(void)
{
	mutex.lock();
	m_showLuminance = !m_showLuminance;	
	mutex.unlock();		
}

void GLvideo_rt::toggleChrominance(void)
{
	mutex.lock();
	m_showChrominance = !m_showChrominance;	
	mutex.unlock();		
}

void GLvideo_rt::setMatrixScaling(bool s)
{
	mutex.lock();
	m_matrixScaling = s;
	m_changeMatrix = true;
	mutex.unlock();	
}

void GLvideo_rt::toggleMatrixScaling()
{
	mutex.lock();
	m_matrixScaling = !m_matrixScaling;
	m_changeMatrix = true;
	mutex.unlock();	
}

void GLvideo_rt::setMatrix(float Kr, float Kg, float Kb)
{
	mutex.lock();
	m_matrixKr = Kr;
	m_matrixKg = Kg;
	m_matrixKb = Kb;
	m_changeMatrix = true;
	mutex.unlock();		
}

void GLvideo_rt::setLuminanceMultiplier(float m)
{
	mutex.lock();
	m_luminanceMultiplier = m;
	mutex.unlock();	
}
    
void GLvideo_rt::setChrominanceMultiplier(float m)
{
	mutex.lock();
	m_chrominanceMultiplier = m;
	mutex.unlock();		
}

void GLvideo_rt::setLuminanceOffset1(float o)
{
	mutex.lock();
	m_luminanceOffset1 = o;	
	mutex.unlock();	
}
    
void GLvideo_rt::setChrominanceOffset1(float o)
{
	mutex.lock();
	m_chrominanceOffset1 = o;
	mutex.unlock();		
}

void GLvideo_rt::setLuminanceOffset2(float o)
{
	mutex.lock();
	m_luminanceOffset2 = o;
	mutex.unlock();	
}
    
void GLvideo_rt::setChrominanceOffset2(float o)
{
	mutex.lock();
	m_chrominanceOffset2 = o;
	mutex.unlock();		
}

void GLvideo_rt::resizeViewport(int width, int height)
{
	mutex.lock();
	m_displaywidth = width;
	m_displayheight = height;
	m_doResize = true;
	mutex.unlock();
}    

#ifdef HAVE_FTGL
void GLvideo_rt::renderOSD(VideoData *videoData, FTFont *font, float fps, int osd, float osdScale)
{	
	//bounding box of text
	float cx1, cy1, cz1, cx2, cy2, cz2;	 		
				
	//text string
	char str[255];
	switch (osd) {
	case 1: 
		sprintf(str, "%06ld", videoData->frameNum);
	 	font->BBox("000000", cx1, cy1, cz1, cx2, cy2, cz2);
		break;
	
	case 2: 
		sprintf(str, "%06ld, fps:%3.2f", videoData->frameNum, fps);
	 	font->BBox("000000, fps:000.00", cx1, cy1, cz1, cx2, cy2, cz2);
		break;
	
	case 3: 
		sprintf(str, "%06ld, %s:%s", videoData->frameNum, m_interlacedSource ? "Int" : "Prog", m_deinterlace ? "On" : "Off");
	 	font->BBox("000000, Prog:Off", cx1, cy1, cz1, cx2, cy2, cz2);
		break;
		
	case 4: 
		sprintf(str, "%s", caption);
	 	font->BBox(caption, cx1, cy1, cz1, cx2, cy2, cz2);
		break;

				
	default: str[0] = '\0';
	}

	//text box location, defaults to bottom left
	float tx=0.05 * videoData->Ywidth;
	float ty=0.05 * videoData->Yheight;

	if(osd==4) {
		//put the caption in the middle of the screen
		tx = videoData->Ywidth - (cx2 - cx1);
		tx/=2;	
	}
	
	//black box that text is rendered onto, larger than the text by 'border'
	float border = 10;	
	float bx1, by1, bx2, by2;
	bx1 = cx1 - border; 
	by1 = cy1 - border; 
	bx2 = cx2 + border;
	by2 = cy2 + border;
	
	//box beind text													
	glPushMatrix();
	glTranslated(tx, ty, 0);
	glScalef(osdScale, osdScale, 0);	//scale the on screen display
		
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.0, 0.0, 0.0, 0.7);
								
	glBegin(GL_QUADS);
	glVertex2f(bx1, by1);
	glVertex2f(bx1, by2);
	glVertex2f(bx2, by2);
	glVertex2f(bx2, by1);																				
	glEnd();			
						
	//text
	glColor4f(1.0, 1.0, 1.0, 0.5);						
	font->Render(str);					
	glPopMatrix();	
}

void drawText(char *str, FTFont *font)
{
	glPushMatrix();		
	font->Render(str);
	glPopMatrix();	
}

void draw2Text(char *str1, char *str2, int h_spacing, FTFont *font)
{
	glPushMatrix();
	
	glPushMatrix();		
	font->Render(str1);
	glPopMatrix();
	
	glTranslated(h_spacing, 0, 0);
	
	glPushMatrix();		
	font->Render(str2);
	glPopMatrix();	
	
	glPopMatrix();
}

void GLvideo_rt::renderPerf(VideoData *videoData, FTFont *font)
{
	float tx = 0.05 * videoData->Ywidth;
	float ty = 0.95 * videoData->Yheight;
	char str[255];
	char str2[255];	
	float cx1, cy1, cz1, cx2, cy2, cz2;	
	
	font->BBox("0", cx1, cy1, cz1, cx2, cy2, cz2);	
	float spacing = (cy2-cy1) * 1.5;
	float width = cx2 - cx1;
	
	//black box that text is rendered onto, larger than the text by 'border'
	float border = 10;	
	float bx1, by1, bx2, by2;
	bx1 = -border; 
	by1 = spacing; 
	bx2 = width * 25;
	by2 = (spacing * -15) - border*2;
	
	glPushMatrix();
	glTranslated(tx, ty, 0);	//near the top left corner
	glScalef(0.25, 0.25, 0);	//reduced in size compared to OSD
	
	//box beind text	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.0, 0.0, 0.0, 0.7);
								
	glBegin(GL_QUADS);
	glVertex2f(bx1, by1);
	glVertex2f(bx1, by2);
	glVertex2f(bx2, by2);
	glVertex2f(bx2, by1);																				
	glEnd();

	glColor4f(0.0, 1.0, 0.0, 0.5);

	sprintf(str, "Rendering");
	drawText(str, font);

	glColor4f(1.0, 1.0, 1.0, 0.5);

	glTranslated(0, -spacing, 0);
	sprintf(str, "ReadData");
	sprintf(str2, "%dms", perf_readData);
	draw2Text(str, str2, (int)width*15, font);

	glTranslated(0, -spacing, 0);
	sprintf(str, "ConvertFormat");
	sprintf(str2, "%dms", perf_convertFormat);
	draw2Text(str, str2, (int)width*15, font);

	glTranslated(0, -spacing, 0);
	sprintf(str, "UpdateVars");
	sprintf(str2, "%dms", perf_updateVars);
	draw2Text(str, str2, (int)width*15, font);

	glTranslated(0, -spacing, 0);
	sprintf(str, "RepeatWait");
	sprintf(str2, "%dms", perf_repeatWait);
	draw2Text(str, str2, (int)width*15, font);

	glTranslated(0, -spacing, 0);	
	sprintf(str, "Upload");
	sprintf(str2, "%dms", perf_upload);
	draw2Text(str, str2, (int)width*15, font);

	glTranslated(0, -spacing, 0);	
	sprintf(str, "RenderVideo");
	sprintf(str2, "%dms", perf_renderVideo);
	draw2Text(str, str2, (int)width*15, font);

	glTranslated(0, -spacing, 0);
	sprintf(str, "RenderOSD");
	sprintf(str2, "%dms", perf_renderOSD);
	draw2Text(str, str2, (int)width*15, font);

	glTranslated(0, -spacing, 0);
	sprintf(str, "SwapBuffers");
	sprintf(str2, "%dms", perf_swapBuffers);
	draw2Text(str, str2, (int)width*15, font);

	glTranslated(0, -spacing, 0);	
	sprintf(str, "Interval");
	sprintf(str2, "%dms", perf_interval);
	draw2Text(str, str2, (int)width*15, font);
	
	glColor4f(0.0, 1.0, 0.0, 0.5);

	glTranslated(0, -spacing, 0);
	glTranslated(0, -spacing, 0);
	sprintf(str, "I/O");
	drawText(str, font);

	glColor4f(1.0, 1.0, 1.0, 0.5);

	glTranslated(0, -spacing, 0);
	sprintf(str, "Future Queue");
	sprintf(str2, "%d", perf_futureQueue);
	draw2Text(str, str2, (int)width*17, font);
	
	glTranslated(0, -spacing, 0);
	sprintf(str, "Past Queue");
	sprintf(str2, "%d", perf_pastQueue);
	draw2Text(str, str2, (int)width*17, font);

	glTranslated(0, -spacing, 0);
	sprintf(str, "I/O Thread Load");
	sprintf(str2, "%d%%", perf_IOLoad);
	draw2Text(str, str2, (int)width*17, font);

	glTranslated(0, -spacing, 0);
	sprintf(str, "Read Rate");
	sprintf(str2, "%dMB/s", perf_IOBandwidth);
	draw2Text(str, str2, (int)width*17, font);
	
	glPopMatrix();
}
#endif

void GLvideo_rt::uploadTextures(VideoData *videoData)
{
	void *ioMem;

	if(videoData->isPlanar) {
		
		glBindTexture(GL_TEXTURE_RECTANGLE_NV, 1);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, io_buf[0]);
		ioMem = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
        assert(ioMem);
		memcpy(ioMem, videoData->Udata, videoData->UdataSize);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, videoData->Cwidth, videoData->Cheight, videoData->glFormat, videoData->glType, NULL);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

		glBindTexture(GL_TEXTURE_RECTANGLE_NV, 2);	
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, io_buf[1]);
		ioMem = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
        assert(ioMem);
		memcpy(ioMem, videoData->Vdata, videoData->VdataSize);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, videoData->Cwidth, videoData->Cheight, videoData->glFormat, videoData->glType, NULL);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);	
	}
	
	//luminance (or muxed YCbCr) data
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, 3);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, io_buf[2]);
	ioMem = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
    assert(ioMem);
	memcpy(ioMem, videoData->Ydata, videoData->YdataSize);
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, videoData->glYTextureWidth, videoData->Yheight, videoData->glFormat, videoData->glType, NULL);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
		
}

void GLvideo_rt::renderVideo(VideoData *videoData)
{		
	glClear(GL_COLOR_BUFFER_BIT);
	
	glBegin(GL_QUADS);
		glTexCoord2i(0,0);
		glVertex2i(0,0);
		glTexCoord2i(videoData->Ywidth,0);
		glVertex2i(videoData->Ywidth,0);
		glTexCoord2i(videoData->Ywidth, videoData->Yheight);
		glVertex2i(videoData->Ywidth, videoData->Yheight);
		glTexCoord2i(0, videoData->Yheight);
		glVertex2i(0, videoData->Yheight);
	glEnd();				
}

//get pointers to openGL functions and extensions
//FIXME - this does nothing if any of these calls fail
//FIXME - this is totally different on OSX - thanks Apple. Details are on the apple developer site
//FIXME - 
void GLvideo_rt::getGLfunctions()
{
#ifdef Q_OS_LINUX
#define XglGetProcAddress(str) glXGetProcAddress((GLubyte *)str)
#endif

#ifdef Q_OS_WIN32
#define XglGetProcAddress(str) wglGetProcAddress((LPCSTR)str)
#endif
	
#ifdef Q_OS_MAC
#define XglGetProcAddress(str) aglGetProcAddress((char *)str)
#endif

#ifdef Q_OS_WIN32
	//enable openGL vsync on windows	
	wglSwapInterval = (PFNWGLSWAPINTERVALFARPROC) XglGetProcAddress("wglSwapIntervalEXT");
	if (wglSwapInterval)
		wglSwapInterval(1);

	if(DEBUG) printf("wglSwapInterval=%p\n",  wglSwapInterval);
#endif

#ifdef Q_WS_X11
	//functions for controlling vsync on X11	
	glXGetVideoSyncSGI = (PFNGLXGETVIDEOSYNCSGIPROC) XglGetProcAddress("glXGetVideoSyncSGI");
	glXWaitVideoSyncSGI = (PFNGLXWAITVIDEOSYNCSGIPROC) XglGetProcAddress("glXWaitVideoSyncSGI");

	if(DEBUG) {
		printf("glXGetVideoSyncSGI=%p\n",  glXGetVideoSyncSGI);
		printf("glXWaitVideoSyncSGI=%p\n",  glXWaitVideoSyncSGI);			
	}	
#endif
	
    glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC) XglGetProcAddress("glLinkProgramARB");
    glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC) XglGetProcAddress("glAttachObjectARB");
    glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC) XglGetProcAddress("glCreateProgramObjectARB");
    glGetInfoLogARB = (PFNGLGETINFOLOGARBPROC) XglGetProcAddress("glGetInfoLogARB");
    glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC) XglGetProcAddress("glGetObjectParameterivARB");
    glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC) XglGetProcAddress("glCompileShaderARB");
    glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC) XglGetProcAddress("glShaderSourceARB");
    glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC) XglGetProcAddress("glCreateShaderObjectARB");
    glBufferData = (PFNGLBUFFERDATAPROC) XglGetProcAddress("glBufferData");
    glBindBuffer = (PFNGLBINDBUFFERPROC) XglGetProcAddress("glBindBuffer");
    glUniform1iARB = (PFNGLUNIFORM1IARBPROC) XglGetProcAddress("glUniform1iARB");
    glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC) XglGetProcAddress("glGetUniformLocationARB");

    glUnmapBuffer = (PFNGLUNMAPBUFFERPROC) XglGetProcAddress("glUnmapBuffer");
    glMapBuffer = (PFNGLMAPBUFFERPROC) XglGetProcAddress("glMapBuffer");
    glUniform3fvARB = (PFNGLUNIFORM3FVARBPROC) XglGetProcAddress("glUniform3fvARB");
    glUniformMatrix3fvARB = (PFNGLUNIFORMMATRIX3FVARBPROC) XglGetProcAddress("glUniformMatrix3fvARB");
    glUniform1fARB = (PFNGLUNIFORM1FARBPROC) XglGetProcAddress("glUniform1fARB");
    glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC) XglGetProcAddress("glUseProgramObjectARB");
    glGenBuffers = (PFNGLGENBUFFERSPROC) XglGetProcAddress("glGenBuffers");

	glActiveTexture = (PFNGLACTIVETEXTUREPROC) XglGetProcAddress("glActiveTexture");
}


void GLvideo_rt::run()
{
	if(DEBUG) printf("Starting renderthread\n");
	
	VideoData *videoData = NULL;

	QTime fpsIntervalTime;		//measures FPS, averaged over several frames
	QTime frameIntervalTime;	//measured the total period of each frame
	QTime perfTimer;			//performance timer for measuring indivdual processes during rendering
	int fpsAvgPeriod=1;
	float fps = 0;
				
	//flags and status
	int lastsrcwidth = 0;
	int lastsrcheight = 0;
	bool createGLTextures = false;
	bool updateShaderVars = false;

	//declare shadow variables for the thread worker and initialise them
	mutex.lock();
	bool aspectLock = m_aspectLock;
	bool doRendering = m_doRendering;
	bool doResize = m_doResize;
	bool showLuminance = m_showLuminance;
	bool showChrominance = m_showChrominance;	
	bool changeFont = m_changeFont;
	int displaywidth = m_displaywidth;
	int displayheight = m_displayheight;
	int frameRepeats = m_frameRepeats;
	int framePolarity = m_framePolarity;
	bool interlacedSource = m_interlacedSource;
	bool deinterlace = m_deinterlace;
	bool matrixScaling = m_matrixScaling;
	bool changeMatrix = m_changeMatrix;
	int field = 0;
	int direction = 0;
	int currentShader = 0;	
	int osd = m_osd;
	bool perf = m_perf;
	float luminanceOffset1 = m_luminanceOffset1;
	float chrominanceOffset1 = m_chrominanceOffset1;
	float luminanceMultiplier = m_luminanceMultiplier;
	float chrominanceMultiplier = m_chrominanceMultiplier;
	float luminanceOffset2 = m_luminanceOffset2;
	float chrominanceOffset2 = m_chrominanceOffset2;
	float colourMatrix[9];
	float matrixKr = m_matrixKr;
	float matrixKg = m_matrixKg;
	float matrixKb = m_matrixKb;
	float osdScale = m_osdScale;
	
	mutex.unlock();

#ifdef HAVE_FTGL
	FTFont *font = NULL;
#endif 	
            	
	//initialise OpenGL	
	getGLfunctions();	
	glw.makeCurrent();

#ifdef Q_OS_MACX
	my_aglSwapInterval(1);
#endif
		
    glGenBuffers(3, io_buf);	
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0,0, displayheight, displaywidth);
    glClearColor(0, 0, 0, 0);
    glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
#ifdef Q_WS_X11
	unsigned int retraceCount = glXGetVideoSyncSGI(&retraceCount);
#endif

	compileFragmentShaders();		
    glUseProgramObjectARB(programs[currentShader]);
    	
	while (doRendering) {
		perfTimer.start();
		//update shadow variables
		mutex.lock();
		
			//values
			displaywidth = m_displaywidth;
			displayheight = m_displayheight;
			frameRepeats = m_frameRepeats;
			framePolarity = m_framePolarity;
			osd = m_osd;
			perf = m_perf;
												
			doResize = m_doResize;				
			m_doResize = false;
						
			//boolean flags
			doRendering = m_doRendering;
			
			if(m_changeMatrix == true) {
				matrixScaling = m_matrixScaling;
				matrixKr = m_matrixKr;
				matrixKg = m_matrixKg;
				matrixKb = m_matrixKb;								
				changeMatrix = true;
				updateShaderVars = true;				
				m_changeMatrix = false;
			}	
			
			if(m_changeFont) {
				changeFont = true;
				m_changeFont = false;	
			}
			
			//check for changed aspect ratio lock
			if(aspectLock != m_aspectLock) {
				aspectLock = m_aspectLock;
				doResize = true;
			}
			
			if(showLuminance != m_showLuminance) {
				showLuminance = m_showLuminance;
				updateShaderVars = true;	
			}

			if(showChrominance != m_showChrominance) {
				showChrominance = m_showChrominance;
				updateShaderVars = true;	
			}

			if(luminanceMultiplier != m_luminanceMultiplier) {
				luminanceMultiplier = m_luminanceMultiplier;
				updateShaderVars = true;
			}

			if(chrominanceMultiplier != m_chrominanceMultiplier) {
				chrominanceMultiplier = m_chrominanceMultiplier;
				updateShaderVars = true;
			}

			if(luminanceOffset1 != m_luminanceOffset1) {
				luminanceOffset1 = m_luminanceOffset1;
				updateShaderVars = true;	
			}

			if(chrominanceOffset1 != m_chrominanceOffset1) {
				chrominanceOffset1 = m_chrominanceOffset1;
				updateShaderVars = true;	
			}

			if(luminanceOffset2 != m_luminanceOffset2) {
				luminanceOffset2 = m_luminanceOffset2;
				updateShaderVars = true;	
			}

			if(chrominanceOffset2 != m_chrominanceOffset2) {
				chrominanceOffset2 = m_chrominanceOffset2;
				updateShaderVars = true;	
			}

			if(interlacedSource != m_interlacedSource) {
				interlacedSource = m_interlacedSource;
				updateShaderVars = true;	
			}

			if(deinterlace != m_deinterlace) {
				deinterlace = m_deinterlace;
				updateShaderVars = true;	
			}
	
		mutex.unlock();
		perf_getParams = perfTimer.elapsed();

		if(changeMatrix) {
			buildColourMatrix(colourMatrix, matrixKr, matrixKg, matrixKb, matrixScaling, matrixScaling);
			changeMatrix = false;	
		}

		//read frame data, one frame at a time, or after we have displayed the second field
		perfTimer.restart();		
		if(interlacedSource == 0 || field == 1) {
			videoData = glw.vr.getNextFrame();
			direction = glw.vr.getDirection();
			perf_futureQueue = glw.vr.getFutureQueueLen();
			perf_pastQueue   = glw.vr.getPastQueueLen();
			perf_IOLoad      = glw.vr.getIOLoad();
			perf_IOBandwidth = glw.vr.getIOBandwidth();
			perf_readData = perfTimer.elapsed();
		}		
		
		if(videoData) {
			perfTimer.restart();			
			switch(videoData->renderFormat) {
			
				case VideoData::V210:
					//check for v210 data - it's very difficult to write a shader for this
					//due to the lack of GLSL bitwise operators, and it's insistance on filtering the textures
					videoData->convertV210();			
					break;
					
				case VideoData::V16P4:
				case VideoData::V16P2:
				case VideoData::V16P0:
					//convert 16 bit data to 8bit
					//this avoids endian-ness issues between the host and the GPU
					//and reduces the bandwidth to the GPU
					videoData->convertPlanar16();
					break;
					
				default:
					//no conversion needed
					break;
			}
			perf_convertFormat = perfTimer.elapsed();
		}

		//check for video dimensions changing		
		if(videoData != NULL) {
			if(lastsrcwidth != videoData->Ywidth || lastsrcheight != videoData->Yheight) {
			
			    glOrtho(0, videoData->Ywidth ,0 , videoData->Yheight, -1 ,1);

				doResize = true;				
				createGLTextures = true;
				updateShaderVars = true;
				
				lastsrcwidth = videoData->Ywidth;
				lastsrcheight = videoData->Yheight;
			}
		}
		
		//check for the shader changing
		if(videoData) {
			int newShader = currentShader;
			
			if(videoData->isPlanar)
				newShader = shaderPlanar;
			else
				newShader = shaderUYVY;
														
			if(newShader != currentShader) {
				if(DEBUG) printf("Changing shader from %d to %d\n", currentShader, newShader);
				createGLTextures = true;
				updateShaderVars = true;
				currentShader = newShader;
    			glUseProgramObjectARB(programs[currentShader]);					
			}							
		}		
		
		if(createGLTextures) {
			if(DEBUG) printf("Creating GL textures\n");
			//create the textures on the GPU
			createTextures(videoData, currentShader);
			createGLTextures = false;
		}			
		
		if(updateShaderVars && videoData) {
			perfTimer.restart();
			if(DEBUG) printf("Updating fragment shader variables\n");

			//data about the input file to the shader			
		    int i=glGetUniformLocationARB(programs[currentShader], "Yheight");
    		glUniform1fARB(i, videoData->Yheight);

		    i=glGetUniformLocationARB(programs[currentShader], "Ywidth");
    		glUniform1fARB(i, videoData->Ywidth);

		    i=glGetUniformLocationARB(programs[currentShader], "CHsubsample");
    		glUniform1fARB(i, (float)(videoData->Ywidth / videoData->Cwidth));

		    i=glGetUniformLocationARB(programs[currentShader], "CVsubsample");
    		glUniform1fARB(i, (float)(videoData->Yheight / videoData->Cheight));

			//settings from the c++ program to the shader
		    i=glGetUniformLocationARB(programs[currentShader], "yuvOffset1");
		    float offset1[3];
		    offset1[0] = matrixScaling ? luminanceOffset1 : 0.0;	//don't subtract the initial Y offset if the matrix is unscaled
		    offset1[1] = chrominanceOffset1;
		    offset1[2] = chrominanceOffset1;
			glUniform3fvARB(i, 1, &offset1[0]);		    

		    i=glGetUniformLocationARB(programs[currentShader], "yuvMul");			
			float mul[3];
			mul[0] = showLuminance   ? luminanceMultiplier   : 0.0;
			mul[1] = showChrominance ? chrominanceMultiplier : 0.0;
			mul[2] = showChrominance ? chrominanceMultiplier : 0.0;
			glUniform3fvARB(i, 1, &mul[0]); 									

		    i=glGetUniformLocationARB(programs[currentShader], "yuvOffset2");
		    float offset2[3];
		    offset2[0] = showLuminance ? luminanceOffset2 : 0.5;	//when luminance is off, set to mid grey
		    offset2[1] = chrominanceOffset2;
		    offset2[2] = chrominanceOffset2;
			glUniform3fvARB(i, 1, &offset2[0]);		    
    			    					
		    i=glGetUniformLocationARB(programs[currentShader], "colorMatrix");	    
    		glUniformMatrix3fvARB(i, 1, false, colourMatrix);

		   	i=glGetUniformLocationARB(programs[currentShader], "interlacedSource");
    		glUniform1iARB(i, interlacedSource);

		   	i=glGetUniformLocationARB(programs[currentShader], "deinterlace");
    		glUniform1iARB(i, deinterlace);
			updateShaderVars = false;
			perf_updateVars = perfTimer.elapsed();
		}	
												
		if(doResize && videoData != NULL) {
			//resize the viewport, once we have some video
			if(DEBUG) printf("Resizing to %d, %d\n", displaywidth, displayheight);
			
			float sourceAspect = (float)videoData->Ywidth / (float)videoData->Yheight;
			float displayAspect = (float)displaywidth / (float)displayheight;
		
			if(sourceAspect == displayAspect || aspectLock == false) {
				glViewport(0, 0, displaywidth, displayheight);				
			}
			else {
				if(displayAspect > sourceAspect) {
					//window is wider than image should be
					int width = (int)(displayheight*sourceAspect);
					int offset = (displaywidth - width) / 2;
					glViewport(offset, 0, width, displayheight);
				}
				else {
					int height = (int)(displaywidth/sourceAspect);
					int offset = (displayheight - height) / 2;								
					glViewport(0, offset, displaywidth, height);
				}
			}			
			doResize = false;
		}
		
#ifdef HAVE_FTGL
		if(changeFont && videoData != NULL) {
			if(font) 
				delete font;
						
			font = new FTGLPolygonFont((const char *)fontFile);
			font->FaceSize(72);
			font->CharMap(ft_encoding_unicode);
						
			changeFont = false;	
		}				
#endif

		

#ifdef Q_WS_X11
        if (frameRepeats > 1) {
        	perfTimer.restart();
         	glXWaitVideoSyncSGI(frameRepeats, framePolarity, &retraceCount);
         	perf_repeatWait = perfTimer.elapsed();
        }
#endif

		if(interlacedSource) {
		   	int i=glGetUniformLocationARB(programs[currentShader], "field");
	   		glUniform1iARB(i, field);
	   		
		   	i=glGetUniformLocationARB(programs[currentShader], "direction");
    		glUniform1iARB(i, direction);
		}			
																					
		if(videoData) {
			
			//upload the texture data for each new frame, or for before we render the first field
			perfTimer.restart();
			if(interlacedSource == 0 || field == 0) uploadTextures(videoData);
			perf_upload = perfTimer.elapsed();
						
			perfTimer.restart();
			renderVideo(videoData);			
			perf_renderVideo = perfTimer.elapsed();
						
#ifdef HAVE_FTGL
			perfTimer.restart();
			glUseProgramObjectARB(0);							
			if(osd && font != NULL) renderOSD(videoData, font, fps, osd, osdScale);
			perf_renderOSD = perfTimer.elapsed();						
			if(perf==true && font != NULL) renderPerf(videoData, font);
			glUseProgramObjectARB(programs[currentShader]);			
#endif			
		}
																															   			   																									   			  
		glFlush();
		perfTimer.restart();		
		glw.swapBuffers();
		perf_swapBuffers = perfTimer.elapsed();		
		
		if(interlacedSource) {
			//move to the next field			
			field++;
			if(field > 1) field = 0;	
		}

		//measure overall frame period		
  		perf_interval = frameIntervalTime.elapsed();
  		frameIntervalTime.restart();
		
  		//calculate FPS  		
  		if (fpsAvgPeriod == 0) {
  			int fintvl = fpsIntervalTime.elapsed();
  			if (fintvl) fps = 10*1e3/fintvl;
  			fpsIntervalTime.start();
  		}
		fpsAvgPeriod = (fpsAvgPeriod + 1) %10;
	}
}

