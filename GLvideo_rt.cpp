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

//shader program for planar video formats
//should work with all chroma subsamplings
static char *FProgram=
  "uniform samplerRect Ytex;\n"
  "uniform samplerRect Utex,Vtex;\n"
  "uniform float Yheight, CHsubsample, CVsubsample;\n"
  "uniform mat3 colorMatrix;\n"
  "uniform vec3 yuvOffset;\n"
        
  "void main(void) {\n"
  "  float nx,ny,a;\n"
  "  vec3 yuv;\n"
  "  vec3 rgb;\n"
  
  "  nx=gl_TexCoord[0].x;\n"
  "  ny=Yheight-gl_TexCoord[0].y;\n"
  
  "  yuv[0]=textureRect(Ytex,vec2(nx,ny)).r;\n"
  "  yuv[1]=textureRect(Utex,vec2(nx/CHsubsample, ny/CVsubsample)).r;\n"
  "  yuv[2]=textureRect(Vtex,vec2(nx/CHsubsample, ny/CVsubsample)).r;\n"

  "  yuv = yuv + yuvOffset;\n"  
  "  rgb = yuv * colorMatrix;\n"

  "	 a=1.0;\n"
      
  "  gl_FragColor=vec4(rgb ,a);\n"
  "}\n";

GLvideo_rt::GLvideo_rt(GLvideo_mt &gl) 
      : QThread(), glw(gl)
{	
	m_frameRepeats = 2;	
	m_doRendering = true;
	m_aspectLock = true;		
}

void GLvideo_rt::compileFragmentShader()
{
	GLint length;			//log length
	
    shader=glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

	if(DEBUG) printf("Shader program is %s\n", FProgram);

    glShaderSourceARB(shader ,1,(const GLcharARB**)&FProgram,NULL);    
	glCompileShaderARB(shader);
    
    glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);
    glGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);    
    char *s=(char *)malloc(length);
    glGetInfoLogARB(shader, length, &length ,s);
    if(DEBUG)printf("Compile Status %d, Log: (%d) %s\n", compiled, length, s);
    free(s);

   	/* Set up program objects. */
    program=glCreateProgramObjectARB();
    glAttachObjectARB(program, shader);
    glLinkProgramARB(program);

    /* And print the link log. */
    if(compiled) {
    	glGetObjectParameterivARB(program, GL_OBJECT_LINK_STATUS_ARB, &linked);
    	glGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);            
    	s=(char *)malloc(length);
    	glGetInfoLogARB(shader, length, &length, s);
    	if(DEBUG) printf("Link Status %d, Log: (%d) %s\n", linked, length, s);
    	free(s);
    }

    /* Finally, use the program. */
    glUseProgramObjectARB(program);
}

void GLvideo_rt::createTextures(int Ywidth, int Yheight, int Cwidth, int Cheight)
{
    /* Select texture unit 1 as the active unit and bind the U texture. */
    glActiveTexture(GL_TEXTURE1);
    int i=glGetUniformLocationARB(program, "Utex");
    glUniform1iARB(i,1);  /* Bind Utex to texture unit 1 */
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,1);

    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    //glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
	glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_LUMINANCE, Cwidth , Cheight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
    
    /* Select texture unit 2 as the active unit and bind the V texture. */
    glActiveTexture(GL_TEXTURE2);
    i=glGetUniformLocationARB(program, "Vtex");
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,2);
    glUniform1iARB(i,2);  /* Bind Vtext to texture unit 2 */

    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    //glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
	glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_LUMINANCE, Cwidth , Cheight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

    /* Select texture unit 0 as the active unit and bind the Y texture. */
    glActiveTexture(GL_TEXTURE0);
    i=glGetUniformLocationARB(program, "Ytex");
    glUniform1iARB(i,0);  /* Bind Ytex to texture unit 0 */
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,3);

    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    //glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
	glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_LUMINANCE, Ywidth , Yheight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

	//create buffer objects that are used to transfer the texture data to the card
	//this is much faster than using glTexSubImage2D() later on to replace the texture data
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, io_buf[0]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, Cwidth * Cheight, NULL, GL_STREAM_DRAW);    

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, io_buf[1]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, Cheight * Cwidth, NULL, GL_STREAM_DRAW);    
    
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, io_buf[2]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, Yheight * Ywidth, NULL, GL_STREAM_DRAW);    
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
    
void GLvideo_rt::resizeViewport(int width, int height)
{
	glw.lockMutex();
	m_displaywidth = width;
	m_displayheight = height;
	m_doResize = true;
	glw.unlockMutex();
}    

