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
#include <string.h>
#include <pngFrame.h>
#include "errmsg.h"
#include "util.h"
#include <X11/Xlib.h>

/* A guess as to the gamma exponent for our display.  It'll
 * be combined with any gamma exponent in the file.
 */
#define DISPLAY_EXPONENT 2.2

// ===============================================================
int PNGFrameInfo::PrepPng(string filename, 
                          FILE **fPtr, png_structp *readStructPtr, 
                          png_infop *infoStructPtr) {
#define SIGNATURE_SIZE 8
  unsigned char signature[SIGNATURE_SIZE];
  FILE *f;
  png_structp readStruct;
  png_infop infoStruct;
  int rv;

  /* Try to open the given file */
  f = fopen(filename.c_str(), "rb");
  if (f == NULL) {
	WARNING("cannot open file '%s'", filename.c_str());
	return 0;
  }

  /* Make sure the file represents a real PNG image. */
  fread(signature, 1, SIGNATURE_SIZE, f);
  if (!png_check_sig(signature, SIGNATURE_SIZE)) {
	DEBUGMSG("PNG signature check failed for filename %s", filename.c_str());
	fclose(f);
	return 0;
  }
    
  readStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                      NULL, NULL, NULL);
  if (readStruct == NULL) {
	ERROR("cannot allocate PNG read structure");
	fclose(f);
	return 0;
  }

  /* And the PNG-defined info structure */
  infoStruct = png_create_info_struct(readStruct);
  if (infoStruct == NULL) {
	ERROR("cannot allocate PNG info structure");
	png_destroy_read_struct(&readStruct, &infoStruct, NULL);
	fclose(f);
	return 0;
  }

#ifdef PNG_SETJMP_SUPPORTED
  /* The PNG library uses a setjmp-style of error reporting.
   * When setjmp is called initially, it will return a value
   * of 0.  If any error is discovered, the code will jump
   * back to the setjmp position, but with a non-zero return
   * value reported for setjmp.  If, then, we get a non-zero
   * setjmp value, we have to clean up everything.
   */
  rv = setjmp(png_jmpbuf(readStruct));
  if (rv != 0) {
	ERROR("libpng error: setjmp returned %d", rv);
	png_destroy_read_struct(&readStruct, &infoStruct, NULL);
	fclose(f);
	return 0;
  }
#endif

  /* Set up to read from the file pointer, tell the PNG
   * library that we've already read the signature, and
   * read the rest of the provided info.
   */
  png_init_io(readStruct, f);
  png_set_sig_bytes(readStruct, SIGNATURE_SIZE);
  png_read_info(readStruct, infoStruct);

  /* Return the pointers we've gathered so far. */
  *fPtr = f;
  *readStructPtr = readStruct;
  *infoStructPtr = infoStruct;

  return 1;
}

