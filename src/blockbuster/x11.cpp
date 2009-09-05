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
#include "errmsg.h"
#include "blockbuster_x11.h"
#include "cache.h"
#include "util.h"
#include "frames.h"
#include "errmsg.h"
#include "convert.h"

#include "errmsg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdbe.h>
#include "x11glue.h"



/* These global variables define our behavior.  They are modified by 
 * passing in options.
 */
static int globalSync = 0;
static XdbeSwapAction globalSwapAction = XdbeBackground;
struct x11SwapAction {
    char *name;
    XdbeSwapAction action;
    char *description;
} ;
static x11SwapAction swapActions[] = {
    {"undefined", XdbeUndefined, "back buffer becomes undefined on swap"},
    {"background", XdbeBackground, "back buffer is cleared to window background on swap"},
    {"untouched", XdbeUntouched, "back buffer is contents of front buffer on swap"},
    {"copied", XdbeCopied, "back buffer is held constant on swap"},
    {NULL, XdbeUndefined, NULL}
};
char *GetSwapActionName(XdbeSwapAction swapAction)
{
    register int i;
    for (i = 0; swapActions[i].name != NULL; i++) {
        if (swapActions[i].action == swapAction) return swapActions[i].name;
    }
    return "unknown";
}

 x11SwapAction *GetSwapAction(char *name)
{
    register int i;
    for (i = 0; swapActions[i].name != NULL; i++) {
        if (strcmp(swapActions[i].name, name) == 0) return &swapActions[i];
    }
    return NULL;
}



void x11_HandleOptions(int &argc, char *argv[])
{

  x11SwapAction *swapAction;

  while (argc > 1) {
    if (!strcmp(argv[1], "-h")) {
      int i=0; 
      fprintf(stderr, "Renderer: %s\n", X11_NAME);
      fprintf(stderr, "%s\n", X11_DESCRIPTION);
      fprintf(stderr, "Options:\n");
      fprintf(stderr, "-h gives help\n");
      fprintf(stderr, "-s toggles XSynchronize [%s]\n",
              globalSync?"on":"off");
      fprintf(stderr, "-a specifies swap action [%s]:\n",
              GetSwapActionName(globalSwapAction));
      for (i = 0; swapActions[i].name != NULL; i++) {
        fprintf(stderr, "    %s: %s\n",
                swapActions[i].name, swapActions[i].description);
      }
      exit(MOVIE_HELP);
    } else if (!strcmp(argv[1], "-s")) {
      ConsumeArg(argc, argv, 1); 
      globalSync = !globalSync;
    } else if (!strcmp(argv[1], "-a")) {
      ConsumeArg(argc, argv, 1); 
      swapAction = GetSwapAction(argv[1]);
      ConsumeArg(argc, argv, 1); 
      if (swapAction == NULL) {
        fprintf(stderr, "Renderer %s: no such swap action.  Use -h for help.\n",
                X11_NAME);
        exit(MOVIE_BAD_FLAG);
      }
      else {
        globalSwapAction = swapAction->action;
      }      
    }
    else {
      /* stop consuming args when one does not match */ 
      return; 
    }
  }
  return; 
}



