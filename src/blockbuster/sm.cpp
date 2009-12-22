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
#include <math.h>
#include "errmsg.h"
#include "canvas.h"
#include "errmsg.h"
#include "frames.h"
#include "frames.h"
#include "sm/smBase.h"

#include "zlib.h"
#include <X11/Xlib.h>

/* This structure stores the SM instance that we'll use to read these images.
 * There's also a use counter, so that we know when we can destroy the 
 * SM instance.
 */
typedef struct {
    smBase *sm;
    int useCount;
} privateData;

int
smLoadImage(Image *image, struct FrameInfo *frameInfo, 
          Canvas *canvas, const Rectangle *desiredSub, int levelOfDetail)
{
  //  DEBUGMSG(QString("smLoadImage(%1").arg(frameInfo->frameNumber)); 
  const privateData *p = (privateData *)frameInfo->privateData;
  const uint32_t imgWidth = frameInfo->width >> levelOfDetail;
  const uint32_t imgHeight = frameInfo->height >> levelOfDetail;
  bb_assert(image);
  bb_assert(imgWidth > 0);
  bb_assert(imgHeight > 0);
  
  if (!image->imageData) {
    /* The Canvas gets the opportunity to allocate image data first. */
    image->imageData = calloc(1, imgWidth * imgHeight * 3);
    if (image->imageData == NULL) {
      ERROR("could not allocate %dx%dx24 image data",
            frameInfo->width, frameInfo->height);
      return 0;
    }
    image->width = imgWidth;
    image->height = imgHeight;
	image->imageDataBytes = imgWidth * imgHeight * 3;
     image->imageFormat.bytesPerPixel = 3;
    image->imageFormat.scanlineByteMultiple = 1;
    image->imageFormat.byteOrder = MSB_FIRST;
    image->imageFormat.rowOrder = BOTTOM_TO_TOP; /* OpenGL order */
    image->levelOfDetail = levelOfDetail;
  }
  else {
	/* some sanity checks */
	bb_assert(image->width > 0);
	bb_assert(static_cast<int32_t>(image->width) <= frameInfo->width);
	bb_assert(image->height > 0);
	bb_assert(static_cast<int32_t>(image->height) <= frameInfo->height);
    bb_assert(image->imageFormat.rowOrder == BOTTOM_TO_TOP);
    bb_assert(image->imageDataBytes >= imgWidth * imgHeight * 3);
    bb_assert(image->width == imgWidth);
    bb_assert(image->height == imgHeight);
  }
  
  /* compute position and size of region in res=levelOfDetail coordinates */
  const int destStride = image->width * 3;
  int size[2], pos[2], step[2];
  char *dest;
  pos[0] = desiredSub->x;
  pos[1] = image->height - (desiredSub->y + desiredSub->height);
  size[0] = desiredSub->width;
  size[1] = desiredSub->height;
  step[0] = 1;
  step[1] = 1;
  dest = (char *) image->imageData + (pos[1] * image->width + pos[0]) * 3;
  
  DEBUGMSG("smLoadImage: Getting frame %d, SM region at (%d, %d), size (%d x %d)  of image sized (%d x %d)  destStride=%d",
           frameInfo->frameNumber, desiredSub->x, desiredSub->y, 
           size[0], size[1],
           image->width, image->height, destStride);
  
  p->sm->getFrameBlock(frameInfo->frameNumber, (void *) dest, 
                       GetCurrentThreadID(), destStride,
                       size, pos, step, levelOfDetail);
  
  image->loadedRegion.x = desiredSub->x;
  image->loadedRegion.y = desiredSub->y;
  image->loadedRegion.width = desiredSub->width;
  image->loadedRegion.height = desiredSub->height;
  
  return 1;
}

/* This routine is called to release all the memory associated with a frame. */
void smDestroyFrameInfo(FrameInfo *frameInfo)
{
    if (frameInfo) {
	/* One less frame using the private data.  If the private data is gone,
	 * free it.  This won't happen until all the frameInfo structures are
	 * freed, so we shouldn't have any dangling pointers.
	 */
	if (frameInfo->privateData) {
	    privateData *p = (privateData *)frameInfo->privateData;
	    if (--p->useCount == 0) {
		delete p->sm;
		free(p);
	    }
	}

	/* Call the default routine to free the rest of the frame */
	DefaultDestroyFrameInfo(frameInfo);
    }
}

#define ORDINAL_SUFFIX(x) (\
    (x > 3 && x < 21)?"th":\
    ((x % 10) == 2)?"nd":  \
    ((x % 10) == 3)?"rd":  \
    "th"                   \
)

