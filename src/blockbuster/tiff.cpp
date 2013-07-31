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
#include "frames.h"
#include "util.h"
#include "frames.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "errmsg.h"
#include <errno.h> 
#include <X11/Xlib.h>
#include "tiff.h"

#define int16 myint16
#define int32 myint32
#include <tiffio.h>
#undef int16
#undef int32



// ==================================================================
/* Use the TIFFRGBAImage facilities to load any supported image */
/* image -- output image
   all other params are input parameters
*/ 
static int RGBALoadImage(Image *image,  FrameInfo *frameInfoPtr,
                         ImageFormat *requiredImageFormat, const Rectangle *,
                         int levelOfDetail)
{
  TIFF *f;
  register uint32_t i, j, k;
  uint32_t extraBytesPerPixel, extraBytesPerScanline;
  int scanlineBytes;
   TiffFrameInfo *frameInfo = reinterpret_cast<TiffFrameInfo *>(frameInfoPtr); 
   /*   
  TiffFrameInfo *tfip = reinterpret_cast<TiffFrameInfo *>(frameInfoPtr.get()); 
  TiffFrameInfoPtr frameInfo(frameInfoPtr, tfip); 
   */
  if (!frameInfo) {
    ERROR("programmer mistake:  FrameInfoPtr could not be recast to TiffFrameInfoPtr\n"); 
    return 0; 
  }

  register unsigned char *dest;
  TIFFRGBAImage rgbaImg;
  char errMesg[1024];
  int rc;

  bb_assert(image);

  printf("RGBALoadImage CALLED\n");

  /* Calculate how much image data we need. */
  extraBytesPerPixel = requiredImageFormat->bytesPerPixel - 3;
  scanlineBytes = ROUND_TO_MULTIPLE(
                                    requiredImageFormat->bytesPerPixel * frameInfo->width,
                                    requiredImageFormat->scanlineByteMultiple
                                    );
  extraBytesPerScanline = scanlineBytes - 
    requiredImageFormat->bytesPerPixel * frameInfo->width;


  if (!image->imageData) {
	if (requiredImageFormat->rowOrder == ROW_ORDER_DONT_CARE)
      image->imageFormat.rowOrder = BOTTOM_TO_TOP; /* Bias toward OpenGL */
	else
      image->imageFormat.rowOrder = requiredImageFormat->rowOrder;

	/* The Canvas gets the opportunity to allocate image data first. */
	image->imageData = calloc(1, frameInfo->height * scanlineBytes);
	if (image->imageData == NULL) {
      ERROR("could not allocate %dx%dx%d image data",
            frameInfo->width, frameInfo->height, frameInfo->depth);
      return 0;
	}
	image->imageDataBytes = frameInfo->height * scanlineBytes;

	/* We sent only the necessary rows, but all pixels in each row.
	 * So tell this to the caller.
	 */
	image->width = frameInfo->width;
	image->height = frameInfo->height;
	image->imageFormat.bytesPerPixel = requiredImageFormat->bytesPerPixel;
	image->imageFormat.scanlineByteMultiple = requiredImageFormat->scanlineByteMultiple;
	image->imageFormat.byteOrder = requiredImageFormat->byteOrder;
	image->levelOfDetail = levelOfDetail;
  }

  f = TIFFOpen(frameInfo->filename.c_str(), "r");
  if (f == NULL) {
	SYSERROR("cannot open TIFF file %s", frameInfo->filename.c_str());
    if (image->imageData) free(image->imageData);
	image->imageData = NULL;
	image->imageDataBytes = 0;
	return 0;
  }

  /* Here we worry about adding or stripping alpha bytes and
   * switching RGB to BGR.  There's almost certainly a faster
   * way to do this with the TIFF RGBA Image utilities;
   * but I haven't yet figured out the intricacies of the
   * "put" functions.
   */

  /*
   * NOTE: We generally can't read a sub-image unless the tiff is
   * uncompressed.  Read whole image.
   */
  memset( errMesg, 0, 1024 );
  rc = TIFFRGBAImageBegin( &rgbaImg, f, 0, errMesg);
  if( rc != 1 ) {
	WARNING("TIFFRGBAImageBegin() failed: %s.1024", errMesg);
	TIFFClose( f );
	return 0;
  }
  uint32* raster = (uint32*)(&frameInfo->scanlineBuffer[0]);
  dest = (unsigned char *)image->imageData;
  for (i = 0; i < image->height; i++) {
	const int row = /*desiredSub->y +*/ i;
	int rv;
	
	/* If we're going from bottom to top, put the scanline at the bottom;
	 * otherwise, it should still be well-placed for what we need.
	 */
	if (image->imageFormat.rowOrder == BOTTOM_TO_TOP) {
      dest = (unsigned char *)image->imageData + (frameInfo->height - row - 1) * scanlineBytes;
	}
    else {
      bb_assert(image->imageFormat.rowOrder == TOP_TO_BOTTOM);
      dest = (unsigned char *)image->imageData + row * scanlineBytes;
    }

    /* Use width 1 to get a single full width scanline */
	rv = TIFFRGBAImageGet( &rgbaImg, raster, frameInfo->width, 1);
	if (rv != 1) {
      WARNING("TIFFRGBAImageGet() row=%d i=%d returned %d", row, i, rv);
      TIFFClose( f );
      TIFFRGBAImageEnd( &rgbaImg );
      return 0;
	}

    /* Move down a row for next ImageGet call */
	rgbaImg.row_offset++;

	for (j = 0; j < static_cast<uint32_t>(frameInfo->width); j++) {
      unsigned char red, green, blue;
      red = TIFFGetR( raster[j] );
      green = TIFFGetG( raster[j] );
      blue = TIFFGetB( raster[j] );

      if (requiredImageFormat->byteOrder == MSB_FIRST) {
		*dest++ = red;
		*dest++ = green;
		*dest++ = blue;
      }
      else {
		*dest++ = blue;
		*dest++ = green;
		*dest++ = red;
      }
      for (k = 0; k < extraBytesPerPixel; k++) {
		*dest++ = 0xff;
      }
	}
	dest += extraBytesPerScanline;
  }
  TIFFRGBAImageEnd( &rgbaImg );

  TIFFClose(f);

  image->loadedRegion.x = 0;
  image->loadedRegion.y = 0;
  image->loadedRegion.width = frameInfo->width;
  image->loadedRegion.height = frameInfo->height;

  return 1;
}