// ===============================================================
/* Load the desired subimage into a set of RGB bytes */
/*int PNGFrameInfo::LoadImage(ImageFormat *requiredImageFormat, 
  const Rectangle *desiredSubregion, 
  int levelOfDetail) {
*/
int PNGFrameInfo::LoadImage(ImagePtr mImage,
                             ImageFormat *requiredImageFormat, 
                             const Rectangle */*desiredSubregion*/, 
                             int levelOfDetail) {

  png_structp readStruct;
  png_infop infoStruct;
  int rv;
  FILE *f;
  int depth, colorType, rowBytes;
  double gamma;
  int scanlineBytes;
  int bytesPerPixel = 3;
  int byteMultiple = 1;
  int byteOrder = MSB_FIRST;

  /* We know how to handle 3 bytes per pixel
   * and 4 bytes per pixel, with any scanlineByteMultiple.  If these are
   * requested, we'll use them.
   *
   * Otherwise, we're going to return 3 bytes per pixel with no pad, and
   * leave it up to the main program to convert to the desired format.
   *
   * We always ignore the desiredSub* parameters, because this module has
   * no idea how to read a subimage of a PNG.  We'll always return the whole
   * image and let the renderer figure it out.
   */
  if (requiredImageFormat->bytesPerPixel == 3 || 
      requiredImageFormat->bytesPerPixel == 4) {
	bytesPerPixel = requiredImageFormat->bytesPerPixel;
	byteMultiple = requiredImageFormat->scanlineByteMultiple;
	byteOrder = requiredImageFormat->byteOrder;
  }

  scanlineBytes = ROUND_UP_TO_MULTIPLE(
                                    bytesPerPixel * mWidth,
                                    byteMultiple
                                    );

  if (!mImage->allocate(mHeight * scanlineBytes)) {
    ERROR("could not allocate %dx%dx%d image data",
          mWidth, mHeight, bytesPerPixel);
    return 0;
  }
    
    
  mImage->width = mWidth;
  mImage->height = mHeight;
  mImage->imageFormat.bytesPerPixel = bytesPerPixel;
  mImage->imageFormat.scanlineByteMultiple = byteMultiple;
  mImage->imageFormat.byteOrder = byteOrder;
  if (requiredImageFormat->rowOrder == ROW_ORDER_DONT_CARE)
    mImage->imageFormat.rowOrder = BOTTOM_TO_TOP; /* Bias for OpenGL */
  else
    mImage->imageFormat.rowOrder = requiredImageFormat->rowOrder;
    
  mImage->levelOfDetail = levelOfDetail;
    
 
  /* PNG requires an array of row pointers.  Allocate the array
   */
  png_bytep *rowPointers = new png_bytep[mHeight]; 
  if (rowPointers == NULL) {
    ERROR("could not allocate row pointers for frame");
    return 0;
  }
  bb_assert(scanlineBytes > 0);
  for (uint32_t i = 0; i < mHeight; i++) {
	if (mImage->imageFormat.rowOrder == TOP_TO_BOTTOM) {
      rowPointers[i] = (png_bytep) mImage->Data() + i * scanlineBytes;
	}
	else {
      rowPointers[mHeight - i - 1] = (png_bytep) mImage->Data() + i * scanlineBytes;
	}
  }

  if (!PrepPng(mFilename.c_str(), &f, &readStruct, &infoStruct)) {
	/* Error has already been reported */
	delete rowPointers;
	return 0;
  }

#ifdef PNG_SETJMP_SUPPORTED
  /* The PNG library uses a setjmp-style of error reporting.
   * When setjmp is called initially, it will return a value
   * of 0.  If any error is discovered, the code will jump
   * back to the setjmp position, but with a non-zero return
   * value reported for setjmp.  If, then, we get a non-zero
   * setjmp value, we have to clean up everything.
   */
  rv = setjmp(png_jmpbuf(readStruct));
  if (rv != 0) {
	ERROR("libpng error: setjmp returned %d", rv);
	png_destroy_read_struct(&readStruct, &infoStruct, NULL);
	delete rowPointers;
	fclose(f);
	return 0;
  }
#endif

  /* Get some info */
  png_get_IHDR(readStruct, infoStruct, 
               NULL, NULL, &depth, &colorType, 
               NULL, NULL, NULL);

  /* Ensure that we always read RGB information, without
   * alpha, with 8 bits of red, green, and blue, gamma corrected
   * if there's a gamma in the file.
   */
  png_set_expand(readStruct);
  if (depth == 16)
	png_set_strip_16(readStruct);
  if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
	png_set_gray_to_rgb(readStruct);
  if (colorType == PNG_COLOR_TYPE_GRAY_ALPHA || colorType == PNG_COLOR_TYPE_RGB_ALPHA)
	png_set_strip_alpha(readStruct);
  if (png_get_gAMA(readStruct, infoStruct, &gamma)) 
	png_set_gamma(readStruct, DISPLAY_EXPONENT, gamma);
  if (bytesPerPixel == 4)
	png_set_filler(readStruct, 0xff, PNG_FILLER_AFTER);
  /* Our byte order is LSB_FIRST only if we're reading a 3-byte or 4-byte
   * image and the receiver wants LSB_FIRST.  In this case, we need to
   * send BGR bytes down, not RGB.
   */
  if (byteOrder == LSB_FIRST)
	png_set_bgr(readStruct);

  /* With transformations registered, see if our sizes are expected */
  png_read_update_info(readStruct, infoStruct);
  rowBytes = png_get_rowbytes(readStruct, infoStruct);
  if (rowBytes != scanlineBytes) {
	WARNING("got row bytes of %d, expected %d",
            rowBytes, scanlineBytes);
  }

  /* Read in the whole image.  This will put it directly into the data area. */
  png_read_image(readStruct, rowPointers);

  /* All done.  Clean up the PNG structures, return the format we've
   * chosen, and go home.
   */
  png_destroy_read_struct(&readStruct, &infoStruct, NULL);

  fclose(f);
  delete rowPointers;

  /* Since we're reading the whole
   * image, our offset is at (0,0), and our width and height
   * are the width and height of the whole image.  Had we been
   * able to read a portion of the image, we would have set
   * these values appropriately.
   */
  mImage->loadedRegion.x = 0;
  mImage->loadedRegion.y = 0;
  mImage->loadedRegion.height = mHeight;
  mImage->loadedRegion.width = mWidth;

  return 1;
}

