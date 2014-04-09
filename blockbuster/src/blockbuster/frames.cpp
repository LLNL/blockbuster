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

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "util.h"
#include <libgen.h>
#include "frames.h"
#include "pnmFrame.h"
#include "pngFrame.h"
#include "tiffFrame.h"
#include "sgi-rgbFrame.h"
#include "smFrame.h"
#include "errmsg.h"
#include "settings.h"
#include <X11/Xlib.h>

static QString matchName; 

#ifdef _LARGEFILE64_SOURCE
#define MYSTAT stat64
#else
#define MYSTAT stat
#endif

//====================================================================
/* Note:  __APPLE__ assumes gcc on OS X*/
#if defined(irix) || defined(aix) 
static int matchStartOfName(struct dirent *entry)
#else
  static int matchStartOfName(const struct dirent *entry)
#endif
{
  /* RDC -- this is all that is needed.  The old version of this function called stat on every matching file, for no good reason I could tell.  Unfortunately, on a 32 bit OS with large files, this caused weird behavior, so I just pulled it out.  We're left with a very easy check  - check the name to see if it matches */
  return (QString(entry->d_name) == matchName); 
}


//====================================================================
void FrameInfo::ConvertPixel(const ImageFormat *srcFormat,
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

//====================================================================
ImagePtr FrameInfo::ConvertImageToFormat( ImagePtr image, ImageFormat *canvasFormat)
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

//===============================================================
/* This utility function handles loading and converting images appropriately,
 * for both the single-threaded and multi-threaded cases.
 */
#define MAX_CONVERSION_WARNINGS 10
ImagePtr FrameInfo::LoadAndConvertImage(ImageFormat *canvasFormat, 
                                        const Rectangle *region, 
                                        int levelOfDetail)
{
  DEBUGMSG(QString("LoadAndConvertImage frame %1, region %2, frameInfo %3").arg( mFrameNumber).arg(string(*region).c_str()).arg((uint64_t)this)); 

  int rv;
  static int conversionCount = 0;


  ImagePtr image(new Image()); 
  if (!image) {
	ERROR("Out of memory in LoadAndConvertImage");
	return image;
  }

  DEBUGMSG("LoadImage being called"); 

  // call derived LoadImage() function
  rv = LoadImage(image, canvasFormat,region, levelOfDetail);

  if (!rv ) {
	ERROR("could not load frame %d (frame %d of file name %s) for the cache",
          mFrameNumber,
          mFrameNumberInFile,
          mFilename.c_str()
          );
	return ImagePtr();
  }
  image->frameNumber = mFrameNumber; 
  DEBUGMSG("LoadImage done"); 


  /* The file format module, which loaded the image for us, tried to
   * do as good a job as it could to match the format our canvas
   * requires, but it might not have succeeded.  Try to convert
   * this image to the correct format.  If the same image comes
   * back, no conversion was necessary; if a NULL image comes back,
   * conversion failed.  If any other image comes back, we can
   * discard the original, and use the conversion instead.
   */
  ImagePtr convertedImage = ConvertImageToFormat(image, canvasFormat);
  if (convertedImage != image) {
    /* We either converted, or failed to convert; in either
     * case, the old image is useless.
     */  
    image = convertedImage;
    if (!image) {
      ERROR("failed to convert frame %d", mFrameNumber);
    }
    else {
      conversionCount++;
      if (conversionCount < MAX_CONVERSION_WARNINGS) {
        /* We'll issue a warning anyway, as conversion is an expensive
         * process.
         */
        WARNING("had to convert frame %d", mFrameNumber);
          
        if (conversionCount +1 == MAX_CONVERSION_WARNINGS) {
          WARNING("(suppressing further conversion warnings)");
        }
      }
    }
  }

  /* If we have a NULL image, the converter already reported it. */
  DEBUGMSG("Done with LoadAndConvertImage, frame %d", mFrameNumber); 
  return image;
}




//===============================================================

 
/*
 * Given a list of filenames, return a FrameList describing the movie
 * frames.
 * Input: fileCount - length of files array
 * Input: files - array of filenames
 * Input: settings - a place to store recent files
 * Global variable used: fileFormats - list of acceptable file formats
 * Return: true if files were loaded, else false
 */
bool FrameList::LoadFrames(QStringList &files) {
  DEBUGMSG("LoadFrameList()");
  int32_t index;
  
  char  *filename, directory[BLOCKBUSTER_PATH_MAX], base[BLOCKBUSTER_PATH_MAX]; 
  for (index = 0; index < files.size(); index++) {
    filename = strdup(MakeAbsolutePath(files[index]).toStdString().c_str()); 
    DEBUGMSG(QString("Checking for file %1").arg( filename)); 
    strncpy(base, basename(filename), strlen(filename)); 
    filename = strdup(MakeAbsolutePath(files[index]).toStdString().c_str()); 
    strncpy(directory, dirname(filename), strlen(filename));; 
    filename = strdup(MakeAbsolutePath(files[index]).toStdString().c_str()); 
    
    QString recentFileName;
    int numFiles;
    struct dirent **fileList;
    int i;
       
    /* Get a list of files that match the given filename; these represent
     * our movie files.  Each can be a movie of itself, or a still image.
     */
    matchName = base; // matchName is a static variable
    /* Special case for the sm network based files.  No directory,
     * just a name prefixed by "dfc:".
     */
    if (QString(base).startsWith("dfc:")) {
      numFiles = 1;
      fileList = new struct dirent *;
      fileList[0] = new struct dirent;
      strcpy(fileList[0]->d_name,base);
      directory[0] = '\0';
    } 
    else if ((numFiles = scandir(directory,
                                 &fileList, 
                                 matchStartOfName, alphasort)) < 0){
      ERROR(QString("Error scanning directory '%1': %2").arg(directory ).arg(strerror(errno)));
      return false;
    }
    else if (numFiles == 0) {
      ERROR(QString("no files match '%1'.  Skipping...").arg(filename));
      return false;
    }
    
    /* One or more files matched - we're happy as clams.  Now the
     * fun and exciting part: each file may include one or more
     * frames (movie files in particular can include multiple frames).
     */
    /*   recentFileName = QString("%1/%2").arg(directory).arg(base);
         SetRecentFileSetting((void*)settings, recentFileName);
    */ 
    for (i = 0; i < numFiles; i++) {
      QString matchedFileName = 
        QString("%1/%2").arg(directory).arg(fileList[i]->d_name);;
      DEBUGMSG(QString("Matched file name '%1'").arg( matchedFileName));
      
      FrameListPtr frameListFromFile;
           
      /* Search for a format driver that can read the matched file. */
      frameListFromFile = smGetFrameList(matchedFileName.toStdString().c_str()); 
      if (!frameListFromFile) {
        frameListFromFile = tiffGetFrameList(matchedFileName.toStdString().c_str()); 
      }
      if (!frameListFromFile) {
        frameListFromFile = pnmGetFrameList(matchedFileName.toStdString().c_str()); 
      }
      if (!frameListFromFile) {
        frameListFromFile = pngGetFrameList(matchedFileName.toStdString().c_str()); 
      }
      if (!frameListFromFile) {
        frameListFromFile = sgirgbGetFrameList(matchedFileName.toStdString().c_str()); 
      }
      /* See if any of the formats matched */
      if (frameListFromFile) {
        /* Spit out a little information about the matched
         * frames.  
         */
        DEBUGMSG(QString("Matched format '%1' with file '%2'").
                 arg(frameListFromFile->mFormatName).arg(matchedFileName));
        /* This operation will dump more information about the
         * matched frames, and will destroy frameListFromFile
         */
        //allFrames = AppendFrameList(allFrames, frameListFromFile);
        append(frameListFromFile); 
        if (i==0) {
          DEBUGMSG(QString("Using FPS and stereo settings from first file '%1'").
                   arg(matchedFileName));
          mStereo = frameListFromFile->mStereo; 
          mTargetFPS = frameListFromFile->mTargetFPS; 
          mWidth =frameListFromFile->mWidth; 
          mHeight = frameListFromFile->mHeight; 
          mDepth = frameListFromFile->mDepth; 
        } else if (frameListFromFile->mWidth != mWidth || 
                  frameListFromFile->mHeight != mHeight || 
                  frameListFromFile->mDepth != mDepth ) {
          ERROR(QString("Error: dimensions of file \"%1\" does not match with previous files in file set.").
                arg(matchedFileName));
          return false; 
        }
     }
      else {
        /* No format was able to open the file.  Give an error. */
        ERROR(QString("Error: no known format could open the file '%1'")
              .arg(matchedFileName));
        return false; 
      }
      
    } /* loop scanning all file names that match a single argument */
    
    /* Done with this argument; free up the matching file list and
     * other associated variables.
     */
    for (i = 0; i < numFiles; i++) {
      delete fileList[i];
    }
    delete fileList; 
  }
  
  return mFrames.size()>0;
}


