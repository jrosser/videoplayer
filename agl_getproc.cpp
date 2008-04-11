#include <Carbon/Carbon.h>
#include <AGL/agl.h>

CFBundleRef gBundleRefOpenGL = NULL;

void my_aglEnableMultiThreading(void)
{
	CGLError err = 0;
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

// -------------------------

OSStatus _aglInitEntryPoints(void)
{
	OSStatus err = noErr;
	const Str255 frameworkName = "\pOpenGL.framework";
	FSRefParam fileRefParam;
	FSRef fileRef;
	CFURLRef bundleURLOpenGL;

	memset(&fileRefParam, 0, sizeof(fileRefParam));
	memset(&fileRef, 0, sizeof(fileRef));

	fileRefParam.ioNamePtr = frameworkName;
	fileRefParam.newRef = &fileRef;

	// Frameworks directory/folder
	err = FindFolder(kSystemDomain, kFrameworksFolderType, false,
	                 &fileRefParam.ioVRefNum, (SInt32*)&fileRefParam.ioDirID);
	if (noErr != err) {
		DebugStr("\pCould not find frameworks folder");
		return err;
	}
	err = PBMakeFSRefSync(&fileRefParam); // make FSRef for folder
	if (noErr != err) {
		DebugStr("\pCould make FSref to frameworks folder");
		return err;
	}
	// create URL to folder
	bundleURLOpenGL = CFURLCreateFromFSRef(kCFAllocatorDefault, &fileRef);
	if (!bundleURLOpenGL) {
		DebugStr("\pCould create OpenGL Framework bundle URL");
		return paramErr;
	}
	// create ref to GL's bundle
	gBundleRefOpenGL = CFBundleCreate(kCFAllocatorDefault, bundleURLOpenGL);
	if (!gBundleRefOpenGL) {
		DebugStr("\pCould not create OpenGL Framework bundle");
		return paramErr;
	}
	CFRelease(bundleURLOpenGL); // release created bundle
	// if the code was successfully loaded, look for our function.
	if (!CFBundleLoadExecutable(gBundleRefOpenGL)) {
		DebugStr("\pCould not load MachO executable");
		return paramErr;
	}
	return err;
}

// -------------------------

void aglDellocEntryPoints(void)
{
	if (gBundleRefOpenGL != NULL) {
		// unload the bundle's code.
		CFBundleUnloadExecutable(gBundleRefOpenGL);
		CFRelease(gBundleRefOpenGL);
		gBundleRefOpenGL = NULL;
	}
}

void aglInitEntryPoints(void)
{
	if (_aglInitEntryPoints() != noErr)
		abort();
}

// -------------------------

void * aglGetProcAddress(char * pszProc)
{
	return CFBundleGetFunctionPointerForName(
	                                         gBundleRefOpenGL,
	                                         CFStringCreateWithCStringNoCopy(
	                                                                         NULL,
	                                                                         pszProc,
	                                                                         CFStringGetSystemEncoding(),
	                                                                         NULL));
}
