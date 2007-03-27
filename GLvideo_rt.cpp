#include "GLvideo_rt.h"
#include "GL/glx.h"

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

GLvideo_rt::GLvideo_rt(QGLWidget &gl) 
      : QThread(), glw(gl)
{
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

void GLvideo_rt::createTextures()
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
	doRendering = false;
}
    
void GLvideo_rt::resizeViewport(int width, int height)
{
	displaywidth = width;
	displayheight = height;
	doResize = true;
}    

void GLvideo_rt::setFileName(const QString &fn)
{
	fileName = fn;
}

void GLvideo_rt::setSourceSize(int width, int height)
{
	srcwidth = width;
	srcheight = height;

	//reallocate the texture buffers in memory
	
	//reallocate the texture buffers on the video board
	
	//send the size differences to the fragment shader program
	
}    

void GLvideo_rt::render()
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
		
	displaywidth = glw.width();
	displayheight = glw.height();
	srcwidth = 1920;
	srcheight = 1080;
	frameRepeats = 2;
	
	fp = NULL;
		
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
	
	allocateTextures = true;	
	createGLTextures = true;
	doRendering = true;
	doResize = false;
	openFile = false;
	
	while (doRendering) {
				
		if(allocateTextures) {
			//allocate or change the allocation of the memory that the textures are read into	
			if(Ytex) free(Ytex);
			if(Utex) free(Utex);
			if(Vtex) free(Vtex);
									
    		Ytex=(GLubyte *)malloc(srcwidth*srcheight);
    		Utex=(GLubyte *)malloc(srcwidth*srcheight/4);
    		Vtex=(GLubyte *)malloc(srcwidth*srcheight/4);
    		
    		allocateTextures = false;			
		}
		
		if(createGLTextures) {
			//create the textures on the GPU
			createTextures();
			createGLTextures = false;
		}			
					
		if(doResize){
			//resize the viewport
			printf("Resizing to %d, %d\n", displaywidth, displayheight);			
			glViewport(0, 0, displaywidth, displayheight);
			doResize = false;
		}

		//loop at EOF
		if(fp != NULL) {
			if(feof(fp)) {
				printf("End of file...\n");		
				fclose(fp);
				fp = NULL;
			}
		}
		
		//open a new file, close the old one and set file pointer to NULL
		if(openFile) {	
			if(fp) fclose(fp);
			fp = NULL;
			openFile = false;	
		}
		
		//open input
		if(fp == NULL) {
			fp = fopen(fileName.toLatin1().data(), "rb");		
			//fp = fopen("/video/crocs_5851_19.86Mbps_9.0.yuv", "rb");
			printf("Opening input file, fp=%p\n", fp);		
			repeats = 0;
		}
			
		//oh no - could not open input file
		if(fp == NULL)	
			printf("Could not open input video\n");
	
		//read frame data
		repeats++;	
		if(repeats == frameRepeats) {
			fread(Ytex, 1, srcwidth*srcheight,fp);
			fread(Utex, 1, srcwidth*srcheight/4,fp);
			fread(Vtex, 1, srcwidth*srcheight/4,fp);
			repeats = 0;		
		}
       		
		//render       		
		printf("Rendering...\n");			
		render();
       			
		//wait for VSYNC - must have __GL_SYNC_TO_VBLANK=1 environment variable set....
		glw.swapBuffers();
		glFlush();
	}
}
