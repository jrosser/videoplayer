/*
 * Very simple example of how to perform YUV->RGB (YCrCb->RGB)
 * conversion with an OpenGL fragmen shader. The data (not included)
 * is presumed to be three files with Y, U and V samples for a 720x576
 * pixels large image.
 *
 * Note! The example uses NVidia extensions for rectangular textures
 * to make it simpler to read (non-normalised coordinates).
 * Rewriting it without the extensions is quite simple, but left as an
 * exercise to the reader. (The trick is that the shader must know the
 * image dimensions instead)
 *
 * The program also does not check to see if the shader extensions are
 * available - this is after all just a simple example.
 *
 * This code is released under a BSD style license. Do what you want, but
 * do not blame me.
 *
 * Peter Bengtsson, Dec 2004.
 */


#define NO_SDL_GLEXT
#define GL_GLEXT_PROTOTYPES
#include <SDL/SDL_opengl.h>
#include <SDL/SDL.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#define uglGetProcAddress(x) (*glXGetProcAddressARB)((const GLubyte*)(x))

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

int Quit=0;
bool shading_enabled = false;

static int width=1920;
static int height=1080;

int main(int argc,char *argv[])
{
SDL_Surface *Win=NULL;
GLubyte *Ytex,*Utex,*Vtex;
SDL_Event evt;
int i;
GLhandleARB FSHandle,PHandle;
char *s;
FILE *fp;

char *FProgram=
  "uniform samplerRect Ytex;\n"
  "uniform samplerRect Utex,Vtex;\n"
  "void main(void) {\n"
  "  float nx,ny,r,g,b,y,u,v;\n"
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

  "  gl_FragColor=vec4(r,g,b,1.0);\n"
  "}\n";


if(!SDL_Init(SDL_INIT_VIDEO)) {

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);

  Win=SDL_SetVideoMode(width,height,32,SDL_HWSURFACE|SDL_ANYFORMAT|SDL_OPENGL);

  if(Win) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,width,0,height,-1,1);
    glViewport(0,0,width,height);
    glClearColor(0,0,0,0);
    glColor3f(1.0,0.84,0.0);
    glHint(GL_POLYGON_SMOOTH_HINT,GL_NICEST);

    /* Set up program objects. */
    PHandle=glCreateProgramObjectARB();
    FSHandle=glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

    /* Compile the shader. */
    glShaderSourceARB(FSHandle,1,(const GLcharARB**)&FProgram,NULL);
    glCompileShaderARB(FSHandle);

    /* Print the compilation log. */
    glGetObjectParameterivARB(FSHandle,GL_OBJECT_COMPILE_STATUS_ARB,&i);
    s=(char *)malloc(32768);
    glGetInfoLogARB(FSHandle,32768,NULL,s);
    printf("Compile Log: %s\n", s);
    free(s);

    /* Create a complete program object. */
    glAttachObjectARB(PHandle,FSHandle);
    glLinkProgramARB(PHandle);

    /* And print the link log. */
    s=(char *)malloc(32768);
    glGetInfoLogARB(PHandle,32768,NULL,s);
    printf("Link Log: %s\n", s);
    free(s);

    /* Finally, use the program. */
    glUseProgramObjectARB(PHandle);

    /* Load the textures. */
    Ytex=(GLubyte *)malloc(width*height);
    Utex=(GLubyte *)malloc(width*height/4);
    Vtex=(GLubyte *)malloc(width*height/4);
   
   /* This might not be required, but should not hurt. */
    glEnable(GL_TEXTURE_2D);
    
    fp=fopen(argv[1],"rb");
 
    /* Select texture unit 1 as the active unit and bind the U texture. */
    glActiveTexture(GL_TEXTURE1);
    i=glGetUniformLocationARB(PHandle,"Utex");
    glUniform1iARB(i,1);  /* Bind Utex to texture unit 1 */
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,1);

    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV,0,GL_LUMINANCE,960,540,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,Utex);

    /* Select texture unit 2 as the active unit and bind the V texture. */
    glActiveTexture(GL_TEXTURE2);
    i=glGetUniformLocationARB(PHandle,"Vtex");
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,2);
    glUniform1iARB(i,2);  /* Bind Vtext to texture unit 2 */

    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV,0,GL_LUMINANCE,960,540,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,Vtex);

    /* Select texture unit 0 as the active unit and bind the Y texture. */
    glActiveTexture(GL_TEXTURE0);
    i=glGetUniformLocationARB(PHandle,"Ytex");
    glUniform1iARB(i,0);  /* Bind Ytex to texture unit 0 */
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,3);

    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV,0,GL_LUMINANCE,1920,1080,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,Ytex);

    /* Simple loop, just draws the image and waits for quit. */
    while(!Quit) {
      if(SDL_PollEvent(&evt)) {
        switch(evt.type) {
        case  SDL_KEYDOWN:
        case  SDL_QUIT:
          Quit=1;
        break;
        }
      }
    
      glClear(GL_COLOR_BUFFER_BIT);

      /* Draw image (again and again). */

      glBegin(GL_QUADS);
        glTexCoord2i(0,0);
        glVertex2i(0,0);
        glTexCoord2i(width,0);
        glVertex2i(width,0);
        glTexCoord2i(width,height);
        glVertex2i(width,height);
        glTexCoord2i(0,height);
        glVertex2i(0,height);
      glEnd();

      /* Flip buffers. */
      fread(Ytex,width*height,1,fp);
      fread(Utex,width*height/4,1,fp);
      fread(Vtex,width*height/4,1,fp);

      glFlush();
      SDL_GL_SwapBuffers();
      
      timeval tv;
      gettimeofday(&tv, NULL);
		printf("%d.%d\n", tv.tv_sec, tv.tv_usec);
		
    /* Select texture unit 1 as the active unit and bind the U texture. */
    glActiveTexture(GL_TEXTURE1);
    i=glGetUniformLocationARB(PHandle,"Utex");
    glUniform1iARB(i,1);  /* Bind Utex to texture unit 1 */
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,1);

    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV,0,GL_LUMINANCE,960,540,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,Utex);

    /* Select texture unit 2 as the active unit and bind the V texture. */
    glActiveTexture(GL_TEXTURE2);
    i=glGetUniformLocationARB(PHandle,"Vtex");
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,2);
    glUniform1iARB(i,2);  /* Bind Vtext to texture unit 2 */

    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV,0,GL_LUMINANCE,960,540,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,Vtex);

    /* Select texture unit 0 as the active unit and bind the Y texture. */
    glActiveTexture(GL_TEXTURE0);
    i=glGetUniformLocationARB(PHandle,"Ytex");
    glUniform1iARB(i,0);  /* Bind Ytex to texture unit 0 */
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,3);

    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV,0,GL_LUMINANCE,1920,1080,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,Ytex);

		
      //sleep(1);
    } /* while(!Quit) */

    /* Clean up before exit. */

    glUseProgramObjectARB(0);
    //glDeleteObjectARB(sprog);
    
    fclose(fp);
    free(Ytex);
    free(Utex);
    free(Vtex);

  } else {
    fprintf(stderr,"Unable to create primary surface. \"%s\".\n",SDL_GetError());
  }
  SDL_Quit();
} else {
  fprintf(stderr,"Initialisation failed. \"%s\".\n",SDL_GetError());
}

return(0);
}



