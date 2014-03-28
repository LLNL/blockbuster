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
#include <errno.h>
#include <string.h>
#include "util.h"
#include "errmsg.h"
#include "sgi-rgbFrame.h"

/*
 * The following code comes from old SGI demos
 */

typedef struct _rawImageRec {
  unsigned char imagic[2];
  unsigned short type;
  unsigned short dim;
  unsigned short sizeX, sizeY, sizeZ;
  unsigned long min, max;
  unsigned long wasteBytes;
  char name[80];
  unsigned long colorMap;
  FILE *file;
  unsigned char *tmp, *tmpR, *tmpG, *tmpB, *tmpA;
  unsigned long rleEnd;
  unsigned int *rowStart;
  int *rowSize;
} rawImageRec;

static void ConvertShort(unsigned short *array, long length)
{
  unsigned long b1, b2;
  unsigned char *ptr;

  ptr = (unsigned char *)array;
  while (length--) {
    b1 = *ptr++;
    b2 = *ptr++;
    *array++ = (unsigned short) ((b1 << 8) | (b2));
  }
}

static void ConvertLong(unsigned int *array, long length)
{
  unsigned long b1, b2, b3, b4;
  unsigned char *ptr;

  ptr = (unsigned char *)array;
  while (length--) {
    b1 = *ptr++;
    b2 = *ptr++;
    b3 = *ptr++;
    b4 = *ptr++;
    *array++ = (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4);
  }
}

static rawImageRec *RawImageOpen(const char *fileName)
{
  union {
    int testWord;
    char testByte[4];
  } endianTest;
  rawImageRec *raw;
  int swapFlag;
  int x;

  endianTest.testWord = 1;
  if (endianTest.testByte[0] == 1) {
    swapFlag = 1;
  } else {
    swapFlag = 0;
  }

  raw = (rawImageRec *)malloc(sizeof(rawImageRec));
  if (raw == NULL) {
    dbprintf(0, "Out of memory!\n");
    return NULL;
  }
  if ((raw->file = fopen(fileName, "rb")) == NULL) {
    perror(fileName);
    return NULL;
  }

  fread(raw, 1, 12, raw->file);

  if (raw->imagic[0] != 0x1 || raw->imagic[1] != 0xda) {
    /* This is not an SGI RGB file */
    fclose(raw->file);
    return NULL;
  }

  if (swapFlag) {
    ConvertShort((unsigned short *) &raw->imagic, 6);
  }

  raw->tmp = (unsigned char *)malloc(raw->sizeX*256);
  raw->tmpR = (unsigned char *)malloc(raw->sizeX*256);
  raw->tmpG = (unsigned char *)malloc(raw->sizeX*256);
  raw->tmpB = (unsigned char *)malloc(raw->sizeX*256);
  raw->tmpA = (unsigned char *)malloc(raw->sizeX*256);
  if (raw->tmp == NULL || raw->tmpR == NULL || raw->tmpG == NULL ||
      raw->tmpB == NULL) {
    dbprintf(0, "Out of memory!\n");
    return NULL;
  }

  if ((raw->type & 0xFF00) == 0x0100) {
    x = raw->sizeY * raw->sizeZ * sizeof(unsigned int);
    raw->rowStart = (unsigned int *)malloc(x);
    raw->rowSize = (int *)malloc(x);
    if (raw->rowStart == NULL || raw->rowSize == NULL) {
      dbprintf(0, "Out of memory!\n");
      return NULL;
    }
    raw->rleEnd = 512 + (2 * x);
    fseek(raw->file, 512, SEEK_SET);
    fread(raw->rowStart, 1, x, raw->file);
    fread(raw->rowSize, 1, x, raw->file);
    if (swapFlag) {
      ConvertLong(raw->rowStart, (long) (x/sizeof(unsigned int)));
      ConvertLong((unsigned int *)raw->rowSize, (long) (x/sizeof(int)));
    }
  }
  return raw;
}

static void RawImageClose(rawImageRec *raw)
{

  fclose(raw->file);
  free(raw->tmp);
  free(raw->tmpR);
  free(raw->tmpG);
  free(raw->tmpB);
  free(raw->tmpA);
  free(raw);
}

static void RawImageGetRow(rawImageRec *raw, unsigned char *buf, int y, int z)
{
  unsigned char *iPtr, *oPtr, pixel;
  int count, done = 0;

  if ((raw->type & 0xFF00) == 0x0100) {
    fseek(raw->file, (long) raw->rowStart[y+z*raw->sizeY], SEEK_SET);
    fread(raw->tmp, 1, (unsigned int)raw->rowSize[y+z*raw->sizeY],
          raw->file);
      
    iPtr = raw->tmp;
    oPtr = buf;
    while (!done) {
      pixel = *iPtr++;
      count = (int)(pixel & 0x7F);
      if (!count) {
        done = 1;
        return;
      }
      if (pixel & 0x80) {
        while (count--) {
          *oPtr++ = *iPtr++;
        }
      } else {
        pixel = *iPtr++;
        while (count--) {
          *oPtr++ = pixel;
        }
      }
    }
  } else {
    fseek(raw->file, 512+(y*raw->sizeX)+(z*raw->sizeX*raw->sizeY),
          SEEK_SET);
    fread(buf, 1, raw->sizeX, raw->file);
  }
}


