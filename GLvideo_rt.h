#ifndef GLVIDEO_RT_H_
#define GLVIDEO_RT_H_

#define GL_GLEXT_PROTOTYPES 1

#include <QtGui>
#include <QGLWidget>

class GLvideo_mt;
class FTFont;
class VideoData;

class GLvideo_rt : public QThread
{
public:

	enum ShaderPrograms { shaderPlanar, shaderUYVY, shaderV216, shaderYV16, shaderV210, shaderMax };

	GLvideo_rt(GLvideo_mt &glWidget);
	void resizeViewport(int w, int h);
	void setFrameRepeats(int r);
	void setAspectLock(bool lock);
	void run();
	void stop();
        
private:
	void setUpFonts(const char* fontfile);
	void compileFragmentShaders();
	void compileFragmentShader(int n, const char *src);
	void createTextures(VideoData *videoData, int currentShader);
	void render(GLubyte *Ytex, GLubyte *Utex, GLubyte *Vtex, int Ywidth, int Yheight, int Cwidth, int Cheight, bool ChromaTextures);

	bool m_doRendering;			//set to false to quit thread
	bool m_doResize;			//resize the openGL viewport
	bool m_aspectLock;			//lock the aspect ratio of the source video
			
	int m_displaywidth;			//width of parent widget
	int m_displayheight;		//height of parent widget
	
	int m_frameRepeats;			//number of times each frame is repeated
	
	GLuint programs[shaderMax];	//handles for the shaders and programs	
	GLuint shaders[shaderMax];
	
   	GLint compiled[shaderMax];
   	GLint linked[shaderMax];	//flags for success
   	
   	GLuint io_buf[3];			//io buffer objects
   	
	GLvideo_mt &glw;			//parent widget
};

#endif /*GLVIDEO_RT_H_*/
