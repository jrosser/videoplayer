#define GL_GLEXT_PROTOTYPES 1

#include "GLvideo_rt.h"
#include "GLvideo_mt.h"
#include "GL/glx.h"

#include "FTGL/FTGLPolygonFont.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define DEBUG 0

//------------------------------------------------------------------------------------------
//shader program for planar video formats
//should work with all planar chroma subsamplings
//glInternalFormat=GL_LUMINANCE glFormat=GL_LUMINANCE glType=GL_UNSIGNED_BYTE
static char *shaderPlanarSrc=
  "uniform samplerRect Ytex;\n"
  "uniform samplerRect Utex,Vtex;\n"
  "uniform float Yheight, Ywidth;\n" 
  "uniform float CHsubsample, CVsubsample;\n"
  "uniform mat3 colorMatrix;\n"
  "uniform vec3 yuvOffset;\n"
        
  "void main(void) {\n"
  " float nx,ny,a;\n"
  " vec3 yuv;\n"
  " vec3 rgb;\n"
  
  " nx=gl_TexCoord[0].x;\n"
  " ny=Yheight-gl_TexCoord[0].y;\n"
  
  " yuv[0]=textureRect(Ytex,vec2(nx,ny)).r;\n"
  " yuv[1]=textureRect(Utex,vec2(nx/CHsubsample, ny/CVsubsample)).r;\n"
  " yuv[2]=textureRect(Vtex,vec2(nx/CHsubsample, ny/CVsubsample)).r;\n"

  " yuv = yuv + yuvOffset;\n"  
  " rgb = yuv * colorMatrix;\n"

  "	a=1.0;\n"
      
  " gl_FragColor=vec4(rgb ,a);\n"
  "}\n";

//------------------------------------------------------------------------------------------
//shader program for UYVY muxed 8 bit data with glInternalFormat=GL_RGBA glFormat=GL_RGBA glType=GL_UNSIGNED_BYTE
//shader program for v216 muxed 16 bit data with glInternalFormat=GL_RGBA glFormat=GL_RGBA glType=GL_UNSIGNED_SHORT
//there are 2 luminance samples per RGBA quad, so divide the horizontal location by two to use each RGBA value twice
static char *shaderUYVYSrc=
  "uniform samplerRect Ytex;\n"
  "uniform samplerRect Utex,Vtex;\n"
  "uniform float Yheight, Ywidth;\n" 
  "uniform float CHsubsample, CVsubsample;\n"
  "uniform mat3 colorMatrix;\n"
  "uniform vec3 yuvOffset;\n"
        
  "void main(void) {\n"
  " float nx,ny,a;\n"
  " vec3 yuv;\n"
  " vec4 rgba;\n"
  " vec3 rgb;\n"

  " nx=gl_TexCoord[0].x;\n"
  " ny=Yheight-gl_TexCoord[0].y;\n"
 
  " rgba = textureRect(Ytex, vec2(floor(nx/2), ny));\n"	 //sample every other RGBA quad to get luminance  
  " yuv[0] = (fract(nx/2) < 0.5) ? rgba.g : rgba.a;\n"   //pick the correct luminance from G or A for this pixel

  " rgba = textureRect(Ytex, vec2((nx/2), ny));\n"		 //sample chrominance at 0, 0.5, 1.0, 1.5... to get interpolation to 4:4:4
  " yuv[1] = rgba.r;\n"
  " yuv[2] = rgba.b;\n"
  
  " yuv = yuv + yuvOffset;\n"  
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
	glw.lockMutex();
	m_doRendering = false;
	glw.unlockMutex();
}

void GLvideo_rt::setAspectLock(bool l)
{
	glw.lockMutex();

	if(m_aspectLock != l) {
		m_aspectLock = l;
		m_doResize = true;
	}
	
	glw.unlockMutex();	
}

void GLvideo_rt::setFrameRepeats(int repeats)
{
	glw.lockMutex();
	m_frameRepeats = repeats;
	glw.unlockMutex();		
}

void GLvideo_rt::toggleAspectLock(void)
{
	glw.lockMutex();
	m_aspectLock = !m_aspectLock;
	m_doResize = true;
	glw.unlockMutex();		
}

void GLvideo_rt::toggleOSD(void)
{
	glw.lockMutex();
	m_osd = !m_osd;	
	glw.unlockMutex();		
}
    
void GLvideo_rt::resizeViewport(int width, int height)
{
	glw.lockMutex();
	m_displaywidth = width;
	m_displayheight = height;
	m_doResize = true;
	glw.unlockMutex();
}    

void GLvideo_rt::renderOSD(VideoData *videoData, FTFont *font)
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
	sprintf(str, "%06ld", videoData->frameNum);
		
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
	glColor3f(1.0, 1.0, 1.0);
	glTranslated(tx, ty, 0);						
	font->Render(str);					
	glPopMatrix();	
}

void GLvideo_rt::renderVideo(VideoData *videoData)
{		
	glClear(GL_COLOR_BUFFER_BIT);

	void *ioMem;

	if(videoData->isPlanar) {
		
		glBindTexture(GL_TEXTURE_RECTANGLE_NV, 1);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, io_buf[0]);
		ioMem = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
		memcpy(ioMem, videoData->Udata, videoData->UdataSize);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, videoData->Cwidth, videoData->Cheight, videoData->glFormat, videoData->glType, NULL);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

		glBindTexture(GL_TEXTURE_RECTANGLE_NV, 2);	
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, io_buf[1]);
		ioMem = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
		memcpy(ioMem, videoData->Vdata, videoData->VdataSize);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, videoData->Cwidth, videoData->Cheight, videoData->glFormat, videoData->glType, NULL);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);	
	}
	
	//luminance (or muxed YCbCr) data
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, 3);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, io_buf[2]);
	ioMem = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
	memcpy(ioMem, videoData->Ydata, videoData->YdataSize);
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, videoData->glYTextureWidth, videoData->Yheight, videoData->glFormat, videoData->glType, NULL);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	
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
    
