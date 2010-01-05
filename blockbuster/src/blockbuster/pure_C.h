#ifndef PURE_C_H
#define PURE_C_H
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdbe.h>

#ifdef __cplusplus
extern "C" {
#endif

XVisualInfo *pureC_x11ChooseVisual(Display *display, int screenNumber);

#ifdef __cplusplus
}
#endif

#endif