FrameList *smGetFrameList(const char *filename)
{
    FrameList *frameList;
    smBase *sm = NULL;
    privateData *privateDataPtr;
    int smType;
    uint32_t numFrames, height, width, flags, maxLOD;
	int stereo = 0;
    register uint32_t i;

    //    if (!initialized) {
    smBase::init();
    //initialized = 1;
    //    }
    ProgramOptions *options = GetGlobalOptions(); 
    sm = smBase::openFile(filename, options->readerThreads+1);
    if (sm == NULL) {
	DEBUGMSG("SM cannot open the file '%s'", filename);
	return NULL;
    }

    DEBUGMSG("SM file '%s': streaming movie version %d",
	filename, sm->getVersion());
    smType = sm->getType();
    switch(smType) {
	case 0: DEBUGMSG("SM format: RAW uncompressed"); break;
	case 1: DEBUGMSG("SM format: RLE compressed"); break;
	case 2: DEBUGMSG("SM format: gzip compressed"); break;
	case 3: DEBUGMSG("SM format: LZ0 compressed"); break;
	case 4: DEBUGMSG("SM format: JPG compressed"); break;
	default: DEBUGMSG("SM format: unknown (%d)", smType);
    }
    numFrames = sm->getNumFrames();
    height = sm->getHeight();
    width = sm->getWidth();
    maxLOD = sm->getNumResolutions() - 1;
    DEBUGMSG("SM size %dx%d, frames %d, frames/sec %0.2f",
	width, height,
	numFrames,
	sm->getFPS()
    );
    DEBUGMSG("SM number of resolutions: %d", sm->getNumResolutions());
    for(i=0;i<sm->getNumResolutions();i++) {
	DEBUGMSG("  SM resolution level %d: size %dx%d, tile %dx%d",
	    i, sm->getWidth(i),sm->getHeight(i),
	    sm->getTileWidth(i),sm->getTileHeight(i)
	);
    }
    flags = sm->getFlags();
    DEBUGMSG("SM flags: 0x%x", flags);
    if (flags & SM_FLAGS_STEREO) {
	DEBUGMSG("  SM_FLAGS_STEREO (0x%x)", SM_FLAGS_STEREO);
	stereo = 1;
    }

    if (numFrames == 0) {
	WARNING("SM file %s has no frames", filename);
	delete sm;
	return NULL;
    }

    /* Get the structures we'll need to return information */
    frameList = new FrameList; 
    //    frameList = (FrameList *)calloc(1, sizeof(FrameList) + numFrames*sizeof(FrameInfo *));
    if (frameList == NULL) {
	ERROR("SM cannot allocate FrameInfo list structure");
	delete sm;
	return NULL;
    }

    /* There'll be one private data pointer for all the frames, to store the SM
     * instance.  It will have a counter (of the number of frames using the SM
     * instance) so that we know when we can destroy it.
     */
    privateDataPtr = (privateData *)calloc(1, sizeof(privateData));
    if (privateDataPtr == NULL) {
	ERROR("SM cannot allocate private data structure");
	delete frameList;
	delete sm;
	return NULL;
    }

    for (i = 0; i < numFrames; i++) {
	FrameInfo *frameInfo;
	frameInfo = (FrameInfo *)calloc(1, sizeof(FrameInfo));
	if (frameInfo == NULL) {
	    ERROR(
		"cannot allocate %d%s FrameInfo structure (of %d) for file %s",
		i, ORDINAL_SUFFIX(i), numFrames, filename);
	    free(privateDataPtr);
	    delete frameList;
	    delete sm;
	    return NULL;
	}
	frameInfo->width = width;
	frameInfo->height = height;
        frameInfo->maxLOD = maxLOD;
	frameInfo->depth = 24;
	frameInfo->frameNumber = i;
	frameInfo->enable = 1;
	frameInfo->DestroyFrameInfo = smDestroyFrameInfo;
	frameInfo->LoadImage = smLoadImage;
	frameInfo->privateData = privateDataPtr;
        frameInfo->filename = strdup(filename);
	frameList->append(frameInfo); 
    }

    /* If we're here, we have a list of frames, all of which point at
     * the same private data structure.  The use count for this structure
     * is obviously the number of frames.
     */
    frameList->targetFPS = sm->getFPS();
    frameList->stereo = stereo;
    privateDataPtr->sm = sm;
    privateDataPtr->useCount = numFrames;

    frameList->formatName = "SM";
    frameList->formatDescription =
      "Multiple frames in an SM (Streaming Movie) file";
    return frameList;
}

