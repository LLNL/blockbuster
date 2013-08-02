
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

x11Renderer::x11Renderer(ProgramOptions *opt, Window parentWindow, 
              BlockbusterInterface *gui, QString name):
  Renderer(opt, parentWindow, gui, name), mSwapAction(XdbeBackground) {
  return; 
}

void x11Renderer::FinishRendererInit(ProgramOptions *) {
  ECHO_FUNCTION(5);
   
  /* This graphics context and font will be used for rendering status messages,
   */
  mGC = XCreateGC(mDisplay, mWindow, 0, NULL);
  XSetFont(mDisplay, mGC, mFontInfo->fid);
  XSetForeground(mDisplay, mGC,
                 WhitePixel(mDisplay, mScreenNumber));
  
  mBackBuffer = 
    XdbeAllocateBackBufferName(mDisplay, mWindow, mSwapAction);
  
 if (mBackBuffer) {
    mDoubleBuffered = 1;
    mDrawable = mBackBuffer;
  }
  else {
    mDoubleBuffered = 0;
    mDrawable = mWindow;
  }
  
  /* Specify our required format.  Note that 24-bit X11 images require
   * *4* bytes per pixel, not 3.
   */
  if (mVisInfo->depth > 16) {
    mRequiredImageFormat.bytesPerPixel = 4;
  }
  else if (mVisInfo->depth > 8) {
    mRequiredImageFormat.bytesPerPixel = 2;
  }
  else {
    mRequiredImageFormat.bytesPerPixel = 1;
  }
  mRequiredImageFormat.scanlineByteMultiple = BitmapPad(mDisplay)/8;
  
  /* If the bytesPerPixel value is 3 or 4, we don't need these;
   * but we'll put them in anyway.
   */
  mRequiredImageFormat.redShift = ComputeShift(mVisInfo->visual->red_mask) - 8;
  mRequiredImageFormat.greenShift = ComputeShift(mVisInfo->visual->green_mask) - 8;
  mRequiredImageFormat.blueShift = ComputeShift(mVisInfo->visual->blue_mask) - 8;
  mRequiredImageFormat.redMask = mVisInfo->visual->red_mask;
  mRequiredImageFormat.greenMask = mVisInfo->visual->green_mask;
  mRequiredImageFormat.blueMask = mVisInfo->visual->blue_mask;
  mRequiredImageFormat.byteOrder = ImageByteOrder(mDisplay);
  mRequiredImageFormat.rowOrder = TOP_TO_BOTTOM;
  
    return; 
}

//====================================================================
x11Renderer::~x11Renderer() {
  XFreeGC(mDisplay, mGC);
  
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
  lodScale = 1 << lod;
  region.x = imageRegion->x / lodScale;
  region.y = imageRegion->y / lodScale;
  region.width = imageRegion->width / lodScale;
  region.height = imageRegion->height / lodScale;
  zoom *= (float) lodScale;
  
  /* Pull the image from our cache */
  ImagePtr image = GetImage(localFrameNumber, &region, lod);
  if (!image) {
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
    
    subWidth = (int) (region.width * zoom + 0.0);
    subHeight = (int) (region.height * zoom + 0.0);
    
    ImagePtr zoomedImage = ScaleImage(image, region.x, region.y,
                             region.width, region.height,
                             subWidth, subHeight);
    
    /* With the image zoomed, we no longer need the image
     * that's in the cache.  But we'll have to free this
     * image later.
     */
    DecrementLockCount(image);
    image = zoomedImage;
    if (!image) {
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
    start = (char*)image->Data() + region.y * stride
      + region.x * image->imageFormat.bytesPerPixel;
  }
  else {
    /* we'll draw the whole rescaled image */
    start = (char*)image->Data();
  }
  
  xImage = XCreateImage(mDisplay,
                        mVisInfo->visual,
                        mVisInfo->depth,
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
  
  XPutImage(mDisplay,
            mDrawable,
            mGC,
            xImage,
            0, 0, /* src_x, src_y */
            destX, destY,
            subWidth, subHeight
            );
  
  if (!mDoubleBuffered) {
    /* clear unpainted window regions */
    int x, y, w, h;
    /* above */
    if (destY > 0) {
      x = 0;
      y = 0;
      w = mWidth;
      h = destY;
      XClearArea(mDisplay, mDrawable,
                 x, y, w, h, False);
    }
    /* below */
    if (destY + (int) (subHeight * zoom) < mHeight) {
      x = 0;
      y = destY + (int) (subHeight * zoom);
      w = mWidth;
      h = mHeight - y;
      XClearArea(mDisplay, mDrawable, 
                 x, y, w, h, False);
    }
    /* left */
    if (destX > 0) {
      x = 0;
      y = destY;
      w = destX;
      h = mHeight - y;
      XClearArea(mDisplay, mDrawable,
                 x, y, w, h, False);
    }
    /* right */
    if (destX + (int) (subWidth * zoom) < mWidth) {
      x = destX + (int) (subWidth * zoom);
      y = destY;
      w = mWidth - x;
      h = mHeight - y;
      XClearArea(mDisplay, mDrawable, 
                 x, y, w, h, False);
    }
  }
  
  /* Don't let the XDestroyImage routine eliminate our external data */
  xImage->data = NULL;
  XDestroyImage(xImage);
  
  /* If we haven't zoomed,
   * we're still using the original image from the cache; we need to
   * release it so that the image cache knows we're done with it.
   */
  if (zoom == 1.0) {
    DecrementLockCount( image);
  }
  return; 
}

//========================================================
void  x11Renderer::DrawString(int row, int column, const char *str) {
  ECHO_FUNCTION(5);
  int x = (column + 1) * mFontHeight;
  int y = (row + 1) * mFontHeight;
  XDrawString(mDisplay,
              mDrawable,
              mGC, 
              x, y, str, strlen(str));
  return; 
}

//========================================================
void x11Renderer::SwapBuffers(void) {
  if (mBackBuffer) {
    /* If we're using DBE */
    XdbeSwapInfo swapInfo;
    swapInfo.swap_window = mWindow;
    swapInfo.swap_action = mSwapAction;
    XdbeSwapBuffers(mDisplay, &swapInfo, 1);
    /* Force sync, in case we get no events (dmx) */
    XSync(mDisplay, 0);
  }
  return;
}
