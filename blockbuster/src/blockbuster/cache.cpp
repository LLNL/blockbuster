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
 * OTHERWISE, ARISING  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "errmsg.h"
#include "cache.h"
#include "errmsg.h"
#include "frames.h"
#include "util.h"
#include "errmsg.h"

#define NEW_CACHE 1

bool cacheDebug = false; 
void enableCacheDebug(bool onoff) { cacheDebug = onoff; }
bool cacheDebug_enabled(void) { return cacheDebug; }

/* This file contains routines for manipulating the Image Cache.
 *
 * An Image Cache is a collection of images, both read from the
 * disk (i.e. full images) and as yet unread (i.e., frame
 * specifications).  The Image Cache abstraction allows for images
 * to be preloaded, loaded upon request, retrieved quickly if they're
 * already loaded, etc.
 *
 * This implementation expects to cache at most a few tens of images;
 * as such, complicated storage and sorting routines aren't necessary,
 * as the few images stored can be searched in very short time compared
 * to image display and image reading.  We'll stick with simple, basic,
 * linear list management.
 */

// debugging pthread mutex deadlock:


#define ERROR_STRING(x) (                               \
                         (x==EINVAL)?"EINVAL":          \
                         (x==EDEADLK)?"EDEADLK":        \
                         (x==EBUSY)?"EBUSY":            \
                         (x==EPERM)?"EPERM":            \
                         (x==ETIMEDOUT)?"ETIMEDOUT":    \
                         (x==EINTR)?"EINTR":            \
                         "unknown")

// ======================================================================
//! returns true if first is MORE important than second, even though this functor implements "less than".  This sorts the queue in reverse order, meaning that most important job is always next in the queue. 
uint32_t gCurrentFrame, gLastFrame; 
bool jobComparer(const ImageCacheJobPtr first, const ImageCacheJobPtr second){
  int64_t diff1 = first->frameNumber - gCurrentFrame, 
    diff2 = second->frameNumber - gCurrentFrame; 
  if (diff1 < 0) diff1 += gLastFrame; 
  if (diff2 < 0) diff2 += gLastFrame; 
  return diff1 > diff2; 
}


//=============================================================
ImageCacheJobPtr  FindJobInQueue(deque<ImageCacheJobPtr> &queue,  
                                 unsigned int frameNumber,
                                 const Rectangle *region,
                                 unsigned int levelOfDetail) {
  deque<ImageCacheJobPtr>::iterator pos = queue.begin(), endpos = queue.end(); 
  while (pos != endpos) {
    if ((*pos)->frameNumber == frameNumber &&
        (*pos)->levelOfDetail == levelOfDetail) {
      if (region == NULL || RectContainsRect(&(*pos)->region, region)) {
        return *pos;
      } 
    }
    ++pos; 
  }
  return ImageCacheJobPtr();  
}

//=============================================================
void RemoveJobFromQueue(deque<ImageCacheJobPtr> &queue, ImageCacheJobPtr job) {
  deque<ImageCacheJobPtr>::iterator pos = queue.begin(), endpos = queue.end(); 
  while (pos != endpos) {
    if (*pos == job) {
      queue.erase(pos); 
      return; 
    }
    ++pos; 
  }
  ERROR("Attempted to remove nonexistent job from queue!"); 
  return; 
}

/*!
  You probably want to call the macro, PrintJobQueue(q)
*/
#define PrintJobQueue(q) __PrintJobQueue(#q,q)
void __PrintJobQueue(QString name, deque<ImageCacheJobPtr>&q) {
  deque<ImageCacheJobPtr>::iterator pos = q.begin(), endpos = q.end(); 
  CACHEDEBUG(QString(" deque<ImageCacheJob *>%1 (length %2): ").arg(name).arg(q.size())); 
  while (pos != endpos){
    CACHEDEBUG(string(**pos)); 
    ++pos; 
  }
  return; 
}

// ======================================================================
void CacheThread::ResetImages(uint32_t numimages) {
  mCachedImages.clear(); 
  mCachedImages.resize(numimages); 
  for (uint32_t i = 0; i < numimages; i++) {
    mCachedImages[i].reset(new CachedImage()); 
  }
  return; 
}


// ======================================================================
/* This is the work function, which is used by child threads to 
 * "asynchronously" load images from disk.  Not used in single-threaded case. 
 */

