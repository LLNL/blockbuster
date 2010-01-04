/* this file simply contains routines that cannot compile in C++, because they use reserved C++ keywords and cannot be fixed */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdbe.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>
//#include "x11glue.h"
#include "pure_C.h"
#include "errmsg.h"

/* Convert an X visual class number into a name */
#define VISUAL_CLASS_NAME(class) (\
    (class)==StaticGray?"StaticGray":\
    (class)==GrayScale?"GrayScale":\
    (class)==StaticColor?"StaticColor":\
    (class)==PseudoColor?"PseudoColor":\
    (class)==TrueColor?"TrueColor":\
    (class)==DirectColor?"DirectColor":\
    "unknown")

/* This routine is used both by X11 and by DMX (if enabled) */
  XVisualInfo *pureC_x11ChooseVisual(Display *display, int screenNumber)
{
    /* We want to double-buffer with DBE.  If we can get the extension,
     * we'll use one of the visuals that the extension supports.  If
     * we cannot, we'll just get a list of visuals that X supports.
     */
    int dbeMajorVersion, dbeMinorVersion;
    XdbeScreenVisualInfo *dbeVisualInfo;
    long visualInfoMask;
    XVisualInfo visualInfoTemplate, *visualInfo;
    int numVisuals;
    int candidateIndex;
    int candidatePerfLevel = 0;
    Status status;

    status = XdbeQueryExtension(display, &dbeMajorVersion, &dbeMinorVersion);
    if (status == 0) {
      DEBUGMSG("DBE extension not found; running without double-buffering.");
        dbeVisualInfo = NULL;
    }
    else { 
        int numDBEVisuals = 0;

        DEBUGMSG("found DBE extension %d.%d", dbeMajorVersion, dbeMinorVersion);
        dbeVisualInfo = XdbeGetVisualInfo(display, NULL, &numDBEVisuals);
        if (dbeVisualInfo == NULL) {
            WARNING("found DBE extension %d.%d, but no DBE visuals", 
                dbeMajorVersion, dbeMinorVersion);
        }
        else {
            DEBUGMSG("found DBE extension %d.%d and %d DBE visual%s", 
                dbeMajorVersion, dbeMinorVersion, numDBEVisuals,
                numDBEVisuals==1?"":"s");
        }
    }

    /* We ignore the suggestedDepth parameter; in fact, as under Linux
     * we're pretty much restricted to having only one choice for
     * visual depth for any one screen, we're just taking that choice.
     * Get the list of visuals that support the screen we've got,
     * compare them with the list of visuals that support DBE, and
     * choose the one with the best DBE-reported performance level.
     */
    visualInfoMask = VisualScreenMask;
    visualInfoTemplate.screen = screenNumber;
    visualInfo = XGetVisualInfo(display, visualInfoMask,
                                &visualInfoTemplate, &numVisuals);
    if (visualInfo == NULL) {
        ERROR("no visuals matching screen %d found", screenNumber);
        if (dbeVisualInfo != NULL) {
            XdbeFreeVisualInfo(dbeVisualInfo);
        }
        return NULL;
    }
    else {
        DEBUGMSG("found %d visuals on screen %d", numVisuals, screenNumber);
    }
    candidateIndex = -1;
    if (dbeVisualInfo) {
        int i, j;
        for (i = 0; i < numVisuals; i++) {
            /* Look for the appropriate visual in the DBE visual list */
            for (j = 0; j < dbeVisualInfo->count; j++) {
                if (visualInfo[i].visualid == dbeVisualInfo->visinfo[j].visual) {
                    /* If it's the first candidate, or a superior candidate,
                     * save it
                     */
                    if (
                        candidateIndex == -1 || 
                        dbeVisualInfo->visinfo[j].perflevel > candidatePerfLevel
                    ) {
                        candidateIndex = i;
                        candidatePerfLevel = dbeVisualInfo->visinfo[j].perflevel;
                    }
                }
            }
        }

        /* All done with dbeVisualInfo - free it */
        XdbeFreeVisualInfo(dbeVisualInfo);
        dbeVisualInfo = NULL;
    }
    else {
       /* just use first visual */
       candidateIndex =0;
    }

    if (candidateIndex == -1) {
        WARNING(
            "no visuals on screen %d supporting DBE were found; using a non-DBE visual",
            screenNumber);
        candidateIndex = 0;
    }

    DEBUGMSG("found visual ID %d,  depth %d, class %s, bits_per_rgb %d",
            visualInfo[candidateIndex].visualid,
            visualInfo[candidateIndex].depth,
            VISUAL_CLASS_NAME(visualInfo[candidateIndex].class),
            visualInfo[candidateIndex].bits_per_rgb);

    /* X has allocated the entire visual list in visualInfo[].  We
     * want to free that before we return, so query again for a
     * visual, using the known visual ID.
     */
    visualInfoMask = VisualIDMask | VisualScreenMask;
    visualInfoTemplate.screen = screenNumber;
    visualInfoTemplate.visualid = visualInfo[candidateIndex].visualid;
    XFree(visualInfo);

    visualInfo = XGetVisualInfo(display, visualInfoMask,
                                &visualInfoTemplate, &numVisuals);
    if (visualInfo == NULL) {
        ERROR("visual disappeared!");
        return NULL;
    }

    return visualInfo;
}

