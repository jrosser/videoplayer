#include <Carbon/Carbon.h>
#include <OpenGL/OpenGL.h>

void osxSwapInterval(int interval)
{
	CGLContextObj cgl_ctx = CGLGetCurrentContext();
	CGLSetParameter(cgl_ctx, kCGLCPSwapInterval, &interval);
}
