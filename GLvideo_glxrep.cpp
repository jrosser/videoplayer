#include <cassert>

#include "GLfuncs.h"
#include "GLvideo_glxrep.h"

#include "videoData.h"

namespace GLVideoRepeater {
GlxRepeater::~GlxRepeater()
{
}

void GlxRepeater::createRGBDestination(VideoData *)
{
}

void GlxRepeater::setupRGBDestination()
{
}

void GlxRepeater::setupScreenDestination()
{
}

void GlxRepeater::renderVideo(VideoData *)
{
}

unsigned int GlxRepeater::repeat(unsigned int n)
{
	unsigned int retraceCount;
	GLfuncs::glXWaitVideoSyncSGI(n, 0, &retraceCount);	
	return n;
}

} // namespace
