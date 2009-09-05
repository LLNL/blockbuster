#ifndef BLOCKBUSTER_GL_H
#define BLOCKBUSTER_GL_H
#include "common.h"

void gl_HandleOptions(int &argc, char *argv[]);
MovieStatus gl_Initialize(Canvas *canvas, const ProgramOptions *options);
MovieStatus gl_InitializeStereo(Canvas *canvas, const ProgramOptions *options);


#define GL_NAME "gl"
#define GL_DESCRIPTION "Render using OpenGL glDrawPixels to an X11 window"

#define GL_NAME_STEREO "gl_stereo"
#define GL_DESCRIPTION_STEREO "Render using OpenGL glDrawPixels to an X11 window in stereo"
#endif

