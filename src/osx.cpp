#include <Carbon/Carbon.h>
#include <AGL/agl.h>
#include <OpenGL/OpenGL.h>

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
