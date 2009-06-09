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

#ifndef _X11GLUE_H
#define _X11GLUE_H 1

#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* This file details the structure that the X11 Renderer requires
 * in order to render.  It is used by the X11 Renderer itself
 * (naturally), and also any UserInterface that wishes to support
 * the X11 Renderer (through the "glue" routines associated wtih
 * the UserInterface and the Renderer).  It contains all the information
 * from the UserInterface that the Renderer needs to render.
 *
 * A structure of this sort must be loaded into the Canvas'
 * gluePrivateData pointer during "glue" initialization.
 */

 struct X11RendererGlue{
    Display *display;
    Visual *visual;
    Drawable drawable;
    GC gc;
    unsigned int depth;
    int doubleBuffered;
    int fontHeight;
} ;

#endif