void GLvideo_rt::render(GLubyte *Ytex, GLubyte *Utex, GLubyte *Vtex, int Ywidth, int Yheight, int Cwidth, int Cheight, bool ChromaTextures)
{		
	glClear(GL_COLOR_BUFFER_BIT);

	void *ioMem;

	if(ChromaTextures) {
		
		glBindTexture(GL_TEXTURE_RECTANGLE_NV, 1);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, io_buf[0]);
		ioMem = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
		memcpy(ioMem, Utex, Cwidth * Cheight);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, Cwidth, Cheight, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

		glBindTexture(GL_TEXTURE_RECTANGLE_NV, 2);	
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, io_buf[1]);
		ioMem = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
		memcpy(ioMem, Vtex, Cwidth * Cheight);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, Cwidth, Cheight, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);	
	}
	
	//luminance (or muxed YCbCr) data
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, 3);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, io_buf[2]);
	ioMem = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
	memcpy(ioMem, Ytex, Ywidth * Yheight);
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, Ywidth, Yheight, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	
	glBegin(GL_QUADS);
		glTexCoord2i(0,0);
		glVertex2i(0,0);
		glTexCoord2i(Ywidth,0);
		glVertex2i(Ywidth,0);
		glTexCoord2i(Ywidth, Yheight);
		glVertex2i(Ywidth, Yheight);
		glTexCoord2i(0, Yheight);
		glVertex2i(0, Yheight);
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
	glw.unlockMutex();

	//font rendering
  	FTGLPolygonFont font((const char *)"/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans-Bold.ttf");    
    if(font.Error()) {
    	printf("Failed to open font file\n");
    	fflush(stdout);
    }

	font.FaceSize(144);
	font.CharMap(ft_encoding_unicode);
        	
	//initialise OpenGL		
	glw.makeCurrent();			
    glGenBuffers(3, io_buf);	
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0,0, displayheight, displaywidth);
    glClearColor(0, 0, 0, 0);
    glHint(GL_POLYGON_SMOOTH_HINT,GL_NICEST);
    glEnable(GL_TEXTURE_2D);

	compileFragmentShader();		
	
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

		//check for the video format changing - TODO....

		if(createGLTextures) {
			if(DEBUG) printf("Creating GL textures\n");
			//create the textures on the GPU
			createTextures(videoData->Ywidth, videoData->Yheight, videoData->Cwidth, videoData->Cheight);
			createGLTextures = false;
		}			
		
		if(updateShaderVars) {
			if(DEBUG) printf("Updating fragment shader variables\n");
			
		    int i=glGetUniformLocationARB(program, "Yheight");
    		glUniform1fARB(i, videoData->Yheight);

		    i=glGetUniformLocationARB(program, "CHsubsample");
    		glUniform1fARB(i, (float)(videoData->Ywidth / videoData->Cwidth));

		    i=glGetUniformLocationARB(program, "CVsubsample");
    		glUniform1fARB(i, (float)(videoData->Yheight / videoData->Cheight));

		    i=glGetUniformLocationARB(program, "colorMatrix");
		    float matrix[9] = {1.0, 0.0, 1.5958, 1.0, -0.39173, -0.8129, 1.0, 2.017, 0.0};	    
    		glUniformMatrix3fvARB(i, 1, false, &matrix[0]);

		    i=glGetUniformLocationARB(program, "yuvOffset");
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
					glViewport(offset,0, width, displayheight);
				}
				else {
					int height = (int)(displaywidth/sourceAspect);
					int offset = (displayheight - height) / 2;								
					glViewport(0, offset, displaywidth, height);
				}
			}			
			doResize = false;
		}
	
		//wait for some number of extra vertical syncs
		unsigned int retraceCount;		
       	glXGetVideoSyncSGI(&retraceCount);
       	glXWaitVideoSyncSGI(1, (retraceCount+1)%1, &retraceCount);       				
																
		if(videoData) {
			
			if(DEBUG) printf("Rendering...\n");
						
			render((GLubyte *)videoData->Ydata, 
				   (GLubyte *)videoData->Udata, 
				   (GLubyte *)videoData->Vdata,
					videoData->Ywidth, videoData->Yheight, videoData->Cwidth, videoData->Cheight,
					videoData->isPlanar);
			
			glColor3f(1.0, 1.0, 1.0);			
									
			glPushMatrix();			
   			font.Render( "Hello World!");
			glPopMatrix();									
										   			   
			glFlush();
			glw.swapBuffers();
		}

  		//calculate FPS
       	timeval now, diff;
       	gettimeofday(&now, NULL);
       	timersub(&now, &last, &diff);       	
       	if(DEBUG)printf("FPS = %f\n", 1000000.0 / diff.tv_usec);
       	last = now;
       	       	       	       	
	}
}