void CacheThread::run() {
  ImagePtr image;
  ImageCacheJobPtr job;
  CachedImagePtr imageSlot;

  // RegisterThread(this); // see common.cpp -- used to create a 0-based index that can be referenced anywhere. 
  CACHEDEBUG("CacheThread::run() (thread = %p, thread ID = %d, mThreadNum = %d)", QThread::currentThread(), GetCurrentThreadID(), mThreadNum); 

  /* Repeat forever, until the thread is cancelled by the main thread */
  while (1) {

    /* Try to get a job off the work queue.  We want to make sure
     * we're the only ones trying to do so at this particular
     * momemt, so grab the mutex.
     */
    lock("check for job in work queue", __FILE__, __LINE__);
            
    /* Wait for a job to appear on the work queue (which we can do
     * because we currently hold the mutex for the queue).  This is
     * in a "while" loop because sometimes a thread can be woken up
     * spuriously (by signal) or unwittingly (if all threads are woken
     * up on the work ready condition, but one has already grabbed the
     * work).
     */
    while (! mJobQueue.size()) {
      /* Wait for work ready condition to come in. */
      WaitForJobReady("no job in work queue yet", __FILE__, __LINE__); 
      if (mStop) {
        CACHEDEBUG("Thread %p terminating", QThread::currentThread()); 
        UnregisterThread(this); 
        unlock("thread is dying", __FILE__, __LINE__); 
        return; 
      }
    }
    // take the job from head of the queue: 
    job = mJobQueue.at(0); 
    CACHEDEBUG("Worker moving job %d from job queue to pending cue", job->frameNumber); 
    mJobQueue.pop_front(); 
    PrintJobQueue(mJobQueue); 

	/* Add the job removed from the job queue to the pending queue,
	 * so that the main thread can tell the job is in progress.
	 */
    mPendingQueue.push_back(job); 
    PrintJobQueue(mPendingQueue); 
	/* We don't need the mutex (or the cancellation function) any more;
	 * this call will both clear the cancellation function, and cause
	 * it to be called to unlock the mutex, more or less simultaneously.
	 */
    unlock("found job and added to pending queue", __FILE__, __LINE__); 

	/* Do the work.  Note that this function could cause warning and
	 * error output, and could still return an empty ImagePtr.  Even if an
	 * empty ImagePtr comes back, we have to record it, so that the 
	 * boss thread can return.
	 */
    CACHEDEBUG("Worker thread calling LoadAndConvertImage for frame %d", job->frameNumber); 
    image = job->frameInfo->LoadAndConvertImage
      (&mCache->mRequiredImageFormat, 
       &job->region, job->levelOfDetail);

 
	/* With image in hand (or possibly a NULL), we're ready to start
	 * modifying caches and queues.  We always will have to remove
	 * the job from the PendingQueue.
	 */
    lock("finished job, updating queues", __FILE__, __LINE__); 

    CACHEDEBUG("Remove Job From mPendingQueue frame %d", job->frameNumber); 
	RemoveJobFromQueue(mPendingQueue, job);
    PrintJobQueue(mPendingQueue);

	/* First see if this request has been invalidated (because the
	 * main thread changed the frame list while we were working).
	 * If so, there's nothing to do with the image.
	 */
	if (job->requestNumber <= mCache->mValidRequestThreshold) {
      CACHEDEBUG("Destroying useless job frame %d, because job request number %d is less than or equal to the cache valid request threshold %d", job->frameNumber, job->requestNumber, mCache->mValidRequestThreshold); 
      /* The job is useless.  Destroy the image, if we got one,
       * and go home.  We don't need to send an indicator to
       * anyone; the main thread doesn't care.
       */
      unlock("useless job", __FILE__, __LINE__); 
      if (image) {
        image.reset();
      }
	}
	else if (!image) {
      CACHEDEBUG("Error in job frame %d!", job->frameNumber); 
      /* The main thread has to be told that there was
       * an error here; otherwise, it may be waiting forever
       * for an image that never appears.  So put it on
       * the error queue (which the main thread will
       * check) and signal that the job is done.
       */
      mErrorQueue.push_back(job); 
      PrintJobQueue(mErrorQueue);
      unlock("error in job", __FILE__, __LINE__); 
      WakeAllJobDone("error in job",__FILE__, __LINE__); 
	}
	else if (!(imageSlot = GetCachedImageSlot(job->frameNumber))) {
      CACHEDEBUG("No place to put job frame %d!", job->frameNumber); 
      /* We have an image, but no place to put it.  This
       * is an error as well, that the main thread has
       * to be made aware of.  As usual, the error has
       * already been reported by here, so we don't
       * have to report another one.
       */
      mErrorQueue.push_back(job); 
      PrintJobQueue(mErrorQueue);

      unlock("no place for job", __FILE__, __LINE__); 
      WakeAllJobDone("no place for job",__FILE__, __LINE__);    
      image.reset();
	}
	else {
      /* Hey, things actually went okay; we have an image
       * and a place to put it.  We can lose the job,
       * and store the image.
       */
      CACHEDEBUG("frameNumber %d marked complete", job->frameNumber); 
      imageSlot->frameNumber = job->frameNumber;
      imageSlot->levelOfDetail = job->levelOfDetail;
      imageSlot->image = image;
      imageSlot->loaded = 1;
      imageSlot->requestNumber = job->requestNumber;

      unlock("job success", __FILE__, __LINE__);
      WakeAllJobDone("job success", __FILE__, __LINE__); 
 
      /* Fairness among threads is greatly improved by yielding here.
       * Without this, threads can frequently starve for a while and
       * that causes prefetching to get out of order.
       * XXX this probably isn't portable.
       */
      CACHEDEBUG("sched_yield"); 
      sched_yield();
	}

  } /* loop forever, until the thread is cancelled */

  return; 
}