void GLvideo_rt::run()
{
	if(DEBUG) printf("Starting renderthread\n");
	
	VideoData *videoData = NULL;
    
	timeval last;
	
	int (*glXWaitVideoSyncSGI)(int , int , unsigned int *) = NULL;
	int (*glXGetVideoSyncSGI)(unsigned int *) = NULL;
		
	//get addresses of VSYNC extensions
	glXGetVideoSyncSGI = (int (*)(unsigned int *))glXGetProcAddress((GLubyte *)"glXGetVideoSyncSGI");
	glXWaitVideoSyncSGI = (int (*)(int, int, unsigned int *))glXGetProcAddress((GLubyte *)"glXWaitVideoSyncSGI");		
	if(DEBUG) printf("pget=%p pwait=%p\n", glXGetVideoSyncSGI, glXWaitVideoSyncSGI);
	
	//flags and status
	int lastsrcwidth = 0;
	int lastsrcheight = 0;
	bool createGLTextures = false;
	bool updateShaderVars = false;

	//declare shadow variables for the thread worker and initialise them
	glw.lockMutex();
	bool aspectLock = m_aspectLock;
	bool doRendering = m_doRendering;
	bool doResize = m_doResize;
	int displaywidth = m_displaywidth;
	int displayheight = m_displayheight;
	int frameRepeats = m_frameRepeats;
	int currentShader = 0;	
	glw.unlockMutex();
	
	FTFont *font = new FTGLPolygonFont((const char *)"/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans-Bold.ttf"); 	
    
    if(font->Error()) {
    	printf("Failed to open font file\n");
    	fflush(stdout);
    }

	font->FaceSize(144);
	font->CharMap(ft_encoding_unicode);
        	
	//initialise OpenGL		
	glw.makeCurrent();	
    glGenBuffers(3, io_buf);	
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0,0, displayheight, displaywidth);
    glClearColor(0, 0, 0, 0);
    glHint(GL_POLYGON_SMOOTH_HINT,GL_NICEST);
    glEnable(GL_TEXTURE_2D);
 	glEnable(GL_BLEND);
 	
	unsigned int retraceCount = glXGetVideoSyncSGI(&retraceCount);

	compileFragmentShaders();		
    glUseProgramObjectARB(programs[currentShader]);
    	
	while (doRendering) {

		//update shadow variables
		glw.lockMutex();
		
			//values
			displaywidth = m_displaywidth;
			displayheight = m_displayheight;
			frameRepeats = m_frameRepeats;
												
			doResize = m_doResize;				
			m_doResize = false;
						
			//boolean flags
			doRendering = m_doRendering;
			
			//check for changed aspect ratio lock
			if(aspectLock != m_aspectLock) {
				aspectLock = m_aspectLock;
				doResize = true;
			}
			
		glw.unlockMutex();

		//read frame data
		videoData = glw.vr.getNextFrame();

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


		if(createGLTextures) {
			if(DEBUG) printf("Creating GL textures\n");
			//create the textures on the GPU
			createTextures(videoData, currentShader);
			createGLTextures = false;
		}			
		
		if(updateShaderVars) {
			if(DEBUG) printf("Updating fragment shader variables\n");
			
		    int i=glGetUniformLocationARB(programs[currentShader], "Yheight");
    		glUniform1fARB(i, videoData->Yheight);

		    i=glGetUniformLocationARB(programs[currentShader], "Ywidth");
    		glUniform1fARB(i, videoData->Ywidth);

		    i=glGetUniformLocationARB(programs[currentShader], "CHsubsample");
    		glUniform1fARB(i, (float)(videoData->Ywidth / videoData->Cwidth));

		    i=glGetUniformLocationARB(programs[currentShader], "CVsubsample");
    		glUniform1fARB(i, (float)(videoData->Yheight / videoData->Cheight));

		    i=glGetUniformLocationARB(programs[currentShader], "colorMatrix");
		    float matrix[9] = {1.0, 0.0, 1.5958, 1.0, -0.39173, -0.8129, 1.0, 2.017, 0.0};	    
    		glUniformMatrix3fvARB(i, 1, false, &matrix[0]);

		    i=glGetUniformLocationARB(programs[currentShader], "yuvOffset");
    		glUniform3fARB(i, -0.0625, -0.5, -0.5);
			
			updateShaderVars = false;	
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

     	glXWaitVideoSyncSGI(frameRepeats, 0, &retraceCount);
																			
		if(videoData) {
			
			//if(DEBUG) printf("Rendering...\n");
						
			glUseProgramObjectARB(programs[currentShader]);						
			renderVideo(videoData);
			glUseProgramObjectARB(0);			
							
			if(m_osd) renderOSD(videoData, font);
																								   			   																									   			  
			glFlush();
			glw.swapBuffers();
		}
		
		//get the current frame count
       	glXGetVideoSyncSGI(&retraceCount);
       				
  		//calculate FPS
       	timeval now, diff;
       	gettimeofday(&now, NULL);
       	timersub(&now, &last, &diff);       	
       	if(DEBUG)printf("Display frame %d, FPS = %f\n", retraceCount, 1000000.0 / diff.tv_usec);
       	
       	last.tv_sec = now.tv_sec;
       	last.tv_usec = now.tv_usec;       	
       	       	       	       	
	}
}
