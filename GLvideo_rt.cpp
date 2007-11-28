#include "GLvideo_rt.h"
#include "GLvideo_mt.h"

#ifdef HAVE_FTGL
#include "FTGL/FTGLPolygonFont.h"
#endif

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

#ifdef Q_OS_UNIX
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

#define DEBUG 0
#define PERF 0
//------------------------------------------------------------------------------------------
//shader program for planar video formats
//should work with all planar chroma subsamplings
//glInternalFormat=GL_LUMINANCE glFormat=GL_LUMINANCE glType=GL_UNSIGNED_BYTE
static char *shaderPlanarSrc=
  "#extension ARB_texture_rectangle : enable\n"
  "#extension GL_ARB_texture_rect : enable\n"
  "uniform samplerRect Ytex;\n"
  "uniform samplerRect Utex,Vtex;\n"
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
  " vec3 rgb;\n"
  
  " nx=gl_TexCoord[0].x;\n"
  " ny=Yheight-gl_TexCoord[0].y;\n"

  " if(direction < 0) field = 1 - field;\n"		//swap field order when playing interlaced pictures backwards

  " if((interlacedSource == true) && (deinterlace == true) && (mod(floor(ny) + field, 2.0) > 0.5)) {\n"   
  "     //interpolated line in a field\n"
  "     yuv[0] = textureRect(Ytex,vec2(nx, (ny+1))).r + textureRect(Ytex,vec2(nx, (ny-1))).r;\n" 
  "     yuv[1] = textureRect(Utex,vec2(nx/CHsubsample, (ny+1)/CVsubsample)).r + textureRect(Utex,vec2(nx/CHsubsample, (ny-1)/CVsubsample)).r;\n"
  "     yuv[2] = textureRect(Vtex,vec2(nx/CHsubsample, (ny+1)/CVsubsample)).r + textureRect(Vtex,vec2(nx/CHsubsample, (ny-1)/CVsubsample)).r;\n"  
  "     yuv = yuv / 2.0;"
  " }\n"
  " else {\n"
  "     //non interpolated line in a field, or non interlaced\n"
  "     yuv[0]=textureRect(Ytex,vec2(nx,ny)).r;\n"
  "     yuv[1]=textureRect(Utex,vec2(nx/CHsubsample, ny/CVsubsample)).r;\n"
  "     yuv[2]=textureRect(Vtex,vec2(nx/CHsubsample, ny/CVsubsample)).r;\n"    
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
  "#extension ARB_texture_rectangle : enable\n"
  "#extension GL_ARB_texture_rect : enable\n"
  "uniform samplerRect Ytex;\n"
  "uniform samplerRect Utex,Vtex;\n"
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
    
  " nx=gl_TexCoord[0].x;\n"
  " ny=Yheight-gl_TexCoord[0].y;\n"
 
  " if(direction < 0) field = 1 - field;\n"		//swap field order when playing interlaced pictures backwards	
 
  " if((interlacedSource == true) && (deinterlace == true) && (mod(floor(ny) + field, 2.0) > 0.5)) {\n"   
  "     //interpolated line in a field\n"
  "     above = textureRect(Ytex, vec2(floor(nx/2), ny+1));\n"
  "     below = textureRect(Ytex, vec2(floor(nx/2), ny-1));\n"

  "     yuv[0]  = (fract(nx/2) < 0.5) ? above.g : above.a;\n"
  "     yuv[0] += (fract(nx/2) < 0.5) ? below.g : below.a;\n"
  "     yuv[1] = above.r + below.r;\n"
  "     yuv[2] = above.b + below.b;\n"
  "     yuv = yuv / 2.0;"
  " }\n"
  " else {\n"
  "     //non interpolated line in a field, or non interlaced\n"
  "     rgba = textureRect(Ytex, vec2(floor(nx/2), ny));\n"	 //sample every other RGBA quad to get luminance
  "     yuv[0] = (fract(nx/2) < 0.5) ? rgba.g : rgba.a;\n"   //pick the correct luminance from G or A for this pixel
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
	compileFragmentShader((int)shaderV216,   shaderUYVYSrc);						
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
    	glBindTexture(GL_TEXTURE_RECTANGLE_NV,1);

    	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, videoData->glMinMaxFilter);
    	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, videoData->glMinMaxFilter);
    	//glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
		glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, videoData->glInternalFormat, videoData->Cwidth , videoData->Cheight, 0, videoData->glFormat, videoData->glType, NULL);
    
    	/* Select texture unit 2 as the active unit and bind the V texture. */
    	glActiveTexture(GL_TEXTURE2);
    	i=glGetUniformLocationARB(programs[currentShader], "Vtex");
    	glBindTexture(GL_TEXTURE_RECTANGLE_NV,2);
    	glUniform1iARB(i,2);  /* Bind Vtext to texture unit 2 */

    	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, videoData->glMinMaxFilter);
    	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, videoData->glMinMaxFilter);
    	//glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
		glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, videoData->glInternalFormat, videoData->Cwidth , videoData->Cheight, 0, videoData->glFormat, videoData->glType, NULL);
	}
    
    /* Select texture unit 0 as the active unit and bind the Y texture. */
    glActiveTexture(GL_TEXTURE3);
    i=glGetUniformLocationARB(programs[currentShader], "Ytex");
    glUniform1iARB(i,3);  /* Bind Ytex to texture unit 3 */
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,3);

    glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, videoData->glMinMaxFilter);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, videoData->glMinMaxFilter);
    //glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
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
void GLvideo_rt::renderOSD(VideoData *videoData, FTFont *font, float fps, int osd)
{
	//positions of text
	float tx=0.05 * videoData->Ywidth;
	float ty=0.05 * videoData->Yheight;
	float border = 10;
	
	//character size for '0'
	float cx1, cy1, cz1, cx2, cy2, cz2;	
	font->BBox("000000", cx1, cy1, cz1, cx2, cy2, cz2); 		
	
	//text box location
	float bx1, by1, bx2, by2;
	float charWidth = cx2 - cx1;
	float charHeight = cy2 - cy1;
	bx1 = tx - border; 
	by1 = ty - border; 
	bx2 = tx + charWidth + 2*border;
	by2 = ty + charHeight + border;
			
	//text string
	char str[20];
	switch (osd) {
	case 1: sprintf(str, "%06ld", videoData->frameNum); break;
	case 2: sprintf(str, "%06ld, fps:%3.2f", videoData->frameNum, fps); break;
	case 3: sprintf(str, "%06ld, %s:%s", videoData->frameNum, m_interlacedSource ? "Int" : "Prog", m_deinterlace ? "On" : "Off"); break;	
	default: str[0] = '\0';
	}
			
	//box beind text													
	glPushMatrix();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.0, 0.0, 0.0, 0.7);
	
	//glTranslated(80, 80, 0);							
	glBegin(GL_QUADS);
	glVertex2f(bx1, by1);
	glVertex2f(bx1, by2);
	glVertex2f(bx2, by2);
	glVertex2f(bx2, by1);																				
	glEnd();			
	glPopMatrix();						

	//text
	glPushMatrix();
	glColor4f(1.0, 1.0, 1.0, 0.5);
	glTranslated(tx, ty, 0);						
	font->Render(str);					
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
#ifdef Q_WS_X11
#define XglGetProcAddress(str) glXGetProcAddress((GLubyte *)str)
#else
#define XglGetProcAddress(str) wglGetProcAddress((LPCSTR)str)
#endif
	
