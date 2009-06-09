#ifndef BLOCKBUSTER_TEXTURE_H
#define BLOCKBUSTER_TEXTURE_H
#include "canvas.h"

void gltexture_HandleOptions(int &argc, char *argv[]);
MovieStatus gltexture_Initialize(Canvas *canvas, const ProgramOptions *options);

#define GLTEXTURE_NAME "gltexture"
#define GLTEXTURE_DESCRIPTION "Render using OpenGL texture mapping to an X11 window"


#endif
