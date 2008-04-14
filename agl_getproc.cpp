#include <Carbon/Carbon.h>
#include <AGL/agl.h>
#include <OpenGL.h>

CFBundleRef gBundleRefOpenGL = NULL;

void my_aglEnableMultiThreading(void)
{
	CGLError err = kCGLNoError;
	CGLContextObj ctx = CGLGetCurrentContext();

	// Enable the multi-threading
	err = CGLEnable( ctx, kCGLCEMPEngine);

	if (err != kCGLNoError )
	{
		fprintf(stderr, "Could not enable OSX multithreaded opengl");
		// Multi-threaded execution is possibly not available
		// Insert your code to take appropriate action
	}
}

//nasty ugly code because including agl.h in elsewhere causes compile errors
void my_aglSwapInterval(int interval)
{
	AGLContext cx = aglGetCurrentContext();

	aglSetInteger(cx, AGL_SWAP_INTERVAL, (GLint*)&interval);

	if (interval != 0)
		aglEnable(cx, AGL_SWAP_INTERVAL);
	else
		aglDisable(cx, AGL_SWAP_INTERVAL);
}