// ======================================================================
/*
 * Allocate a cache slot.  If the cache is full, an existing frame will have
 * to be discarded.
 * Input: newFrameNumber - indicates the number of the new frame we want to
 * insert into the cache.  We use this to determine which entry to discard
 * if the cache is full.
 */
CachedImagePtr CacheThread::GetCachedImageSlot(uint32_t newFrameNumber)
{
  CachedImagePtr imageSlot;
  /*#ifdef NEW_CACHE
  unsigned long maxDist = 0;
#endif
  */ 
  CACHEDEBUG("GetCachedImageSlot(%d)", newFrameNumber); 
  volatile uint32_t slotnum = 0, foundslot = -1, numslots = mCachedImages.size(), oldestRequest = 1000*1000*1000; 
  for (vector<CachedImagePtr>::iterator cachedImage = mCachedImages.begin(); 
       cachedImage != mCachedImages.end();  cachedImage++, slotnum++) {
    /* Look for an empty slot, or a slot with the oldest request number. 
     */
    if ((*cachedImage)->requestNumber == 0 || !*cachedImage) {
      imageSlot = *cachedImage;
      foundslot = slotnum; 
      break;
    }
    if ((*cachedImage)->requestNumber < oldestRequest) {
      imageSlot = *cachedImage; 
      oldestRequest = (*cachedImage)->requestNumber; 
      foundslot = slotnum; 
    }
  }
  
  CACHEDEBUG("Removing frame %u in slot %u of %u to make room for %u", 
             imageSlot->frameNumber, foundslot, numslots, newFrameNumber);
  if (imageSlot->loaded) {
    imageSlot->image.reset();
    imageSlot->loaded = 0;
  }

  return imageSlot;
}

// ======================================================================
/*
 * Evalute the "distance" between two frame numbers and return a score.
 * The higher the score, the farther apart the frames are in sequence.
 * In cache replacement, we'll use the highest score to determine the victim.
 */
