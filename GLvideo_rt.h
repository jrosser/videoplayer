#ifndef GLVIDEO_RT_H_
#define GLVIDEO_RT_H_

#define GL_GLEXT_PROTOTYPES 1

#include <QtGui>
#include <QGLWidget>

class GLvideo_mt;

class GLvideo_rt : public QThread
{
public:
	GLvideo_rt(GLvideo_mt &glWidget);
	void resizeViewport(int w, int h);
	void setFrameRepeats(int r);
	void setSourceSize(int w, int h);
	void setAspectLock(bool lock);
	void setFileName(const QString &fileName);
	void run();
	void stop();
        
private:
	void compileFragmentShader();
	void createTextures(GLubyte *Ytex, GLubyte *Utex, GLubyte *Vtex);
	void render(GLubyte *Ytex, GLubyte *Utex, GLubyte *Vtex, int srcwidth, int srcheight);

	bool m_doRendering;			//set to false to quit thread
	bool m_doResize;			//resize the openGL viewport
	bool m_doOpen;				//open a new file
	bool m_renderingPause;		//stop rendering whilst we meddle with the fragment shader or texture buffers
	bool m_allocateTextures;	//allocate, or reallocate computer memory for texture storage
	bool m_createGLTextures;	//create, or re-create storage on the graphics card for texture storage
	bool m_openFile;			//open a new input file
	bool m_updateShaderVars;	//send new parameters to the fragment shader
	bool m_compileShader;		//compile and link the fragment shader
	bool m_aspectLock;			//lock the aspect ratio of the source video
	
	QString m_fileName;			//new input file
		
	int m_displaywidth;			//width of parent widget
	int m_displayheight;		//height of parent widget
	int m_srcwidth;				//width of source video
	int m_srcheight;			//height of the source video
	
	int m_frameRepeats;			//number of times each frame is repeated
	
	GLuint shader, program;		//handles for the shader and program
   	GLint compiled, linked;		//flags for success
   	
	GLvideo_mt &glw;				//parent widget
};

#endif /*GLVIDEO_RT_H_*/
