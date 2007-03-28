#include "GLvideo_rt.h"
#include "GLvideo_mt.h"
#include "GL/glx.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define DEBUG 0

static char *FProgram=
  "uniform samplerRect Ytex;\n"
  "uniform samplerRect Utex,Vtex;\n"
  "void main(void) {\n"
  "  float nx,ny,r,g,b,a,y,u,v;\n"
  "  vec4 txl,ux,vx;"
  "  nx=gl_TexCoord[0].x;\n"
  "  ny=1080.0-gl_TexCoord[0].y;\n"
  "  y=textureRect(Ytex,vec2(nx,ny)).r;\n"
  "  u=textureRect(Utex,vec2(nx/2.0,ny/2.0)).r;\n"
  "  v=textureRect(Vtex,vec2(nx/2.0,ny/2.0)).r;\n"

  "  y=1.1643*(y-0.0625);\n"
  "  u=u-0.5;\n"
  "  v=v-0.5;\n"

  "  r=y+1.5958*v;\n"
  "  g=y-0.39173*u-0.81290*v;\n"
  "  b=y+2.017*u;\n"
  "	 a=1.0;\n"
  
//  " if(y > 235.0/255.0) {\n"
//  "    r=1.0;\n"
//  "    g=0.0;\n"
//  "    b=0.0;\n"
//  "    r=1.0 - 1.0 * ((20.0/255.0)/(y-(235.0/255.0)));\n"  
//  "  }\n"

//  " if(y < 16.0/255.0) {\n"
//  "    r=0.0;\n"
//  "    g=0.0;\n"
//  "    b=1.0;\n"
// "    b=1.0 - 1.0 * (16.0/y);\n"  
//  "  }\n"
      
  "  gl_FragColor=vec4(r,g,b,a);\n"
  "}\n";

GLvideo_rt::GLvideo_rt(GLvideo_mt &gl) 
      : QThread(), glw(gl)
{
	m_srcwidth = 1920; 
	m_srcheight = 1080;
	m_frameRepeats = 2;
	m_allocateTextures = true;	
	m_createGLTextures = true;
	m_doRendering = true;
	m_doResize = false;
	m_openFile = false;
	m_aspectLock = true;
}

void GLvideo_rt::compileFragmentShader()
{
	GLint length;			//log length
	
    shader=glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

	printf("Shader program is %s\n", FProgram);

    glShaderSourceARB(shader ,1,(const GLcharARB**)&FProgram,NULL);    
	glCompileShaderARB(shader);
    
    glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);
    glGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length); 
    printf("Compile status is %d, log length is %d\n", compiled, length);   
    char *s=(char *)malloc(length);
    glGetInfoLogARB(shader, length, &length ,s);
    printf("Compile Log: (%d) %s\n", length, s);
    free(s);

   	/* Set up program objects. */
    program=glCreateProgramObjectARB();
    glAttachObjectARB(program, shader);
    glLinkProgramARB(program);

    /* And print the link log. */
    if(compiled) {
    	glGetObjectParameterivARB(program, GL_OBJECT_LINK_STATUS_ARB, &linked);
    	glGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
    	printf("Link status is %d, log length is %d\n", linked, length);            
    	s=(char *)malloc(length);
    	glGetInfoLogARB(shader, length, &length, s);
    	printf("Link Log: (%d) %s\n", length, s);
    	free(s);
    }

    /* Finally, use the program. */
    glUseProgramObjectARB(program);
}

void GLvideo_rt::createTextures(GLubyte *Ytex, GLubyte *Utex, GLubyte *Vtex)
{
    /* Select texture unit 1 as the active unit and bind the U texture. */
    glActiveTexture(GL_TEXTURE1);
    int i=glGetUniformLocationARB(program, "Utex");
    glUniform1iARB(i,1);  /* Bind Utex to texture unit 1 */
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,1);

    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV,0,GL_LUMINANCE,960,540,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,Utex);

    /* Select texture unit 2 as the active unit and bind the V texture. */
    glActiveTexture(GL_TEXTURE2);
    i=glGetUniformLocationARB(program, "Vtex");
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,2);
    glUniform1iARB(i,2);  /* Bind Vtext to texture unit 2 */

    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV,0,GL_LUMINANCE,960,540,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,Vtex);

    /* Select texture unit 0 as the active unit and bind the Y texture. */
    glActiveTexture(GL_TEXTURE0);
    i=glGetUniformLocationARB(program, "Ytex");
    glUniform1iARB(i,0);  /* Bind Ytex to texture unit 0 */
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,3);

    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV,0,GL_LUMINANCE,1920,1080,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,Ytex);	
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

void GLvideo_rt::setFileName(const QString &fn)
{
	glw.lockMutex();
	m_fileName = fn;
	glw.unlockMutex();
}

void GLvideo_rt::setSourceSize(int width, int height)
{
	glw.lockMutex();
	m_srcwidth = width;
	m_srcheight = height;

	//m_allocateTextures = true;
	//m_createGLTextures = true;
	//m_updateShaderVars = true;
	
	glw.unlockMutex();
}    