/* unsigned int CacheThread::Distance(unsigned int oldFrame, unsigned int newFrame)
   {
   int totalFrames = mCache->mCurrentEndFrame - mCache->mCurrentStartFrame + 1; 
   if (oldFrame > newFrame) {
   // wrap-around; use modular/ring/clock arithmetic 
   unsigned int d1 = oldFrame - newFrame;
   unsigned int d2 = newFrame + totalFrames - oldFrame;
   return MIN2(d1, d2);
   }
   else {
   return newFrame - oldFrame;
   }
   }
*/
// ======================================================================
/* For debugging */
void CacheThread::Print(string description)
{

  if (!cacheDebug) return; 

  register uint32_t  i=0, minframe = 1000*1000*1000, maxframe = 0;
  QString msg; 
  CACHEDEBUG("%s: Printing cache state.", description.c_str()); 
  for (vector<CachedImagePtr>::iterator cachedImage = mCachedImages.begin(); 
       cachedImage != mCachedImages.end();  cachedImage++, i++) {
    string roimsg; 
    if ((*cachedImage)->image) {
      roimsg = string("roi:")+string((*cachedImage)->image->loadedRegion);
    }
    msg = QString("%1:  Slot %2: frame:%3 lod:%4  req:%5 %6 ")
      .arg(description.c_str())
      .arg( i)
      .arg((*cachedImage)->frameNumber)
      .arg((*cachedImage)->levelOfDetail)
      .arg((*cachedImage)->requestNumber)
      .arg(roimsg.c_str());
    if (minframe > (*cachedImage)->frameNumber) {
      minframe = (*cachedImage)->frameNumber;
    }
    if (maxframe < (*cachedImage)->frameNumber) {
      maxframe = (*cachedImage)->frameNumber;
    }
    CACHEDEBUG(msg); 
  }
  CACHEDEBUG("mHighest = %d, minframe = %d, maxframe = %d\n", mCache->mHighestFrameNumber, minframe, maxframe); 
}
//==================================================================
ImageCache::ImageCache(int numthreads, int numimages, ImageFormat &required): 
  mNumReaderThreads(numthreads), mMaxCachedImages(numimages), 
  mRequiredImageFormat(required), mRequestNumber(0), 
  mValidRequestThreshold(0), 
  mHighestFrameNumber(0), mPreloadFrames(0),  mStereo(false), 
  mCurrentPlayDirection(0), mCurrentStartFrame(0),  mCurrentEndFrame(0) {
  CACHEDEBUG("ImageCache constructor"); 
  
  register int i;
  CACHEDEBUG("CreateImageCache(mNumReaderThreads = %d, mMaxCachedImages = %d)", mNumReaderThreads, mMaxCachedImages);
  
  int threadImages = 0; 
  if (mNumReaderThreads > 0) {
    
    /* Now create the threads.  If we fail to create a thread, we can still
     * continue with fewer threads or with none (in the single-threaded case).
     */
    for (i = 0; i < mNumReaderThreads; i++) {
      if (numimages % numthreads > i) {
        threadImages = numimages/numthreads + 1; 
      } else {
        threadImages = numimages/numthreads; 
      }
      mThreads.push_back(CacheThreadPtr(new CacheThread(this, threadImages, i))); 
      mThreads[i]->start(); 
    }
  } 
  else {
    // push back a place to cache images but DO NOT START THE THREAD
    mThreads.push_back(CacheThreadPtr(new CacheThread(this, threadImages, 0))); }
  return;
}

// ======================================================================
ImageCache::~ImageCache() {
  /* If we have reader threads, we have to clear the cache
   * work queue and cancel all the reader threads.
   */
  CACHEDEBUG("DestroyImageCache");
  if (mNumReaderThreads > 0) {
	register int i;
    
	/* Clear the work queue so no more work is taken up */
    ClearJobQueues(); 

	/* Cancel all the threads, stopping all the work being done */
    CACHEDEBUG("Canceling %d threads", mNumReaderThreads);
    for (i = 0; i < mNumReaderThreads; i++) {
      CACHEDEBUG("terminating thread %d", i);
      // mThreads[i]->terminate(); 
      mThreads[i]->stop();  // more gentle;  give up mutexes, etc. 
    }
    
    for (i = 0; i < mNumReaderThreads; i++) {
      mThreads[i]->wait();    
	}
    for (i = 0; i < mNumReaderThreads; i++) {
      mThreads[i]->mPendingQueue.clear();
      mThreads[i]->mErrorQueue.clear();
      mThreads[i].reset(); 
    }
    mThreads.clear();     

  }
    
  /* Any images in the cache must be released */
  ClearImages();

  return; 
}

//=============================================================
void ImageCache::ClearQueue(deque<ImageCacheJobPtr> &queue) {
  queue.clear(); 
}

//=============================================================

/* This function clears out all images still residing in the image
 * cache.  It is used when the cache is destroyed, or when its
 * intrinsic FrameList is changed (thus invalidating all frame
 * numbers and potentially the images as well).
 */
void ImageCache::ClearImages(void)
{
  CACHEDEBUG("ImageCache::ClearImages"); 
  /* Any images in the cache must be released */
  for (uint32_t i=0; i<mThreads.size(); i++) {
    mThreads[i]->ResetImages(mMaxCachedImages); 
  }

  return; 
}

//=============================================================
void ImageCache::ClearJobQueues(void)
{
  /* All pending work jobs must be cancelled */
  for (uint32_t i = 0; i < mThreads.size(); i++) {
    mThreads[i]->lock("clearJobQueue", __FILE__, __LINE__); 
    mThreads[i]->mJobQueue.clear();
    mThreads[i]->unlock("JobQueue cleared", __FILE__, __LINE__); 
  }
  /* All pending requests that have a request number below or equal
   * to the threshold will be discarded when complete.
   */
  mValidRequestThreshold = mRequestNumber;
  return; 
}


