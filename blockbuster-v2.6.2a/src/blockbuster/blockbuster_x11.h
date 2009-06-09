#ifndef BLOCKBUSTER_X11_H
#define BLOCKBUSTER_X11_H
#include "common.h"
#include "canvas.h"

void x11_HandleOptions(int &argc, char *argv[]);
MovieStatus x11_Initialize(Canvas *canvas, const ProgramOptions *options);

#define X11_NAME "x11"
#define X11_DESCRIPTION "Render using X11 rendering routines to an X11 window"

#endif