/* Load the desired subimage into a set of RGB bytes */
/* This image loader function is installed if the TIFF file
 * contains 8-bit samples and 3 samples per pixel.
 */
static int
Color24LoadImage(Image *image,  FrameInfo* frameInfoPtr,
                 ImageFormat *requiredImageFormat, const Rectangle *,
                 int levelOfDetail)
{
  TIFF *f;
  /*
  TiffFrameInfo *tfip = reinterpret_cast<TiffFrameInfo *>(frameInfoPtr.get()); 
  TiffFrameInfoPtr frameInfo(frameInfoPtr, tfip); 
  */
  TiffFrameInfo *frameInfo = reinterpret_cast<TiffFrameInfo *>(frameInfoPtr); 

  if (!frameInfo) {
    ERROR("programmer mistake:  FrameInfoPtr could not be recast to TiffFrameInfoPtr\n"); 
    return 0; 
  }
  register uint32_t i, j, k;
  int extraBytesPerPixel, extraBytesPerScanline;
  int scanlineBytes;
  register unsigned char *dest, *src;

  bb_assert(image);

  /* Calculate how much image data we need. */
  extraBytesPerPixel = requiredImageFormat->bytesPerPixel - 3;
  scanlineBytes = ROUND_TO_MULTIPLE(
                                    requiredImageFormat->bytesPerPixel * frameInfo->width,
                                    requiredImageFormat->scanlineByteMultiple
                                    );
  extraBytesPerScanline = scanlineBytes - 
    requiredImageFormat->bytesPerPixel * frameInfo->width;


  if (!image->imageData) {
	if (requiredImageFormat->rowOrder == ROW_ORDER_DONT_CARE)
      image->imageFormat.rowOrder = BOTTOM_TO_TOP; /* Bias toward OpenGL */
	else
      image->imageFormat.rowOrder = requiredImageFormat->rowOrder;

	/* The Canvas gets the opportunity to allocate image data first. */
	image->imageData = calloc(1, frameInfo->height * scanlineBytes);
	if (image->imageData == NULL) {
      ERROR("could not allocate %dx%dx%d image data ",
            frameInfo->width, frameInfo->height, frameInfo->depth);
      return 0;
	}
	image->imageDataBytes = frameInfo->height * scanlineBytes;

	/* We sent only the necessary rows, but all pixels in each row.
	 * So tell this to the caller.
	 */
	image->width = frameInfo->width;
	image->height = frameInfo->height;
	image->imageFormat.bytesPerPixel = requiredImageFormat->bytesPerPixel;
	image->imageFormat.scanlineByteMultiple = requiredImageFormat->scanlineByteMultiple;
	image->imageFormat.byteOrder = requiredImageFormat->byteOrder;
	image->levelOfDetail = levelOfDetail;
  }

  f = TIFFOpen(frameInfo->filename.c_str(), "r");
  if (f == NULL) {
	SYSERROR("cannot open TIFF file %s", frameInfo->filename.c_str());
	if (image->imageData) free(image->imageData);
	image->imageData = NULL;
	image->imageDataBytes = 0;
	return 0;
  }

  /* Here we worry about adding or stripping alpha bytes and
   * switching RGB to BGR.  There's almost certainly a faster
   * way to do this with the TIFF RGBA Image utilities;
   * but I haven't yet figured out the intricacies of the
   * "put" functions.
   */

  /*
   * NOTE: We generally can't read a sub-image unless the tiff is
   * uncompressed.  Read whole image.
   */
  dest = (unsigned char *)image->imageData;
  for (i = 0; i < image->height; i++) {
	const int row = /*desiredSub->y +*/ i;
	int rv;
	src = &frameInfo->scanlineBuffer[0];
	/* If we're going from bottom to top, put the scanline at the bottom;
	 * otherwise, it should still be well-placed for what we need.
	 */
	if (image->imageFormat.rowOrder == BOTTOM_TO_TOP) {
      dest = (unsigned char *)image->imageData + (frameInfo->height - row - 1) * scanlineBytes;
	}
    else {
      bb_assert(image->imageFormat.rowOrder == TOP_TO_BOTTOM);
      dest = (unsigned char *)image->imageData + row * scanlineBytes;
    }
	rv = TIFFReadScanline(f, src, row, 0);
	if (rv != 1) {
      WARNING("TIFFReadScanline(row=%d) i=%d returned %d", row, i, rv);
      return 0;
	}
	for (j = 0; j < static_cast<uint32_t>(frameInfo->width); j++) {
      unsigned char red, green, blue;
      red = *src++;
      green = *src++;
      blue = *src++;

      if (requiredImageFormat->byteOrder == MSB_FIRST) {
		*dest++ = red;
		*dest++ = green;
		*dest++ = blue;
      }
      else {
		*dest++ = blue;
		*dest++ = green;
		*dest++ = red;
      }
      for (k = 0; k < static_cast<uint32_t>(extraBytesPerPixel); k++) {
		*dest++ = 0xff;
      }
	}
	dest += extraBytesPerScanline;
  }

  TIFFClose(f);

  image->loadedRegion.x = 0;
  image->loadedRegion.y = 0;
  image->loadedRegion.width = frameInfo->width;
  image->loadedRegion.height = frameInfo->height;

  return 1;
}


