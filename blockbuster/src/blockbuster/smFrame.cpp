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
#include "smFrame.h"
#include "frames.h"
#include "sm/smBase.h"
#include "settings.h"
#include "zlib.h"
#include <X11/Xlib.h>

#include "boost/shared_ptr.hpp"

//============================================================
int SMFrameInfo::LoadImage(ImagePtr image, ImageFormat *, const Rectangle *desiredSub, int levelOfDetail) {
  const uint32_t imgWidth = mWidth >> levelOfDetail;
  const uint32_t imgHeight = mHeight >> levelOfDetail;
  bb_assert(imgWidth > 0);
  bb_assert(imgHeight > 0);
  
  if (!image->allocate(imgWidth * imgHeight * 3)) {
    ERROR("could not allocate %dx%dx24 image data",
          mWidth, mHeight);
    return 0;
  }
  image->width = imgWidth;
  image->height = imgHeight;
  image->imageFormat.bytesPerPixel = 3;
  image->imageFormat.scanlineByteMultiple = 1;
  image->imageFormat.byteOrder = MSB_FIRST;
  image->imageFormat.rowOrder = BOTTOM_TO_TOP; /* OpenGL order */
  image->levelOfDetail = levelOfDetail;


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
  dest = (char*) image->Data() + (pos[1] * image->width + pos[0]) * 3;
  
  DEBUGMSG("smLoadImage: Getting frame %d, SM region at (%d, %d), size (%d x %d)  of image sized (%d x %d)	destStride=%d",
		   this->mFrameNumberInFile, desiredSub->x, desiredSub->y, 
		   size[0], size[1],
		   image->width, image->height, destStride);
  
  this->mSM->getFrameBlock(this->mFrameNumberInFile, (void *) dest, 
                           GetCurrentThreadID(), destStride,
                           size, pos, step, levelOfDetail);
  
  image->loadedRegion.x = desiredSub->x;
  image->loadedRegion.y = desiredSub->y;
  image->loadedRegion.width = desiredSub->width;
  image->loadedRegion.height = desiredSub->height;
  
  return 1;
}




#define ORDINAL_SUFFIX(x) (                         \
                           (x > 3 && x < 21)?"th":  \
                           ((x % 10) == 2)?"nd":    \
                           ((x % 10) == 3)?"rd":    \
                           "th"                     \
                           )

FrameListPtr smGetFrameList(const char *filename)
{
  int smType;
  uint32_t numFrames, height, width, flags, maxLOD;
  int stereo = 0;
  register uint32_t i;
  FrameListPtr frameList; 
  
  ProgramOptions *options = GetGlobalOptions(); 
  boost::shared_ptr<smBase> sm(smBase::openFile(filename, O_RDONLY , options->readerThreads+1)); 
  //sm = smBase::openFile(filename, options->readerThreads+1);
  if (!sm) {
	DEBUGMSG("SM cannot open the file '%s'", filename);
	return frameList;
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
	DEBUGMSG("	SM resolution level %d: size %dx%d, tile %dx%d",
             i, sm->getWidth(i),sm->getHeight(i),
             sm->getTileWidth(i),sm->getTileHeight(i)
             );
  }
  flags = sm->getFlags();
  DEBUGMSG("SM flags: 0x%x", flags);
  if (flags & SM_FLAGS_STEREO) {
	DEBUGMSG("	SM_FLAGS_STEREO (0x%x)", SM_FLAGS_STEREO);
	stereo = 1;
  }

  if (numFrames == 0) {
	WARNING("SM file %s has no frames", filename);
	return frameList;
  }

  /* Get the structures we'll need to return information */
  frameList.reset(new FrameList); 
  if (!frameList) {
	ERROR("SM cannot allocate FrameInfo list structure");
  } else {
    // valid frameList
    for (i = 0; i < numFrames; i++) {
      FrameInfoPtr frameInfo
        (new SMFrameInfo(filename, width, height, 24, maxLOD, i, sm)); 
      if (!frameInfo) {
        ERROR( "cannot allocate %d%s FrameInfo structure (of %d) for file %s",
               i, ORDINAL_SUFFIX(i), numFrames, filename);
        frameList.reset(); 
        return frameList;
      }
      frameList->append(frameInfo); 
    }

    /* If we're here, we have a list of frames, all of which point at
     * the same private data structure.	 The use count for this structure
     * is obviously the number of frames.
     */
    frameList->mTargetFPS = sm->getFPS();
    frameList->mStereo = stereo;
    frameList->mWidth = sm->getWidth(0); 
    frameList->mHeight = sm->getHeight(0); 
    frameList->mFormatName = "SM";
    frameList->mFormatDescription =
      "Multiple frames in a shared SM (Streaming Movie) file";
  }
  return frameList;
}