// ======================================================================
/* This routine is called to allow the Image Cache to manage the
 * associated frames.  The frames will be added to any list the
 * image cache is already managing.
 * 
 * Note that the frameList may be freed after this call; but the
 * individual FrameInfo pointers in its frameList->frames[] array
 * may not be, as they become the responsibility of the image
 * cache after the call.
 *
 * This must be called from the main thread; it is not safe
 * for more than one thread to call at once.
 */
void ImageCache::ManageFrameList(FrameListPtr frameList)
{
  /* If we've got worker threads, there may already be work that
   * is being done with the current framelist.  Clear all the
   * outstanding work requests, and make sure that any jobs
   * in progress will not enter the image cache.
   */
  if (mNumReaderThreads > 0) {
    ClearJobQueues();
  }
  gLastFrame = frameList->numActualFrames(); 

  /* If we've already cached images, they are now all out-of-date.
   * Free them.
   */
  ClearImages();

  /* 
     Put the new frame list in place. As long as they are here, 
     we have ownership of the shared_ptrs and they won't disappear even 
     if deleted elsewhere, like in movieLoop. 
  */
  mFrameList = frameList;

  /* The caller should already have updated the canvas' frame list. */
}


// ======================================================================
/* This routine looks for an image already in the image cache, pending and job queues and returns a list of images not in any of them in the given range. 
 * the cached image slot itself, or an empty CachedImagePtr if not found.
 * Note: we don't care about the loaded image region at this point.
 */
  
CachedImagePtr ImageCache::FindImage(uint32_t frame, uint32_t lod) {
  CACHEDEBUG("FindImage frame %d", frame); 
  /* Search the caches to see whether an appropriate image already
   * exists within the cache.
   */
  if (mThreads.size()) {
    uint32_t threadnum = frame % mThreads.size(); 
    
    for (vector<CachedImagePtr>::iterator cachedImage = 
           mThreads[threadnum]->mCachedImages.begin(); 
         cachedImage != mThreads[threadnum]->mCachedImages.end();  cachedImage++) {
      if ((*cachedImage)->loaded &&
          (*cachedImage)->frameNumber == frame &&
          (*cachedImage)->levelOfDetail == lod) {
        CACHEDEBUG("Found frame number %d", frame); 
        return *cachedImage;
      }
    }
  }

  return CachedImagePtr();
}


// ======================================================================
//! GetImage()
/*!
 * This routine gets an image from the cache, specified by frameNumber,
 * region of interest, and levelOfDetail.  It places the image in the cache
 * slot, then returns the image.  
 * 
 * The region of interest is adjusted to the level of detail.
 * That is, if the original, LOD=0 image is 4K x 4K, then a region
 * within the LOD=2 image will be within a 1K x 1K boundary.
 *
 * The image will already have been converted (if necessary) to a format
 * appropriate for the given Canvas.
 *
 * Note that there may be more than one cached image of the same frame;
 * this can happen if two different requests appear for the same frame, where
 * the region of interest of the second is not a subset of the region of interest
 * of the first.  It may be interesting to try to predict when this will happen,
 * and to load larger regions than asked in an effort to minimize the total
 * amount of file I/O; but in the expected high-demand case, where a movie is
 * playing, sequential requests will be of different frames.  As such, at this
 * juncture I don't believe such complexity is warranted.
 * 
 * Note that one time this theoretical case actually happens is upon startup, when -play 
 * is in effect.  The frames get subtly resized by DMX and so images are 
 * off by a pixel or two, resulting in non-overlapping ROI's.  
 * So it might pay off to play a game of "add two pixels to each edge." 
 */