static void x11_Render(Canvas *canvas, int frameNumber,
                   const Rectangle *imageRegion,
                   int destX, int destY, float zoom, int lod)
{
    X11RendererGlue *glueInfo;
    XImage *xImage;
    Image *image;
    char *start;
    int stride;
    int lodScale;
    int subWidth, subHeight;
        int localFrameNumber;
    Rectangle region = *imageRegion;

#if 0
    printf("x11::Render %d, %d %d x %d  at %d, %d  z=%f  lod=%d\n",
           imageRegion->x, imageRegion->y, imageRegion->width, imageRegion->height,
           destX, destY, zoom, lod);
#endif

    bb_assert(canvas->gluePrivateData);
    glueInfo = (X11RendererGlue *)canvas->gluePrivateData;

        if (canvas->frameList->stereo) {
          localFrameNumber = frameNumber *2; /* we'll display left frame only */
        }
        else {
          localFrameNumber = frameNumber;
        }

    /*
     * Compute possibly reduced-resolution image region to display.
     */
	bb_assert(lod <= canvas->frameList->getFrame(localFrameNumber)->maxLOD);
    lodScale = 1 << lod;
    region.x = imageRegion->x / lodScale;
    region.y = imageRegion->y / lodScale;
    region.width = imageRegion->width / lodScale;
    region.height = imageRegion->height / lodScale;
    zoom *= (float) lodScale;

    /* Pull the image from our cache */
    image = canvas->imageCache->GetImage(localFrameNumber, &region, lod);
    if (image == NULL) {
        /* error has already been reported */
        return;
    }

    bb_assert(region.x + region.width <= static_cast<int32_t>(image->width));
    bb_assert(region.y + region.height <= static_cast<int32_t>(image->height));
    bb_assert(image->imageFormat.rowOrder == TOP_TO_BOTTOM);

    /* If we have to, rescale the image.  Note that this is rather inefficient
     * in the X11 renderer case, as there is no API to do this directly; we
     * have to instead scale the image.
     *
     * Ultimately it may be a better idea to scale the image before putting
     * it in cache, so that the scaled image is directly available.  As we're
     * not likely to improve the X11 renderer, this is unlikely to ever
     * be implemented.
     */

    if (zoom != 1.0) {
        Image *zoomedImage;

        subWidth = (int) (region.width * zoom + 0.0);
        subHeight = (int) (region.height * zoom + 0.0);

        zoomedImage = ScaleImage(image, canvas,
                        region.x, region.y,
                        region.width, region.height,
                        subWidth, subHeight);

        /* With the image zoomed, we no longer need the image
         * that's in the cache.  But we'll have to free this
         * image later.
         */
        canvas->imageCache->ReleaseImage(image);
        image = zoomedImage;
        if (image == NULL) {
            /* error has already been reported */
            return;
        }
    }
    else {
        subWidth = region.width;
        subHeight = region.height;
    }

    /*
     * Create an XImage for the imageRegion.
     */
    /* Compute row stride and start address of sub-region */
    stride = ROUND_TO_MULTIPLE(image->width
                               * image->imageFormat.bytesPerPixel, 4);
    if (zoom == 1.0) {
        start = (char *) image->imageData + region.y * stride
            + region.x * image->imageFormat.bytesPerPixel;
    }
    else {
        /* we'll draw the whole rescaled image */
        start = (char *) image->imageData;
    }

    xImage = XCreateImage(
        glueInfo->display,
        glueInfo->visual,
        glueInfo->depth,
        ZPixmap,
        0, /* no offset to the image data */
        start,
        subWidth,
        subHeight,
        image->imageFormat.scanlineByteMultiple * 8,
        stride
        );

    if (xImage == NULL) {
        ERROR("In X11::Render could not create %dx%dx%d XImage",
              image->loadedRegion.width, image->loadedRegion.height, 24);
        return;
    }

    XPutImage(glueInfo->display,
              glueInfo->drawable,
              glueInfo->gc,
              xImage,
              0, 0, /* src_x, src_y */
              destX, destY,
              subWidth, subHeight
              );

    if (!glueInfo->doubleBuffered) {
        /* clear unpainted window regions */
        int x, y, w, h;
        /* above */
        if (destY > 0) {
            x = 0;
            y = 0;
            w = canvas->width;
            h = destY;
            XClearArea(glueInfo->display, glueInfo->drawable,
                    x, y, w, h, False);
        }
        /* below */
        if (destY + (int) (subHeight * zoom) < canvas->height) {
            x = 0;
            y = destY + (int) (subHeight * zoom);
            w = canvas->width;
            h = canvas->height - y;
            XClearArea(glueInfo->display, glueInfo->drawable, 
                    x, y, w, h, False);
        }
        /* left */
        if (destX > 0) {
            x = 0;
            y = destY;
            w = destX;
            h = canvas->height - y;
            XClearArea(glueInfo->display, glueInfo->drawable,
                    x, y, w, h, False);
        }
        /* right */
        if (destX + (int) (subWidth * zoom) < canvas->width) {
            x = destX + (int) (subWidth * zoom);
            y = destY;
            w = canvas->width - x;
            h = canvas->height - y;
            XClearArea(glueInfo->display, glueInfo->drawable, 
                    x, y, w, h, False);
        }
    }

    /* Don't let the XDestroyImage routine eliminate our external data */
    xImage->data = NULL;
    XDestroyImage(xImage);

    /* If we zoomed, we've already released the image from the image
     * cache and we're using a locally-allocated image instead; in this
     * case, we need to deallocate the local image.  If we haven't zoomed,
     * we're still using the original image from the cache; we need to
     * release it so that the image cache knows we're done with it.
     * Note that if we are deallocating the image, the ImageDeallocator
     * function will call the ImageDataDeallocator function itself.
     */
    if (zoom != 1.0) {
        (*image->ImageDeallocator)(canvas, image);
    }
    else {
      canvas->imageCache->ReleaseImage( image);
    }
}

static void
x11_DrawString(Canvas *canvas, int row, int column, const char *str)
{
    X11RendererGlue *glueInfo = (X11RendererGlue *)canvas->gluePrivateData;
    int x = (column + 1) * glueInfo->fontHeight;
    int y = (row + 1) * glueInfo->fontHeight;
    XDrawString(glueInfo->display,
                glueInfo->drawable,
                glueInfo->gc, 
                x, y, str, strlen(str));
}

/*
 * After the canvas has been created, as well as the corresponding X window,
 * this function is called to do any renderer-specific setup.
 */
 MovieStatus
x11_Initialize(Canvas *canvas, const ProgramOptions *)
{
    /* Trivial for gl renderer: */
    /* Plug in our functions into the canvas */
    canvas->Render = x11_Render;
    /* If the UserInterface implements this routine, we should not use ours */
    if (canvas->DrawString == NULL) { 
        canvas->DrawString = x11_DrawString;
    }
    canvas->ImageDataAllocator = DefaultImageDataAllocator;
    canvas->ImageDataDeallocator = DefaultImageDataDeallocator;
    canvas->DestroyRenderer = NULL;

    /* We CAN'T override resize/move functions - DMX needs to use them - BP */
    /* We don't have anything to do when the UserInterface informs
     * us of resizing or moving.
     */
    //canvas->Resize = NULL;
    //canvas->Move = NULL;

    return MovieSuccess;
}

