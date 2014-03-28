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
#include <stdio.h>
#include <stdlib.h>
#include "convert.h"
#include <X11/Xlib.h>
#include "util.h"
#include "errmsg.h"

static void ConvertPixel(const ImageFormat *srcFormat,
                         const ImageFormat *destFormat,
                         const unsigned char *src, unsigned char *dest)
{
  unsigned char red, green, blue;

  /* Pull out the R, G, and B components */
  if (srcFormat->bytesPerPixel >= 3) {
	if (srcFormat->byteOrder == MSB_FIRST) {
      red = *src++;
      green = *src++;
      blue = *src++;
	}
	else {
      blue = *src++;
      green = *src++;
      red = *src++;
	}
  }
  else {
	/* Fill this in with smarter code later, if necessary */
	static int forewarned = 0;
	if (!forewarned) {
      ERROR("trying to convert from %d bytes per pixel - using black",
		    srcFormat->bytesPerPixel);
      forewarned = 1;
	}
	red = 0;
	green = 0;
	blue = 0;
  }
  if (destFormat->bytesPerPixel >= 3) {
	int destExtraBytes = destFormat->bytesPerPixel - 3;
	if (destFormat->byteOrder == MSB_FIRST) {
      *dest++ = red;
      *dest++ = green;
      *dest++ = blue;
	}
	else {
      *dest++ = blue;
      *dest++ = green;
      *dest++ = red;
	}
	while (destExtraBytes-- > 0) {
      /* This is probably going to be interpreted as alpha or something.
       * We may need a different constant value, or even a variant value.
       */
      *dest++ = 0;
	}
  }
  else if (destFormat->bytesPerPixel == 2) {
#define SHIFT(a,b) ((b)>0?(a)<<(b):(a)>>-(b))
	unsigned long pixel = 
      (SHIFT(red, destFormat->redShift) & destFormat->redMask) |
      (SHIFT(green, destFormat->greenShift) & destFormat->greenMask) |
      (SHIFT(blue, destFormat->blueShift) & destFormat->blueMask);
	if (destFormat->byteOrder == LSB_FIRST) {
      *dest++ = pixel & 0xff;
      *dest++ = (pixel >> 8) & 0xff;
	}
	else {
      *dest++ = (pixel >> 8) & 0xff;
      *dest++ = pixel & 0xff;
	}
  }
  else if (destFormat->bytesPerPixel == 1) {
	/* maybe make this smarter later */
	static int forewarned = 0;
	if (!forewarned) {
      ERROR("unsuccessful conversion to 1 byte per pixel");
      forewarned = 1;
	}
	*dest++ = 0;
  }
  else {
	static int forewarned = 0;
	if (!forewarned) {
      ERROR("trying to convert to %d bytes per pixel?!?", 
		    destFormat->bytesPerPixel);
      forewarned = 1;
	}
  }
}

ImagePtr ConvertImageToFormat( ImagePtr image, ImageFormat *canvasFormat)
{
  /* We used to check for optimized cases; but any time we're
   * here, we're unoptimized, so now just do as complete a job
   * as possible, without worrying about efficiency.
   */
  DEBUGMSG("ConvertImageToFormat(frame %d)", image->frameNumber); 
  const ImageFormat *srcFormat = &image->imageFormat;
  const ImageFormat *destFormat = canvasFormat;

  const int srcBytesPerPixel = srcFormat->bytesPerPixel;
  const int destBytesPerPixel = destFormat->bytesPerPixel;

  const int srcBytesPerScanline = ROUND_UP_TO_MULTIPLE(
                                                    srcBytesPerPixel * image->width,
                                                    srcFormat->scanlineByteMultiple
                                                    );
  const int destBytesPerScanline = ROUND_UP_TO_MULTIPLE(
                                                     destBytesPerPixel * image->width,
                                                     destFormat->scanlineByteMultiple
                                                     );
  register const unsigned char *srcScanline, *src;
  register unsigned char *destScanline, *dest;
  register uint32_t x, y;

  /* Check to make sure that the image isn't good enough already. 
   * We have to check here, instead of earlier, because it's possible
   * for two formats with different "bytesPerScanline" values to 
   * be compatible, if the actual computed bytes per scanline happens
   * to match.
   */
  if (
      srcBytesPerPixel == destBytesPerPixel &&
      srcBytesPerScanline == destBytesPerScanline &&
      srcFormat->byteOrder == destFormat->byteOrder &&
      (srcFormat->rowOrder == destFormat->rowOrder ||
       destFormat->rowOrder == ROW_ORDER_DONT_CARE)
      ) {
    /* It's possible that we match - we need a closer look. */
    if (srcBytesPerPixel >= 3) {
      /* Match, as we ignore the extra bytes in a pixel */
      return image;
    }
      
    /* Less than 3 bytes per pixel - we match if our
     * shifts and masks all match.
     */
    if (srcFormat->redShift == destFormat->redShift &&
        srcFormat->greenShift == destFormat->greenShift &&
        srcFormat->blueShift == destFormat->blueShift &&
        srcFormat->redMask == destFormat->redMask &&
        srcFormat->greenMask == destFormat->greenMask &&
        srcFormat->blueMask == destFormat->blueMask)
      return image;
  }
  /* Otherwise, we suffer a non-trivial conversion. Create a new
   * image from scratch.
   */
  ImagePtr destImage(new Image()); 
  if (!destImage) {
	ERROR("could not allocate Image structure");
	return ImagePtr();
  }
  if (!destImage->allocate(image->height * destBytesPerScanline)) {
	ERROR("could not allocate %dx%dx%d image data",
          image->height, image->width, destBytesPerPixel*8);
	return ImagePtr();
  }

  if ((srcFormat->rowOrder != destFormat->rowOrder) && (destFormat->rowOrder != ROW_ORDER_DONT_CARE))
    srcScanline = (unsigned char *) image->ConstData() + (image->height - 1) * srcBytesPerScanline;
  else
    srcScanline = (unsigned char *) image->ConstData();

  /* XXX this code does not properly handle the loadedRegion info!!!
   * It was broken before and just removed for now.
   */
  destScanline = (unsigned char *) destImage->Data();
  for (y = 0; y < image->height; y++) {
	src = srcScanline;
	dest = destScanline;
	for (x = 0; x < image->width; x++) {
      ConvertPixel(srcFormat, destFormat, src, dest);
      src += srcBytesPerPixel;
      dest += destBytesPerPixel;
	}
    if ((srcFormat->rowOrder != destFormat->rowOrder) && (destFormat->rowOrder != ROW_ORDER_DONT_CARE))
      srcScanline -= srcBytesPerScanline;
    else
      srcScanline += srcBytesPerScanline;
	destScanline += destBytesPerScanline;
  }

  destImage->width = image->width;
  destImage->height = image->height;
  destImage->imageFormat = *canvasFormat;
  if (canvasFormat->rowOrder == ROW_ORDER_DONT_CARE)
    destImage->imageFormat.rowOrder = image->imageFormat.rowOrder;
  bb_assert(destImage->imageFormat.rowOrder != ROW_ORDER_DONT_CARE);
  destImage->loadedRegion = image->loadedRegion;
  DEBUGMSG("Done with ConvertImageToFormat"); 
  return destImage;
}

/*
 * Take the indicated source region (srcX, srcY, srcWidth, srcHeight) of
 * the image and return a new image of that region scaled to zoomeWidth
 * by zoomedHeight
 */
ImagePtr ScaleImage( ImagePtr image, int srcX, int srcY,
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