ImagePtr ImageCache::GetImage(uint32_t frameNumber, 
                              const Rectangle *newRegion, 
                              uint32_t levelOfDetail, bool preload)  {

  ImagePtr image;
  CachedImagePtr cachedImage;
  CachedImagePtr imageSlot;
  //    int rv;
  Rectangle region = *newRegion;
  gCurrentFrame = frameNumber; 
  int threadnum = 0; 
  if (mThreads.size() > 0)  {
    threadnum = frameNumber % mThreads.size(); 
  }

  if (!mFrameList) {
    WARNING("frame %d requested but cache has no frame list",
            frameNumber);
    return image;
  }
  if (frameNumber > mFrameList->numActualFrames()) {
    WARNING("image %d requested from a cache managing %d images",
            frameNumber, mFrameList->numActualFrames() + 1);
	return image;
  }

  DEBUGMSG("ImageCache::GetImage frame %d",frameNumber); 
  
  if (preload) {
    uint32_t preloadmax = mCurrentEndFrame-mCurrentStartFrame, 
      frame=frameNumber, preloaded=0;
    if (preloadmax > mPreloadFrames) {
      preloadmax = mPreloadFrames;
    }
    int playdir = mCurrentPlayDirection ? mCurrentPlayDirection: 1;
    bool skipOddFrames = (mFrameList->mStereo && !mStereo); 
    while (preloaded < preloadmax) {
      frame += playdir; 
      if (frame > mCurrentEndFrame) frame = mCurrentStartFrame; 
      if (frame < mCurrentStartFrame) frame = mCurrentEndFrame; 
      if (frame % 2 && skipOddFrames) {
        continue; 
      }
      PreloadImage(frame, newRegion, levelOfDetail);        
      ++preloaded;     
    }    

    DEBUGMSG("Preloaded %d frames from %d to %d stepping by %d (max is %d)", preloaded, frameNumber, frame-playdir, playdir, preloadmax); 
  } // end preloading

  /* Loop until we find the request.  There are three ways we can
   * find the request; it can either be:
   *  - in the cache (in which case we return the cached image)
   *  - in the JobQueue (in which case we wait until an image
   *    is ready, and check again; note that the JobQueue is
   *    always empty in the single-threaded case)
   *  - in the PendingQueue (in which case we also wait until
   *    an image is ready, and check again; this queue is also
   *    always empty in the single-threaded case)
   *  - in the ErrorQueue (in which case we return with
   *    an empty ImagePtr; like the other queues, the ErrorQueue
   *    is always empty in the single-threaded case)
   *  - or not in any of the above (in which case we either
   *    load the image ourselves, in the single-threaded case,
   *    modify a job in the JobQueue, if a job with the same
   *    frame exists there; or add a new job to the JobQueue,
   *    and wait for an image to be ready)
   */

  if (mNumReaderThreads > 0) {
    mThreads[threadnum]->lock("checking for ready image", __FILE__, __LINE__);
  }

  /* This loop will continue until the code issues a "return" (after
   * the image has been found in the cache), or until the code issues
   * a "break" (to exit the loop and read the image itself).
   *
   * The loop is present to handle the case that a condition variable
   * might spuriously awake, before the correct image is present.
   */
  
  while (1) {

	/* Search the cache to see whether an appropriate image already
	 * exists within the cache.  The image is appropriate if the region
	 * of interest loaded for the frame is a superset of the region this
	 * caller wishes to see.
	 */
    cachedImage = FindImage(frameNumber, levelOfDetail);
	
	if (cachedImage) {
      if (RectContainsRect(&cachedImage->image->loadedRegion, &region)) {
		bb_assert(cachedImage->image->imageFormat.rowOrder != ROW_ORDER_DONT_CARE);        
		cachedImage->requestNumber = mRequestNumber;
		if (mNumReaderThreads > 0) {
          mThreads[threadnum]->unlock("found interesting frame", __FILE__, __LINE__); 
		}
        DEBUGMSG("Returning found image %d", frameNumber); 
		return cachedImage->image;
      }
      else {
        DEBUGMSG("Frame %d does not fully match, so augment rectangle", frameNumber); 
        region = RectUnionRect(&cachedImage->image->loadedRegion, &region);
      }
	}
    DEBUGMSG("Frame %d wasn't found, look for it in queues", frameNumber); 
	/* It's not in cache already, darn.  We need to check to see if a 
	 * job for this frame is already in one of the work queues or the
	 * error queue.  This, of course, can only happen in the multi-
	 * threaded case.
	 */
	if (mNumReaderThreads > 0) {
      
      /* Remember that we still have the imageCacheLock from above.
       * Look for a pending job (one that is being worked on) that
       * matches our requirements.
       */
      DEBUGMSG("Looking in pending queue for frame %d", frameNumber); 
      ImageCacheJobPtr job = 
        FindJobInQueue(mThreads[threadnum]->mPendingQueue, frameNumber, &region,levelOfDetail);
      
      if (job) {
		/* We've got a job coming soon that will give us this image.
		 * Just wait for the reader thread to complete.  This should
		 * kick us back to the top of the loop when we return, with
		 * the imageCacheLock already re-acquired.
		 */
        mThreads[threadnum]->WaitForJobDone("waiting for pending job", __FILE__, __LINE__); 
      }
      else {
		/* Look for a matching job in the work queue, one that may
		 * have been pre-loaded earlier.  We're a little less picky
		 * here; the job we find can refer to any region.  We're going
		 * to modify the job request, if necessary, to include the
		 * desired region.
		 */
        DEBUGMSG("Looking in job queue for frame %d with other rectangle", frameNumber); 
        job = FindJobInQueue(mThreads[threadnum]->mJobQueue, frameNumber, NULL, levelOfDetail);
        if (job) {
          /* Add our region of interest to the job; if a request had
           * been made earlier for a different region of the same
           * frame, the request will be modified to use the union
           * of the two regions.
           */
          
          job->region = RectUnionRect(&job->region, &region);
          job->requestNumber = mRequestNumber;
          /* Move the job to the head of the work queue, in case
           * it was behind a few other preload jobs; it's more important
           * now because the main thread is waiting on it.
           */
          RemoveJobFromQueue(mThreads[threadnum]->mJobQueue, job); 
          mThreads[threadnum]->mJobQueue.push_front(job); 
          
          /* Now wait for an image to appear. */
          mThreads[threadnum]->WaitForJobDone("waiting for modified pending job", __FILE__, __LINE__); 
		}
		else {
          /* No job in the pending queue nor in the work queue.  Check
           * the error queue to make sure the job hasn't been attempted
           * but has failed.
           */
          DEBUGMSG("No job for image %d in the pending queue nor in the work queue", frameNumber); 
          job = FindJobInQueue(mThreads[threadnum]->mErrorQueue, frameNumber,&region,levelOfDetail);
          if (job) {
			/* The error, hopefully, has already been reported.  We
			 * have to remove the job from the queue and return to
			 * the caller with an error.
			 */
			RemoveJobFromQueue(mThreads[threadnum]->mErrorQueue, job);
            
            mThreads[threadnum]->unlock("found error in job", __FILE__, __LINE__);
			return image;
          }
          else {
			/* No job anywhere.  Break out of the condition
			 * variable loop so the main thread can load the
			 * image itself.
			 */
            DEBUGMSG("No job for image %d anywhere, main thread must do it or something, shrug", frameNumber); 
            mThreads[threadnum]->unlock("no job found", __FILE__, __LINE__); 
            break;
          }
		}
      }
	} /* searching through queues in the multi-threaded case */
	else {
      /* single-threaded case, with no image in the cache; we need
       * to break out and load the image on our own.
       */
      break;
	}
  } /* looping for condition variables */
    
    /* If we get here, no one has asked for the image before us, so we
     * have to load it ourselves.  Before starting a load, do some 
     * basic error checks.  (We don't do these above in order to speed
     * the critical performance case.)
     */
  DEBUGMSG("Have to load image %d in main thread", frameNumber); 
   
  /* The requested image is valid enough.  If we're single-threaded, load the 
   * image ourselves; if we're multi-threaded, add work to the work queue and 
   * wait for an image to become ready.
   */
  FrameInfoPtr fip = mFrameList->getFrame(frameNumber); 
  image = fip-> LoadAndConvertImage(&mRequiredImageFormat, 
                                    &region, levelOfDetail);
  if (!image) {
	return image;
  }
  bb_assert(image->imageFormat.rowOrder != ROW_ORDER_DONT_CARE);

  /* We have an image; we'll add it to the cache ourselves.  Find
   * a slot we can stick it in.
   */
  if (mMaxCachedImages > 0 ) {
    if (mNumReaderThreads > 0) {
      mThreads[threadnum]->lock("adding image to cached image slots in main thread", __FILE__, __LINE__);
    }
    /* Either update the current image slot or find a new one */
    if (cachedImage) {
      imageSlot = cachedImage;
    }
    else {
      imageSlot = mThreads[threadnum]->GetCachedImageSlot(frameNumber);
    }
    
    if (!imageSlot) {
      /* We have an image, but no place to put it!
       * The error has been reported; destroy the image
       * and return empty image.
       */
      if (mNumReaderThreads > 0) {
        mThreads[threadnum]->unlock("no place for image", __FILE__, __LINE__); 
      }
      return ImagePtr(); 
    }
    
    /* if there's an image in this slot, free it! */
    imageSlot->image.reset(); 
    
    /* Otherwise, we're happy.  Store the image away. */
    imageSlot->frameNumber = frameNumber;
    imageSlot->levelOfDetail = levelOfDetail;
    imageSlot->image = image;
    imageSlot->loaded = 1;
    imageSlot->requestNumber = mRequestNumber;
    if (mNumReaderThreads > 0) {
      mThreads[threadnum]->unlock("image stored and locked successfully", __FILE__, __LINE__); 
    }
  }
  DEBUGMSG("Done with GetImage()"); 
  return image;
}


