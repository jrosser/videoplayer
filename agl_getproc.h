#ifndef AGL_GETPROC_H_
#define AGL_GETPROC_H_

void my_aglSwapInterval(int interval);
void * aglGetProcAddress (char * pszProc);
void aglInitEntryPoints (void);
void aglDellocEntryPoints(void);
#endif /*AGL_GETPROC_H_*/
