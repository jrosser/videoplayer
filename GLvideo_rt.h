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
	void setInterlacedSource(bool i);	
	void run();
	void stop();
	void toggleAspectLock();
	void toggleOSD();
	void toggleLuminance();
	void toggleChrominance();
	void toggleDeinterlace();	
	void toggleMatrixScaling();
	void setLuminanceOffset1(float o);
	void setChrominanceOffset1(float o);	
	void setLuminanceMultiplier(float m);
	void setChrominanceMultiplier(float m);
	void setLuminanceOffset2(float o);
	void setChrominanceOffset2(float o);
	void setDeinterlace(bool d);
	void setMatrixScaling(bool s);
	void setMatrix(float Kr, float Kg, float Kb);
					        
private:
	//void setUpFonts(const char* fontfile);
	void buildColourMatrix(float *matrix, const float Kr, const float Kg, const float Kb, bool Yscale, bool Cscale);
	void compileFragmentShaders();
	void compileFragmentShader(int n, const char *src);
	void createTextures(VideoData *videoData, int currentShader);
	void uploadTextures(VideoData *videoData);
	void renderVideo(VideoData *videoData);
#ifdef HAVE_FTGL
	void renderOSD(VideoData *videoData, FTFont *font, float fps, int osd);
#endif	

	bool m_doRendering;			//set to false to quit thread
	bool m_doResize;			//resize the openGL viewport
	bool m_aspectLock;			//lock the aspect ratio of the source video
	bool m_changeFont;			//set a new font for the OSD
	int m_osd;					//type of OSD shown
	bool m_showLuminance;		//use Y data or 0.5
	bool m_showChrominance;		//use C data or 0.5
	bool m_interlacedSource;	//is the source video interlaced?
	bool m_deinterlace;			//do we try to deinterlace an interlaced source?
	bool m_matrixScaling;		//is the colour matrix scaled to produce 0-255 computer levels or 16-235 video levels
	bool m_changeMatrix;
	
	float m_luminanceOffset1;		//Y & C offsets applied to data passed to the shader directly from the file
	float m_chrominanceOffset1;
	float m_luminanceMultiplier;	//Y & C scaling factors applied to offset Y/C data
	float m_chrominanceMultiplier;
	float m_luminanceOffset2;		//Y & C offsets applied to scaled data
	float m_chrominanceOffset2;
	float m_matrixKr;				//colour matrix specification
	float m_matrixKg;
	float m_matrixKb;
	
	static const int MAX_OSD = 4;
				
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
