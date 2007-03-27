#ifndef GLVIDEO_RT_H_
#define GLVIDEO_RT_H_

#define GL_GLEXT_PROTOTYPES 1

#include <QtGui>
#include <QGLWidget>

class GLvideo_rt : public QThread
{
public:
	GLvideo_rt(QGLWidget &glWidget);
	void resizeViewport(int w, int h);
	void setFrameRepeats(int r);
	void setSourceSize(int w, int h);
	void setFileName(const QString &fileName);
	void run();
	void stop();
        
private:
	void compileFragmentShader();
	void createTextures();
	void render();

	bool doRendering;		//set to false to quit thread
	bool doResize;			//resize the openGL viewport
	bool doOpen;			//open a new file
	bool renderingPause;	//stop rendering whilst we meddle with the fragment shader or texture buffers
	bool allocateTextures;
	bool createGLTextures;
	bool openFile;			//open a new input file
	
	QString fileName;
	
	int displaywidth, displayheight;	//width and height of the viewport
	int srcwidth, srcheight;			//width and height of the source video
	
	int frameRepeats;		//number of times each frame is repeated
	int repeats;			//frame repeat counter
	
	GLuint shader, program;	//handles for the shader and program
   	GLint compiled, linked;	//flags for success
   	
   	GLubyte *Ytex, *Utex, *Vtex;	//Y,U,V textures
		
	FILE *fp;				//file pointer
	QGLWidget &glw;			//parent widget
	QGLContext *glcontext;	//GL context for this thread
};

#endif /*GLVIDEO_RT_H_*/