// ======================================================================
/* This routine is informatory; it notifies the cache that a particular frame
 * will likely be of interest soon, so that the cache can start loading the
 * image. 
 *
 * It should only be called from the main thread, since it accesses the
 * frameList field (which is unprotected).
 */
void ImageCache::PreloadImage(uint32_t frameNumber, 
                              const Rectangle *region, uint32_t levelOfDetail)
{
  CACHEDEBUG("PreloadImage() frame %d", frameNumber); 
  CachedImagePtr cachedImage;
  uint32_t threadnum = 0; 
  if (mThreads.size() > 0) {
    threadnum = frameNumber % mThreads.size();
  }
  
  /* Keep track of highest frame number.  We need it for cache
   * replacement.
   */
  if (frameNumber > mHighestFrameNumber)
    mHighestFrameNumber = frameNumber;
  
  /* This entry point is only useful if there are threads in the system. */
  if (mNumReaderThreads == 0) {
	return;
  }
  
  if (frameNumber >= mFrameList->numActualFrames()) {
	ERROR("trying to preload non-existent frame (%d of %d)",
	      frameNumber, mFrameList->numActualFrames());
	return;
  }
  
  /* If the image is already in cache, or if a job to load it already 
   * exists, don't bother adding a new job->
   */
  mThreads[threadnum]->lock("checking if image already exists", __FILE__, __LINE__);
  cachedImage = FindImage(frameNumber, levelOfDetail);
  if (cachedImage) {
    CACHEDEBUG("Found match for frame number %d in cache", frameNumber); 
    if (RectContainsRect(&cachedImage->image->loadedRegion, region)) {
      /* Image is already in cache - no need to preload again */
      mThreads[threadnum]->unlock("image already in cache", __FILE__, __LINE__); 
      return;
    } else {
      CACHEDEBUG(str(boost::format("Hmm, region didn't match.  cached region = %1%, region = %2%") % string(cachedImage->image->loadedRegion) % string(*region))); 
    }
    
  }
  
  /* Look for the job in the PendingQueue and JobQueue */
  ImageCacheJobPtr job = FindJobInQueue(mThreads[threadnum]->mPendingQueue, frameNumber, region, levelOfDetail);
  if (job) {
    mThreads[threadnum]->unlock("job found in pending queue", __FILE__, __LINE__); 
    return; 
  }else {
    job = FindJobInQueue(mThreads[threadnum]->mJobQueue, frameNumber, region, levelOfDetail);
    if (job) {
      mThreads[threadnum]->unlock("job found in job queue", __FILE__, __LINE__); 
      return; 
    }
  }

  CACHEDEBUG ("Frame %d: not in cache or in any queue, creating new job", frameNumber); 
  /* If we get this far, there is no such image in the cache, and no such
   * job in the queue, so add a new one.
   */
  ImageCacheJobPtr newJob; 
  newJob.reset(new ImageCacheJob(frameNumber, region, 
                                 levelOfDetail, mRequestNumber, 
                                 mFrameList->getFrame(frameNumber)));
  
  mRequestNumber++; // used to decide which image in a full cache to discard
  /* This counts as an additional cache request; the job will take
   * on the request number, so we can determine which entries are the
   * oldest.  When the image is taken from the queue, it will be given
   * a more recent request number.
   */
  
  /* We save the frameInfo information just in case the FrameList changes
   * while one of the reader threads is trying to read this frame.
   * The results of the image read will be discarded (via the
   * validRequestThreshold mechanism); this just keeps us from
   * dumping core on a bad pointer.
   */
  
  /* Add the job to the back of the work queue */
  //lock("adding new job to work queue", __FILE__, __LINE__);
  mThreads[threadnum]->mJobQueue.push_back(newJob); 
  sort(mThreads[threadnum]->mJobQueue.begin(), mThreads[threadnum]->mJobQueue.end(), jobComparer);
  
  CACHEDEBUG(QString("Added new job for frame %1 and region %2 to job queue").arg(frameNumber).arg(QString(*region))); 
  PrintJobQueue(mThreads[threadnum]->mJobQueue); 

  mThreads[threadnum]->unlock("new job added", __FILE__, __LINE__); 
  //mThreads[threadnum]->Print("new job added"); 
  /* If there's a worker thread that's snoozing, this will
   * wake him up.
   */
  mThreads[threadnum]->WakeAllJobReady("new job added", __FILE__, __LINE__); 
  return; 
}