int SGIFrameInfo::LoadImage(ImagePtr image, 
                            ImageFormat *format, 
                            const Rectangle */*desiredSubRegion*/,
                            int /*LOD*/) {
  
  image->width = mWidth;
  image->height = mHeight;
  image->levelOfDetail = 0;
  image->imageFormat = *format;

  if (!image->allocate(mHeight * mWidth * image->imageFormat.bytesPerPixel)) {
    ERROR("could not allocate %dx%dx%d image data",
          mWidth, mHeight, mDepth);
    return 0;
  }

  if (image->imageFormat.rowOrder == ROW_ORDER_DONT_CARE)
    image->imageFormat.rowOrder = BOTTOM_TO_TOP; /* Bias for OpenGL */
  else
    image->imageFormat.rowOrder = format->rowOrder;
  
  if (image->imageFormat.bytesPerPixel == 0)
    image->imageFormat.bytesPerPixel = 3;
  
  if (image->imageFormat.scanlineByteMultiple == 0)
    image->imageFormat.scanlineByteMultiple = 1;
  
  /*  int rowWidth = ROUND_UP_TO_MULTIPLE(image->width * 
                                   image->imageFormat.bytesPerPixel,
                                   image->imageFormat.scanlineByteMultiple);
  
  */
  
  /* OK, now really load the image */
  {
	rawImageRec *rec;
	char *ptr, *rowStart;
	int i, j, rowStride;
	int rIndex, gIndex, bIndex;

	rec = RawImageOpen(mFilename.c_str());
	if (!rec) {
      DEBUGMSG("SGI reader could not open filename %s", mFilename.c_str());     
      return 0;
    }

	if (image->imageFormat.redShift != 0 ||
	    image->imageFormat.greenShift != 0 ||
	    image->imageFormat.blueShift != 0) {
      rIndex = image->imageFormat.redShift / 8;
      gIndex = image->imageFormat.greenShift / 8;
      bIndex = image->imageFormat.blueShift / 8;
      bb_assert(rIndex < 3);
      bb_assert(gIndex < 3);
      bb_assert(bIndex < 3);
	}
	else {
      rIndex = 0;
      gIndex = 1;
      bIndex = 2;
	}

	if (image->imageFormat.rowOrder == TOP_TO_BOTTOM) {
      rowStart = (char *) image->Data()
		+ (image->height - 1) * mWidth;
      rowStride = -mWidth ;
	}
	else {
      rowStart = (char *) image->Data();
      rowStride = mWidth;
	}

	for (i = 0; i < (int) rec->sizeY; i++) {
      RawImageGetRow(rec, rec->tmpR, i, 0);
      RawImageGetRow(rec, rec->tmpG, i, 1);
      RawImageGetRow(rec, rec->tmpB, i, 2);
      if (rec->sizeZ > 3) {
		RawImageGetRow(rec, rec->tmpA, i, 3);
      }
      ptr = rowStart;
      for (j = 0; j < (int) rec->sizeX; j++) {
		ptr[rIndex] = *(rec->tmpR + j);
		ptr[gIndex] = *(rec->tmpG + j);
		ptr[bIndex] = *(rec->tmpB + j);
		if (image->imageFormat.bytesPerPixel == 4) {
          ptr[3] = *(rec->tmpA + j);
		}
		ptr += image->imageFormat.bytesPerPixel;
      }
      rowStart += rowStride;
	}

	RawImageClose(rec);
  }

  image->loadedRegion.x = 0;
  image->loadedRegion.y = 0;
  image->loadedRegion.height = mHeight;
  image->loadedRegion.width = mWidth;


  return 1;
}

SGIFrameInfo::SGIFrameInfo(string fname): FrameInfo(fname) {
  rawImageRec *rec = RawImageOpen(mFilename.c_str());
  if (!rec) {
	DEBUGMSG("The file '%s' is NOT an SGI RGB file.", mFilename.c_str());
	return ;
  }
  /* Fill out the rest of the frameInfo information */
  mWidth = rec->sizeX;
  mHeight = rec->sizeY;
  mDepth = rec->sizeZ * 8;
  mFrameNumberInFile = 0;
  mFilename = fname;
  RawImageClose(rec);
  DEBUGMSG("The file '%s' is a valid SGI file.", mFilename.c_str());
  mValid = true; 
  return; 
}


FrameListPtr sgirgbGetFrameList(const char *filename)
{

  /* Prepare the FrameList and FrameInfo structures we are to
   * return to the user.  Since an RGB file stores a single 
   * frame, we need only one frameInfo, and the frameList
   * need be large enough only for 2 entries (the information
   * about the single frame, and the terminating NULL).
   */
  FrameInfoPtr frameInfo(new SGIFrameInfo(filename)); 
  if (!frameInfo ) {
	ERROR("cannot allocate FrameInfo structure");
	return FrameListPtr();
  }
  if (!frameInfo->mValid) {
	return FrameListPtr();
  }
    

  FrameListPtr frameList(new FrameList(frameInfo)); 
  if (!frameList) {
	ERROR("cannot allocate FrameInfo list structure");
	return frameList;
  }


  /* Fill out the final return form, and call it a day */
  frameList->append(frameInfo);
  frameList->mTargetFPS = 0.0;
  frameList->mFormatName = "SGI RGB";
  frameList->mFormatDescription = "Single-frame image in an SGI RGB file";
  return frameList;
}



