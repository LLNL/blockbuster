
#include "canvas.h" 
#include "x11Renderer.h"
#include "errmsg.h"
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

x11Renderer::x11Renderer(ProgramOptions *opt, Canvas *canvas, Window parentWindow):
  Renderer(opt, canvas, parentWindow, "x11"), mSwapAction(XdbeBackground) {
  
  ECHO_FUNCTION(5);
   
  /* This graphics context and font will be used for rendering status messages,
   */
  gc = XCreateGC(display, window, 0, NULL);
  XSetFont(display, gc, fontInfo->fid);
  XSetForeground(display, gc,
                 WhitePixel(display, screenNumber));
  
  backBuffer = 
    XdbeAllocateBackBufferName(display, window, mSwapAction);
  
 if (backBuffer) {
    doubleBuffered = 1;
    drawable = backBuffer;
  }
  else {
    doubleBuffered = 0;
    drawable = window;
  }
  gc = gc;
  
  /* Specify our required format.  Note that 24-bit X11 images require
   * *4* bytes per pixel, not 3.
   */
  if (visInfo->depth > 16) {
    canvas->requiredImageFormat.bytesPerPixel = 4;
  }
  else if (visInfo->depth > 8) {
    canvas->requiredImageFormat.bytesPerPixel = 2;
  }
  else {
    canvas->requiredImageFormat.bytesPerPixel = 1;
  }
  canvas->requiredImageFormat.scanlineByteMultiple = BitmapPad(display)/8;
  
  /* If the bytesPerPixel value is 3 or 4, we don't need these;
   * but we'll put them in anyway.
   */
  canvas->requiredImageFormat.redShift = ComputeShift(visInfo->visual->red_mask) - 8;
  canvas->requiredImageFormat.greenShift = ComputeShift(visInfo->visual->green_mask) - 8;
  canvas->requiredImageFormat.blueShift = ComputeShift(visInfo->visual->blue_mask) - 8;
  canvas->requiredImageFormat.redMask = visInfo->visual->red_mask;
  canvas->requiredImageFormat.greenMask = visInfo->visual->green_mask;
  canvas->requiredImageFormat.blueMask = visInfo->visual->blue_mask;
  canvas->requiredImageFormat.byteOrder = ImageByteOrder(display);
  canvas->requiredImageFormat.rowOrder = TOP_TO_BOTTOM;
  
    return; 
}

//====================================================================
x11Renderer::~x11Renderer() {
  XFreeGC(display, gc);
  
  return; 
}


//====================================================================
/* This utility function converts a raw mask into a mask and shift */
int x11Renderer::ComputeShift(unsigned long mask) {
  register int shiftCount = 0;
  while (mask != 0) {
	mask >>= 1;
	shiftCount++;
  }
  return shiftCount;
}

//====================================================================
void x11Renderer::Render(int frameNumber,const Rectangle *imageRegion,
                         int destX, int destY, float zoom, int lod){
  ECHO_FUNCTION(5);
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
    
  if (mFrameList->stereo) {
    localFrameNumber = frameNumber *2; /* we'll display left frame only */
  }
  else {
    localFrameNumber = frameNumber;
  }
  
  /*
   * Compute possibly reduced-resolution image region to display.
   */
  bb_assert(lod <= mFrameList->getFrame(localFrameNumber)->maxLOD);
  lodScale = 1 << lod;
  region.x = imageRegion->x / lodScale;
  region.y = imageRegion->y / lodScale;
  region.width = imageRegion->width / lodScale;
  region.height = imageRegion->height / lodScale;
  zoom *= (float) lodScale;
  
  /* Pull the image from our cache */
  image = GetImage(localFrameNumber, &region, lod);
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
    
    zoomedImage = ScaleImage(image, region.x, region.y,
                             region.width, region.height,
                             subWidth, subHeight);
    
    /* With the image zoomed, we no longer need the image
     * that's in the cache.  But we'll have to free this
     * image later.
     */
    ReleaseImage(image);
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
  
  xImage = XCreateImage(display,
                        visInfo->visual,
                        visInfo->depth,
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
  
  XPutImage(display,
            drawable,
            gc,
            xImage,
            0, 0, /* src_x, src_y */
            destX, destY,
            subWidth, subHeight
            );
  
  if (!doubleBuffered) {
    /* clear unpainted window regions */
    int x, y, w, h;
    /* above */
    if (destY > 0) {
      x = 0;
      y = 0;
      w = mCanvas->width;
      h = destY;
      XClearArea(display, drawable,
                 x, y, w, h, False);
    }
    /* below */
    if (destY + (int) (subHeight * zoom) < mCanvas->height) {
      x = 0;
      y = destY + (int) (subHeight * zoom);
      w = mCanvas->width;
      h = mCanvas->height - y;
      XClearArea(display, drawable, 
                 x, y, w, h, False);
    }
    /* left */
    if (destX > 0) {
      x = 0;
      y = destY;
      w = destX;
      h = mCanvas->height - y;
      XClearArea(display, drawable,
                 x, y, w, h, False);
    }
    /* right */
    if (destX + (int) (subWidth * zoom) < mCanvas->width) {
      x = destX + (int) (subWidth * zoom);
      y = destY;
      w = mCanvas->width - x;
      h = mCanvas->height - y;
      XClearArea(display, drawable, 
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
   */
  if (zoom != 1.0) {
    if (image->imageData) free(image->imageData); 
    free(image);
  }
  else {
    mCanvas->mRenderer->ReleaseImage( image);
  }
}

//========================================================
void  x11Renderer::DrawString(int row, int column, const char *str) {
  ECHO_FUNCTION(5);
  int x = (column + 1) * fontHeight;
  int y = (row + 1) * fontHeight;
  XDrawString(display,
              drawable,
              gc, 
              x, y, str, strlen(str));
  return; 
}

//========================================================
void x11Renderer::SwapBuffers(void) {
  if (backBuffer) {
    /* If we're using DBE */
    XdbeSwapInfo swapInfo;
    swapInfo.swap_window = window;
    swapInfo.swap_action = mSwapAction;
    XdbeSwapBuffers(display, &swapInfo, 1);
    /* Force sync, in case we get no events (dmx) */
    XSync(display, 0);
  }
  return;
}
