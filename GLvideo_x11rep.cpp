#define __GLVIDEO_X11REP

#include <cassert>

#include "GLvideo_repeater.h"
#include <GL/glxew.h>

#include "videoData.h"

/* NB, any changes to the public interface
 * /must/ be reflected in GLvideo_x11rep.h */
namespace GLVideoRepeater
{
class X11Repeater : public GLVideoRepeater {
public:
	virtual void createRGBDestination(VideoData *video_data);
	virtual void setupRGBDestination();
	virtual void renderVideo(VideoData *video_data);
	virtual unsigned int repeat(unsigned int);
	virtual void setupScreenDestination();
	virtual ~X11Repeater();
};

X11Repeater::~X11Repeater()
{
}

void X11Repeater::createRGBDestination(VideoData *)
{
}

void X11Repeater::setupRGBDestination()
{
}

void X11Repeater::setupScreenDestination()
{
}

void X11Repeater::renderVideo(VideoData *)
{
}

unsigned int X11Repeater::repeat(unsigned int n)
{
	unsigned int retraceCount;
	glXWaitVideoSyncSGI(n, 0, &retraceCount);
	return n;
}

} // namespace
