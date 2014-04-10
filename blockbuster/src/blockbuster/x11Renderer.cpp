
#include "x11Renderer.h"
#include "errmsg.h"
#include "util.h"
#include "frames.h"
#include "errmsg.h"
#include "errmsg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdbe.h>


void x11Renderer::FinishRendererInit(void) {
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
  if (mVisualInfo->depth > 16) {
    mRequiredImageFormat.bytesPerPixel = 4;
  }
  else if (mVisualInfo->depth > 8) {
    mRequiredImageFormat.bytesPerPixel = 2;
  }
  else {
    mRequiredImageFormat.bytesPerPixel = 1;
  }
  mRequiredImageFormat.scanlineByteMultiple = BitmapPad(mDisplay)/8;
  
  /* If the bytesPerPixel value is 3 or 4, we don't need these;
   * but we'll put them in anyway.
   */
  mRequiredImageFormat.redShift = ComputeShift(mVisualInfo->visual->red_mask) - 8;
  mRequiredImageFormat.greenShift = ComputeShift(mVisualInfo->visual->green_mask) - 8;
  mRequiredImageFormat.blueShift = ComputeShift(mVisualInfo->visual->blue_mask) - 8;
  mRequiredImageFormat.redMask = mVisualInfo->visual->red_mask;
  mRequiredImageFormat.greenMask = mVisualInfo->visual->green_mask;
  mRequiredImageFormat.blueMask = mVisualInfo->visual->blue_mask;
  mRequiredImageFormat.byteOrder = ImageByteOrder(mDisplay);
  mRequiredImageFormat.rowOrder = TOP_TO_BOTTOM;
  
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
/*
 * Take the indicated source region (srcX, srcY, srcWidth, srcHeight) of
 * the image and return a new image of that region scaled to zoomeWidth
 * by zoomedHeight
 */
ImagePtr x11Renderer::ScaleImage( ImagePtr image, int srcX, int srcY,
                  int srcWidth, int srcHeight,
                  int zoomedWidth, int zoomedHeight)
{
  const ImageFormat *format = &image->imageFormat;
  register int x, y, i;
  const int bytesPerScanline = ROUND_UP_TO_MULTIPLE(
                                                 format->bytesPerPixel * image->width,
                                                 format->scanlineByteMultiple
                                                 );
  const int zoomedBytesPerScanline = ROUND_UP_TO_MULTIPLE(
                                                       format->bytesPerPixel * zoomedWidth,
                                                       format->scanlineByteMultiple
                                                       );

  /*dbprintf(0,"ScaleImage [%d,%d]->[%d,%d]\n",srcWidth,srcHeight,zoomedWidth,zoomedHeight); */
  register unsigned char *zoomedScanline, *zoomedData, *pixelData;

  ImagePtr zoomedImage(new Image()); 
  if (!zoomedImage) {
	ERROR("could not allocate Image structure");
	return ImagePtr();
  }
  if (!zoomedImage->allocate(zoomedHeight * zoomedBytesPerScanline)) {
	ERROR("could not allocate %dx%dx%d zoomed image data",
          zoomedHeight, zoomedWidth, format->bytesPerPixel*8);
	return ImagePtr();
  }
  zoomedImage->height = zoomedHeight;
  zoomedImage->width = zoomedWidth;
  zoomedImage->imageFormat = *format;
  zoomedImage->loadedRegion.x = 0;
  zoomedImage->loadedRegion.y = 0;
  zoomedImage->loadedRegion.width = zoomedWidth;
  zoomedImage->loadedRegion.height = zoomedHeight;
  zoomedImage->width = zoomedWidth;
  zoomedImage->height = zoomedHeight;

  zoomedScanline = (unsigned char *) zoomedImage->Data();
  for (y = 0; y < zoomedHeight; y++) {
	const int unzoomedY = srcY + y * srcHeight / zoomedHeight;
	zoomedData = zoomedScanline;
	if (format->bytesPerPixel == 4) {
      const int *srcRow = (int *)
		((const char *) image->ConstData() + unzoomedY * bytesPerScanline);
      int *dstRow = (int *) zoomedData;
      for (x = 0; x < zoomedWidth; x++) {
		/* Figure out where the source pixel is */
		const int unzoomedX = srcX + x * srcWidth / zoomedWidth;
		dstRow[x] = srcRow[unzoomedX];
      }
	}
	else {
      /* arbitrary bytes per pixel */
      /* XXX optimize someday */
      for (x = 0; x < zoomedWidth; x++) {
		/* Figure out where the source pixel is */
		const int unzoomedX = srcX + x * srcWidth / zoomedWidth;

		pixelData = (unsigned char *) image->ConstData() + 
          unzoomedY * bytesPerScanline +
          unzoomedX * format->bytesPerPixel;

		/* Copy over the pixel */
		for (i = 0; i < format->bytesPerPixel; i++) {
          *zoomedData++ = *pixelData++;
		}
      }
	}

	/* Adjust the scanline */
	zoomedScanline += zoomedBytesPerScanline;
  }

  return zoomedImage;
}

//====================================================================
void x11Renderer::RenderActual(Rectangle imageRegion){
  ECHO_FUNCTION(5);
  XImage *xImage;
  char *start;
  int stride;
  int lodScale;
  int subWidth, subHeight;
  int localFrameNumber;
  /*int frameNumber, RectanglePtr imageRegion,
                               int destX, int destY, float zoom, int lod
  */
#if 0
  printf("x11::Render %d, %d %d x %d  at %d, %d  z=%f  lod=%d\n",
         imageRegion.x, imageRegion.y, imageRegion.width, imageRegion.height,
         mImageDrawX, mImageDrawY, zoom, mLOD);
#endif
    
  if (mFrameList->mStereo) {
    localFrameNumber = mCurrentFrame *2; /* we'll display left frame only */
  }
  else {
    localFrameNumber = mCurrentFrame;
  }
  
  /*
   * Compute possibly reduced-resolution image region to display.
   */
  lodScale = 1 << mLOD;
  imageRegion.x = imageRegion.x / lodScale;
  imageRegion.y = imageRegion.y / lodScale;
  imageRegion.width = imageRegion.width / lodScale;
  imageRegion.height = imageRegion.height / lodScale;
  float zoom = mZoom * (float) lodScale;
  
  /* Pull the image from our cache */
  ImagePtr image = GetImage(localFrameNumber, &imageRegion, mLOD);
  if (!image) {
    /* error has already been reported */
    return;
  }
  
  bb_assert(imageRegion.x + imageRegion.width <= static_cast<int32_t>(image->width));
  bb_assert(imageRegion.y + imageRegion.height <= static_cast<int32_t>(image->height));
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
    
    subWidth = (int) (imageRegion.width * zoom + 0.0);
    subHeight = (int) (imageRegion.height * zoom + 0.0);
    
    ImagePtr zoomedImage = ScaleImage(image, imageRegion.x, imageRegion.y,
                             imageRegion.width, imageRegion.height,
                             subWidth, subHeight);
    
    /* With the image zoomed, we no longer need the image
     * that's in the cache.  But we'll have to free this
     * image later.
     */
    image = zoomedImage;
    if (!image) {
      /* error has already been reported */
      return;
    }
  }
  else {
    subWidth = imageRegion.width;
    subHeight = imageRegion.height;
  }
  
  /*
   * Create an XImage for the imageRegion.
   */
  /* Compute row stride and start address of sub-region */
  stride = ROUND_UP_TO_MULTIPLE(image->width
                             * image->imageFormat.bytesPerPixel, 4);
  if (zoom == 1.0) {
    start = (char*)image->Data() + imageRegion.y * stride
      + imageRegion.x * image->imageFormat.bytesPerPixel;
  }
  else {
    /* we'll draw the whole rescaled image */
    start = (char*)image->Data();
  }
  
  xImage = XCreateImage(mDisplay,
                        mVisualInfo->visual,
                        mVisualInfo->depth,
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
            mImageDrawX, mImageDrawY,
            subWidth, subHeight
            );
  
  if (!mDoubleBuffered) {
    /* clear unpainted window regions */
    int x, y, w, h;
    /* above */
    if (mImageDrawY > 0) {
      x = 0;
      y = 0;
      w = mWindowWidth;
      h = mImageDrawY;
      XClearArea(mDisplay, mDrawable,
                 x, y, w, h, False);
    }
    /* below */
    if (mImageDrawY + (int) (subHeight * zoom) < mWindowHeight) {
      x = 0;
      y = mImageDrawY + (int) (subHeight * zoom);
      w = mWindowWidth;
      h = mWindowHeight - y;
      XClearArea(mDisplay, mDrawable, 
                 x, y, w, h, False);
    }
    /* left */
    if (mImageDrawX > 0) {
      x = 0;
      y = mImageDrawY;
      w = mImageDrawX;
      h = mWindowHeight - y;
      XClearArea(mDisplay, mDrawable,
                 x, y, w, h, False);
    }
    /* right */
    if (mImageDrawX + (int) (subWidth * zoom) < mWindowWidth) {
      x = mImageDrawX + (int) (subWidth * zoom);
      y = mImageDrawY;
      w = mWindowWidth - x;
      h = mWindowHeight - y;
      XClearArea(mDisplay, mDrawable, 
                 x, y, w, h, False);
    }
  }
  
  /* Don't let the XDestroyImage routine eliminate our external data */
  xImage->data = NULL;
  XDestroyImage(xImage);
  
  /* Print info in upper-left corner */
  char str[100];
  int row = 0;
  sprintf(str, "Frame %d of %d", mCurrentFrame + 1, mEndFrame);
  DrawString(row++, 0, str);
  sprintf(str, "Frame Size: %d by %d pixels", mImageWidth, mImageHeight);
  DrawString(row++, 0, str);
  sprintf(str, "Position: %d, %d", mImageDrawX,  mImageDrawY);
  DrawString(row++, 0, str);
  sprintf(str, "Zoom: %5.2f  LOD: %d", mZoom, mLOD);
  
  DrawString(row++, 0, str);
  sprintf(str, "FPS: %5.1f (target %.1f)", mFPS, mTargetFPS);
  DrawString(row++, 0, str);

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