// ======================================================================

PNGFrameInfo::PNGFrameInfo(string fname): FrameInfo(fname)  {
  FILE *f;
  png_structp readStruct;
  png_infop infoStruct;
  int pngdepth, colorType, interlaceType, compressionType, filterMethod;
  png_uint_32 pngwidth, pngheight; 

  /* The info is all read; this routine extracts the info
   * from the structures that already store it.
   */

  /* Start reading the file straight away */
  if (!PrepPng(mFilename, &f, &readStruct, &infoStruct)) {
    /* Error has already been reported */
    return ;
  }
 
  png_get_IHDR(readStruct, infoStruct, 
               &pngwidth, &pngheight, &pngdepth, &colorType, 
               &interlaceType, &compressionType, &filterMethod);
  
  /* Done with all the reading we're going to do.  Close up the
   * file and destroy the structures we allocated to help with
   * the reading.
   */
  png_destroy_read_struct(&readStruct, &infoStruct, NULL);
  fclose(f);
  
  /* Bit depth is interesting - the information returned in
   * "depth" is the depth of *one* channel.  The returned color
   * type indicates which channels are present.  In all cases we
   * want to return the total bit depth required to show the image.
   * Grayscales are converted to RGB; alpha channels are discarded.
   */
  switch (colorType) {
  case PNG_COLOR_TYPE_GRAY:
  case PNG_COLOR_TYPE_GRAY_ALPHA:
  case PNG_COLOR_TYPE_RGB:
  case PNG_COLOR_TYPE_RGB_ALPHA:
    mDepth = pngdepth * 3; 
    break;
  case PNG_COLOR_TYPE_PALETTE:
    mDepth = pngdepth;
    break;
  default:
    WARNING("Unrecognized PNG color type %d ignored.", colorType);
    return; 
  }
  mWidth = pngwidth; 
  mHeight = pngheight; 

  DEBUGMSG("The file '%s' is a valid PNG file.", mFilename.c_str());

  mValid = true; 
  return; 
}

FrameListPtr pngGetFrameList(const char *filename)
{
  FrameListPtr frameList; 

   
  /* Prepare the FrameList and FrameInfo structures we are to
   * return to the user.  Since a PNG file stores a single 
   * frame, we need only one frameInfo, and the frameList
   * need be large enough only for 2 entries (the information
   * about the single frame, and the terminating NULL).
   */
  FrameInfoPtr frameInfo(new PNGFrameInfo(filename)); 
  if (!frameInfo) {
    ERROR("cannot allocate FrameInfo structure");
    return frameList;
  }
    
  if (!frameInfo->mValid) {
    return frameList; 
  }

  frameList.reset(new FrameList(frameInfo)); 
  if (!frameList) {
    ERROR("cannot allocate FrameInfo list structure");
    return frameList;
  }
    
  if (frameList) {
    
    /* Fill out the final return form, and call it a day */
    frameList->append(frameInfo);
    frameList->mTargetFPS = 0.0;
    
    frameList->mFormatName = "PNG"; 
    frameList->mFormatDescription = "Single-frame image in a PNG file"; 
  }
  return frameList;
}

