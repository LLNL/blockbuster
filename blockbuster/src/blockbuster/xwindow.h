/* Copyright (c) 2003 Tungsten Graphics, Inc.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files ("the
 * Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:  The above copyright notice, the Tungsten
 * Graphics splash screen, and this permission notice shall be included
 * in all copies or substantial portions of the Software.  THE SOFTWARE
 * IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef XWINDOW
#define XWINDOW 1

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdbe.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include "canvas.h"

struct RendererSpecificGlue *GetRendererSpecificGlueByName(QString name);
MovieStatus xwindow_Initialize(Canvas *canvas, const ProgramOptions *options,
                               qint32 uiData);
void xwindow_HandleOptions(int &argc, char *argv[]); 
void XWindow_SetTitle(QString title); 
void GetXEvent(Canvas *canvas, int block, MovieEvent *movieEvent);
void CloseXWindow(Canvas *canvas);
void ResizeXWindow(Canvas *canvas, int newWidth, int newHeight, int cameFromX);
void MoveXWindow(Canvas *canvas, int newX, int newY, int cameFromX);
void XWindow_ShowCursor(bool show);
void XWindow_ToggleCursor(void);

#endif