FrameList *tiffGetFrameList(const char *filename)
{
  TIFF *f;
  FrameList *frameList;
  char errMesg[1024];

  /* Values used with TIFFGetField have to be of the specific
   * correct types, as the function is a varargs function
   * and expects these types.
   */
  uint16 bitsPerSample, samplesPerPixel, photometric, planarConfiguration;
  uint32 width, height;
  double minSample = 0.0, maxSample = 0.0;

  /* Override the TIFF error handler, to keep it from reporting error
   * messages that we wish to control (e.g., an error in TIFFOpen()
   * that says a file isn't a TIFF file).
   */
  TIFFSetErrorHandler(NULL);

  f = TIFFOpen(filename, "r");
  if (!f) {
	DEBUGMSG("The file '%s' is not a TIFF file.", filename);
	return NULL;
  }
  if (!TIFFGetField(f, TIFFTAG_BITSPERSAMPLE, &bitsPerSample))
	bitsPerSample = 1;
  if (!TIFFGetField(f, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel))
	samplesPerPixel = 1;
  (void) TIFFGetField(f, TIFFTAG_IMAGEWIDTH, &width);
  (void) TIFFGetField(f, TIFFTAG_IMAGELENGTH, &height);
  (void) TIFFGetField(f, TIFFTAG_PHOTOMETRIC, &photometric);
  (void) TIFFGetField(f, TIFFTAG_PLANARCONFIG, &planarConfiguration);
  (void) TIFFGetFieldDefaulted(f, TIFFTAG_MINSAMPLEVALUE, &minSample);
  (void) TIFFGetFieldDefaulted(f, TIFFTAG_MAXSAMPLEVALUE, &maxSample);

  DEBUGMSG(
           "%s is a %dx%d TIFF [%d] file with %d bits per sample and %d samples per pixel",
           filename, width, height, planarConfiguration, bitsPerSample, samplesPerPixel);
  DEBUGMSG("scanline size %d", TIFFScanlineSize(f));

  TIFFClose(f);

  /* Prepare the FrameList and FrameInfo structures we are to
   * return to the user.  Since a PNG file stores a single 
   * frame, we need only one frameInfo, and the frameList
   * need be large enough only for 2 entries (the information
   * about the single frame, and the terminating NULL).
   */
  TiffFrameInfoPtr frameInfo(new TiffFrameInfo()); 
  if (!frameInfo) {
	ERROR("cannot allocate FrameInfo structure");
	return NULL;
  }

  frameInfo->filename = filename;
    
  frameList = new FrameList(); 
  if (frameList == NULL) {
    ERROR("cannot allocate FrameInfo list structure");
    return NULL;
  }
    
  /* Choose an image loader function based on the type of image
   * we have.
   */
  if (bitsPerSample == 8 && samplesPerPixel == 3) {
	/* 24-bit color image */
	frameInfo->LoadImage = Color24LoadImage;
	/* Each scan line will hold 3 bytes per pixel */
    frameInfo->scanlineBuffer.resize(width * 3);
  } else if ( TIFFRGBAImageOK( f, errMesg ) ) {
	frameInfo->LoadImage = RGBALoadImage;
	/* Each scan line will hold 3 bytes per pixel */
    frameInfo->scanlineBuffer.resize(width * sizeof(uint32));
  }
#if 0
  else if (bitsPerSample == 16 && samplesPerPixel == 3) {
	/* 48-bit color image!?! */
  }
  else if (bitsPerSample == 8 && samplesPerPixel == 1) {
	/* 8-bit grayscale */
  }
  else if (bitsPerSample == 16 && samplesPerPixel == 1) {
	/* 16-bit grayscale */
  }
#endif
  else {
	WARNING(
            "%s: unsupported %d bps/%d spp TIFF file",
            filename, bitsPerSample, samplesPerPixel);
	delete frameList;
	return NULL;
  }
    
  /* Fill out the rest of the frameInfo information */
  frameInfo->width = width;
  frameInfo->height = height;
  frameInfo->depth = 8*samplesPerPixel;
  frameInfo->mFrameNumberInFile = 0;
  frameInfo->enable = 1;

  frameInfo->photometric = photometric;
  frameInfo->minSample = minSample;
  frameInfo->maxSample = maxSample;
  frameInfo->bitsPerSample = bitsPerSample;
  frameInfo->samplesPerPixel = samplesPerPixel;

  /* Fill out the final return form, and call it a day */
  frameList->append(frameInfo); 
  frameList->targetFPS = 0.0;

  frameList->formatName = "TIFF";
  frameList->formatDescription = "Single-frame image in a TIFF file";
  return frameList;
}

