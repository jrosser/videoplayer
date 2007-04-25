/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: GLvideo.cpp,v 1.3 2007-04-25 12:56:59 jrosser Exp $
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

#include "GLvideo.h"

#ifdef Q_OS_UNIX
#include <sys/time.h>
#include <time.h>
#endif

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
  
  " if(y > 235.0/255.0) {\n"
  "    r=1.0;\n"
  "    g=0.0;\n"
  "    b=0.0;\n"
  "    r=1.0 * ((20.0/255.0)/(y-(235.0/255.0)));\n"  
  "  }\n"

  " if(y < 16.0/255.0) {\n"
  "    r=0.0;\n"
  "    g=0.0;\n"
  "    b=1.0;\n"
  "    b=1.0 * (16.0/y);\n"  
  "  }\n"

   
//  "  if(gl_TexCoord[0].x < 10 || gl_TexCoord[0].y < 10) {\n"
//  "    r=1.0;\n"
//  "    g=0.0;\n"
//  "    b=0.0;\n"
//  "  }\n"
      
  "  gl_FragColor=vec4(r,g,b,a);\n"
  "}\n";

GLvideo::GLvideo(QWidget *parent)
{	
	fp = NULL;
	
	srcwidth  = 1920;
	srcheight = 1080;

    /* Load the textures. */
    Ytex=(GLubyte *)malloc(srcwidth*srcheight);
    Utex=(GLubyte *)malloc(srcwidth*srcheight/4);
    Vtex=(GLubyte *)malloc(srcwidth*srcheight/4);
    
    compiled = false;
}

void GLvideo::compileFragmentShader()
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
    glGetObjectParameterivARB(program, GL_OBJECT_LINK_STATUS_ARB, &linked);
    glGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
    printf("Link status is %d, log length is %d\n", linked, length);            
    s=(char *)malloc(length);
    glGetInfoLogARB(shader, length, &length, s);
    printf("Link Log: (%d) %s\n", length, s);
    free(s);

    /* Finally, use the program. */
    glUseProgramObjectARB(program);
        	
	//set this even if the compiltion failed     	
    shaderCompiled = true;	
}

void GLvideo::initialiseGL()
{  
	// Set up the rendering context, define display lists etc.:
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, srcwidth ,0 ,srcheight ,-1 ,1);
    glViewport(0,0, width(), height());
    glClearColor(0,0,0,0);
    glColor3f(0.0,0.0,0.0);
    glHint(GL_POLYGON_SMOOTH_HINT,GL_NICEST);
    glEnable(GL_TEXTURE_2D);
}

void GLvideo::resizeGL(int w, int h)
{	
	// setup viewport, projection etc.:

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();	
	glOrtho(0, srcwidth, 0, srcheight, -1, 1);
	glViewport(0, 0, w, h);
}

void GLvideo::paintGL()
{
	if(shaderCompiled == FALSE)
		compileFragmentShader();
	
	int i;

	//loop at EOF
	if(fp != NULL) {
		if(feof(fp)) {		
			fclose(fp);
			fp = NULL;
		}
	}
	
	//open input
	if(fp == NULL) {
		//fp = fopen("/video/tc1080.i420", "rb");		
		fp = fopen("/video/battle_hannibal_edit_25772_18.65Mbps_9.3.yuv", "rb");
		framenum=0;
	}
			
	//oh no!
	if(fp == NULL)	
		printf("Could not open input video\n");
	
	framenum++;
	
	if(framenum & 0x01) {
		//read input
		fread(Ytex,srcwidth*srcheight,1,fp);
		fread(Utex,srcwidth*srcheight/4,1,fp);
		fread(Vtex,srcwidth*srcheight/4,1,fp);
	}
	
    /* Select texture unit 1 as the active unit and bind the U texture. */
    glActiveTexture(GL_TEXTURE1);
    i=glGetUniformLocationARB(program, "Utex");
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

	glClear(GL_COLOR_BUFFER_BIT);

	glBegin(GL_QUADS);
		glTexCoord2i(0,0);
        glVertex2i(0,0);
        glTexCoord2i(srcwidth,0);
        glVertex2i(srcwidth,0);
        glTexCoord2i(srcwidth,srcheight);
        glVertex2i(srcwidth,srcheight);
        glTexCoord2i(0,srcheight);
        glVertex2i(0,srcheight);
	glEnd();

	glFlush();
      
	timeval tv;
	timeval diff;
	gettimeofday(&tv, NULL);
	timersub(&tv, &last, &diff);
	printf("Instantaneous fps %f\n", diff.tv_usec, 1000000.0/diff.tv_usec);
	last = tv;
}

