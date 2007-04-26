#ifndef GLVIDEO_RT_H_
#define GLVIDEO_RT_H_

#include <QtGui>
#include <QGLWidget>

class GLvideo_mt;
class FTFont;
class VideoData;

class GLvideo_rt : public QThread
{
public:

	enum ShaderPrograms { shaderPlanar, shaderUYVY, shaderV216, shaderMax };

	GLvideo_rt(GLvideo_mt &glWidget);
	void resizeViewport(int w, int h);
	void setFrameRepeats(int repeats);
	void setFramePolarity(int polarity);
	void setFontFile(const char *fontFile);	
	void setAspectLock(bool lock);
	void run();
	void stop();
	void toggleAspectLock();
	void toggleOSD();
		        
private:
	//void setUpFonts(const char* fontfile);
	void compileFragmentShaders();
	void compileFragmentShader(int n, const char *src);
	void createTextures(VideoData *videoData, int currentShader);
	void renderVideo(VideoData *videoData);
#ifdef HAVE_FTGL
	void renderOSD(VideoData *videoData, FTFont *font);
#endif	

	bool m_doRendering;			//set to false to quit thread
	bool m_doResize;			//resize the openGL viewport
	bool m_aspectLock;			//lock the aspect ratio of the source video
	bool m_osd;					//is the OSD shown?
	bool m_changeFont;			//set a new font for the OSD
				
	int m_displaywidth;			//width of parent widget
	int m_displayheight;		//height of parent widget
	
	int m_frameRepeats;			//number of times each frame is repeated
	int m_framePolarity;		//which frame in m_frameRepeats to update on
	
	GLuint programs[shaderMax];	//handles for the shaders and programs	
	GLuint shaders[shaderMax];	
   	GLint compiled[shaderMax];
   	GLint linked[shaderMax];	//flags for success   	
   	GLuint io_buf[3];			//io buffer objects   	
	GLvideo_mt &glw;			//parent widget
	
	char fontFile[255];

	QMutex mutex;	
};

#endif /*GLVIDEO_RT_H_*/