#ifdef Q_OS_WIN32
	//enable openGL vsync on windows	
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALFARPROC) XglGetProcAddress("wglSwapIntervalEXT");
	if (wglSwapIntervalEXT)
		wglSwapIntervalEXT(true);		
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

	QTime frameIntervalTime;
	QTime perfTimer;
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
		
	mutex.unlock();

#ifdef HAVE_FTGL
	FTFont *font = NULL;
#endif 	
            	
	//initialise OpenGL	
	getGLfunctions();	
	glw.makeCurrent();	
    glGenBuffers(3, io_buf);	
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0,0, displayheight, displaywidth);
    glClearColor(0, 0, 0, 0);
    //glHint(GL_POLYGON_SMOOTH_HINT,GL_NICEST);
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
		if(PERF) printf("  getparams %d\n", perfTimer.elapsed());

		if(changeMatrix) {
			buildColourMatrix(colourMatrix, matrixKr, matrixKg, matrixKb, matrixScaling, matrixScaling);
			changeMatrix = false;	
		}

		//read frame data, one frame at a time, or after we have displayed the second field
		if(interlacedSource == 0 || field == 1) {
			videoData = glw.vr.getNextFrame();
			direction = glw.vr.getDirection();
		}
		
		if(PERF) printf("  readData %d\n", perfTimer.elapsed());
		
		//check for v210 data - it's very difficult to write a shader for this
		//due to the lack of GLSL bitwise operators, and it's insistance on filtering the textures
		if(videoData) {
			if(videoData->renderFormat == VideoData::V210)
				videoData->convertV210();
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
		if(PERF) printf("  dimensions %d\n", perfTimer.elapsed());
		
		//check for the shader changing
		
		if(videoData) {
			int newShader = currentShader;
			
			switch(videoData->renderFormat) {
				case VideoData::YV12:
				case VideoData::I420:
				case VideoData::Planar411:
				case VideoData::Planar444: 
					newShader = shaderPlanar;
					break;
					
				case VideoData::UYVY:
					newShader = shaderUYVY;
					break;
					
				case VideoData::V216:
					newShader = shaderV216;
					break;
															
				default:
					//uh-oh!
					break;
			}	
			
			if(newShader != currentShader) {
				if(DEBUG) printf("Changing shader from %d to %d\n", currentShader, newShader);
				createGLTextures = true;
				updateShaderVars = true;
				currentShader = newShader;
    			glUseProgramObjectARB(programs[currentShader]);					
			}							
		}		
		if(PERF) printf("  shader %d\n", perfTimer.elapsed());
		
		if(createGLTextures) {
			if(DEBUG) printf("Creating GL textures\n");
			//create the textures on the GPU
			createTextures(videoData, currentShader);
			createGLTextures = false;
		}
		if(PERF) printf("  createTexture %d\n", perfTimer.elapsed());			
		
		if(updateShaderVars && videoData) {
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
		}	
		if(PERF) printf("  shaderVars %d\n", perfTimer.elapsed());
												
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
		if(PERF) printf("  resize %d\n", perfTimer.elapsed());
		
#ifdef HAVE_FTGL
		if(changeFont) {
			printf("Changing font!!!!\n");
			if(font) 
				delete font;
			
			font = new FTGLPolygonFont((const char *)fontFile);
			font->FaceSize(144);
			font->CharMap(ft_encoding_unicode);
			changeFont = false;	
		}		
		if(PERF) printf("  changefont %d %d\n", perfTimer.elapsed(), changeFont);		
#endif

		

#ifdef Q_WS_X11		
        if (frameRepeats > 1)
         	glXWaitVideoSyncSGI(frameRepeats, framePolarity, &retraceCount);
		if(PERF) printf("  syncwait %d %d\n", perfTimer.elapsed(), frameRepeats);
#endif

		if(interlacedSource) {
		   	int i=glGetUniformLocationARB(programs[currentShader], "field");
	   		glUniform1iARB(i, field);
	   		
		   	i=glGetUniformLocationARB(programs[currentShader], "direction");
    		glUniform1iARB(i, direction);
		}			
		if(PERF) printf("  field %d\n", perfTimer.elapsed());
																					
		if(videoData) {
			
			//upload the texture data for each new frame, or for before we render the first field
			if(interlacedSource == 0 || field == 0) uploadTextures(videoData);
			if(PERF) printf("  upload %d\n", perfTimer.elapsed());
						
			renderVideo(videoData);			
			if(PERF) printf("  render %d\n", perfTimer.elapsed());
						
#ifdef HAVE_FTGL
			glUseProgramObjectARB(0);							
			if(osd && font != NULL) renderOSD(videoData, font, fps, osd);
			glUseProgramObjectARB(programs[currentShader]);
			if(PERF) printf("  osd %d\n", perfTimer.elapsed());			
#endif			
		}
																															   			   																									   			  
		glFlush();
		if(PERF) printf("  flush %d\n", perfTimer.elapsed());		
		glw.swapBuffers();
		if(PERF) printf("  swapbuffers %d\n", perfTimer.elapsed());		
		
		if(interlacedSource) {
			//move to the next field			
			field++;
			if(field > 1) field = 0;	
		}
		       	
  		//calculate FPS
  		int intvl = perfTimer.restart();
  		if(PERF) assert(intvl < 100);
  		if(PERF) printf("interval = %d\n", frameIntervalTime.restart());   		
  		if (fpsAvgPeriod == 0) {
  			int fintvl = frameIntervalTime.elapsed();
  			if (fintvl) fps = 10*1e3/fintvl;
  			frameIntervalTime.start();
  		}
		fpsAvgPeriod = (fpsAvgPeriod + 1) %10;
	}
}