void GLvideo_rt::render(GLubyte *Ytex, GLubyte *Utex, GLubyte *Vtex, int srcwidth, int srcheight)
{		
	glw.makeCurrent();

	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, 1);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_NV, 0, 0, 0, 960, 540, GL_LUMINANCE, GL_UNSIGNED_BYTE, Utex);

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, 2);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_NV, 0, 0, 0, 960, 540, GL_LUMINANCE, GL_UNSIGNED_BYTE, Vtex);	

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, 3);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_NV, 0, 0, 0, 1920, 1080, GL_LUMINANCE, GL_UNSIGNED_BYTE, Ytex);
			
	glBegin(GL_QUADS);
		glTexCoord2i(0,0);
		glVertex2i(0,0);
		glTexCoord2i(srcwidth,0);
		glVertex2i(srcwidth,0);
		glTexCoord2i(srcwidth, srcheight);
		glVertex2i(srcwidth, srcheight);
		glTexCoord2i(0, srcheight);
		glVertex2i(0, srcheight);
	glEnd();				
}
    
void GLvideo_rt::run()
{
	printf("Starting renderthread\n");

	void *textureMemory=NULL;
   	GLubyte *Ytex=NULL, *Utex=NULL, *Vtex=NULL;	//Y,U,V textures
   	int fd = 0;
   	//FILE *fp = NULL;				//file pointer
	int repeats = 0;				//frame repeat counter
	int eof = 0;
	
	//declare shadow variables for the thread worker and initialise them
	glw.lockMutex();
	QString fileName = m_fileName;
	int displaywidth = m_displaywidth;
	int displayheight = m_displayheight;
	int srcwidth = m_srcwidth; 
	int srcheight = m_srcheight;
	int frameRepeats = m_frameRepeats;
	bool allocateTextures = m_allocateTextures;	
	bool createGLTextures = m_createGLTextures;
	bool doRendering = m_doRendering;
	bool doResize = m_doResize;
	bool openFile = m_openFile;
	bool updateShaderVars = m_updateShaderVars;
	bool compileShader = m_compileShader;
	bool aspectLock = m_aspectLock;
	glw.unlockMutex();

	//initialise OpenGL					
	glw.makeCurrent();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, srcwidth ,0 , srcheight, -1 ,1);
    glViewport(0,0, displayheight, displaywidth);
    glClearColor(0, 0, 0, 0);
    glColor3f(0.0, 0.0, 0.0);
    glHint(GL_POLYGON_SMOOTH_HINT,GL_NICEST);
    glEnable(GL_TEXTURE_2D);
    
	compileFragmentShader();		
	
	
	while (doRendering) {

		//update shadow variables
		glw.lockMutex();
		
			//values
			displaywidth = m_displaywidth;
			displayheight = m_displayheight;
			srcwidth = m_srcwidth; 
			srcheight = m_srcheight;
			frameRepeats = m_frameRepeats;
			
			
			//'one-shot' instructions from another thread
			allocateTextures = m_allocateTextures;
			m_allocateTextures = false;
				
			createGLTextures = m_createGLTextures;
			m_createGLTextures = false;
			
			openFile = m_openFile;
			m_openFile = false;
			
			updateShaderVars = m_updateShaderVars;
			m_updateShaderVars = false;
			
			compileShader = m_compileShader;
			m_compileShader = false;
			
			doResize = m_doResize;				
			m_doResize = false;
						
			//boolean flags
			doRendering = m_doRendering;

		glw.unlockMutex();

		if(compileShader) {
			printf("Compiling fragment shader\n");
			compileShader = false;	
		}
		
		if(updateShaderVars) {
			printf("Updating fragment shader variables\n");
			updateShaderVars = false;	
		}	
					
		if(allocateTextures) {
			printf("Allocating textures\n");
			//allocate or change the allocation of the memory that the textures are read into	
			if(Ytex) free(Ytex);
			if(Ytex) free(Utex);
			if(Ytex) free(Vtex);			
			
			//this looks really ugly, but we want aligned memory for O_DIRECT,
			//and for 4:2:0 video the chroma may not be a multiple of 512 bytes as required for O_DIRECT
			//the whole frame is read at once into Ytex, then Utex and Vtex point correctly to the planar chroma
			posix_memalign(&textureMemory, 4096, srcwidth*srcheight*3);	//enough for 4:4:4		
			Ytex = (GLubyte*)textureMemory;			
			Utex = Ytex + srcwidth*srcheight;			
			Vtex = Utex + srcwidth*srcheight/4;
					    		
    		allocateTextures = false;			
		}
		
		if(createGLTextures) {
			printf("Creating GL textures\n");
			//create the textures on the GPU
			createTextures(Ytex, Utex, Vtex);
			createGLTextures = false;
		}			
					
		if(doResize){
			//resize the viewport
			printf("Resizing to %d, %d\n", displaywidth, displayheight);
			glw.makeCurrent();
			
			float sourceAspect = (float)srcwidth / (float)srcheight;
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
		
		//open a new file, close the old one
		if(openFile || eof) {								
			if(fd) close(fd);
			fd = 0;
			eof = 0;	
		}
		
		//open input
		if(fd == 0) {						
			fd = open(fileName.toLatin1().data(), O_RDONLY | O_DIRECT);					
			repeats = 0;
		}
								
		//read frame data
		repeats++;	
		if(repeats == frameRepeats) {
			if(DEBUG) printf("Reading picture data\n");

			int ret;
			
			ret = read(fd, Ytex, srcwidth*srcheight*3/2);			
			
			if(ret == 0) 
				eof = 1;
			else 
				eof = 0;
						
			repeats = 0;			
		}
       	
		//wait for VSYNC - must have __GL_SYNC_TO_VBLANK=1 environment variable set....	
		//render       		
		if(DEBUG) printf("Rendering...\n");			
		render(Ytex, Utex, Vtex, srcwidth, srcheight);
		glw.swapBuffers();       	
		glFlush();
		
	}
}
