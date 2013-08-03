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
#include "convert.h"
#include "util.h"
#include "errmsg.h"

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



// ======================================================================
#define NEW_CACHE 1
#ifdef NEW_CACHE
/*
 * Evalute the "distance" between two frame numbers and return a score.
 * The higher the score, the farther apart the frames are in sequence.
 * In cache replacement, we'll use the highest score to determine the victim.
 */
static unsigned int Distance(unsigned int oldFrame, unsigned int newFrame,
                             unsigned int totalFrames)
{
  if (oldFrame > newFrame) {
    /* wrap-around; use modular/ring/clock arithmetic */
    unsigned int d1 = oldFrame - newFrame;
    unsigned int d2 = newFrame + totalFrames - oldFrame;
    return MIN2(d1, d2);
  }
  else {
    return newFrame - oldFrame;
  }
}
#endif


// ======================================================================
/* This is the work function, which is used by child threads to 
 * "asynchronously" load images from disk.  Not used in single-threaded case. 
 */

void CacheThread::run() {
  ImagePtr image;
  ImageCacheJobPtr job;
  CachedImagePtr imageSlot;
  CACHEDEBUG("CacheThread::run() (thread = %p)", QThread::currentThread()); 

  RegisterThread(this); // see common.cpp -- used to create a 0-based index that can be referenced anywhere. 

  /* Repeat forever, until the thread is cancelled by the main thread */
  while (1) {

    /* Try to get a job off the work queue.  We want to make sure
     * we're the only ones trying to do so at this particular
     * momemt, so grab the mutex.
     */
    mCache->lock("check for job in work queue", __FILE__, __LINE__);
            
    /* Wait for a job to appear on the work queue (which we can do
     * because we currently hold the mutex for the queue).  This is
     * in a "while" loop because sometimes a thread can be woken up
     * spuriously (by signal) or unwittingly (if all threads are woken
     * up on the work ready condition, but one has already grabbed the
     * work).
     */
    while (! this->mCache->mJobQueue.size()) {
      /* Wait for work ready condition to come in. */
      this->mCache->WaitForJobReady("no job in work queue yet", __FILE__, __LINE__); 
      if (mStop) {
        CACHEDEBUG("Thread %p terminating", QThread::currentThread()); 
        UnregisterThread(this); 
        mCache->unlock("thread is dying", __FILE__, __LINE__); 
        return; 
      }
    }
    // take the job from head of the queue: 
    job = this->mCache->mJobQueue.at(0); 
    CACHEDEBUG("Worker moving job %d from job queue to pending cue", job->frameNumber); 
    this->mCache->mJobQueue.pop_front(); 
    mCache->PrintJobQueue(mCache->mJobQueue); 

	/* Add the job removed from the job queue to the pending queue,
	 * so that the main thread can tell the job is in progress.
	 */
    this->mCache->mPendingQueue.push_back(job); 
    mCache->PrintJobQueue(mCache->mPendingQueue); 
	/* We don't need the mutex (or the cancellation function) any more;
	 * this call will both clear the cancellation function, and cause
	 * it to be called to unlock the mutex, more or less simultaneously.
	 */
    mCache->unlock("found job and added to pending queue", __FILE__, __LINE__); 

	/* Do the work.  Note that this function could cause warning and
	 * error output, and could still return an empty ImagePtr.  Even if an
	 * empty ImagePtr comes back, we have to record it, so that the 
	 * boss thread can return.
	 */
    CACHEDEBUG("Worker thread calling LoadAndConvertImage for frame %d", job->frameNumber); 
    image = job->frameInfo->LoadAndConvertImage
      (job->frameNumber, &mCache->mRequiredImageFormat, 
       &job->region, job->levelOfDetail);

 
	/* With image in hand (or possibly a NULL), we're ready to start
	 * modifying caches and queues.  We always will have to remove
	 * the job from the PendingQueue.
	 */
    this->mCache->lock("finished job, updating queues", __FILE__, __LINE__); 

    CACHEDEBUG("RemoveJobFromPendingQueue frame %d", job->frameNumber); 
	this->mCache->RemoveJobFromPendingQueue(job);
    mCache->PrintJobQueue(mCache->mPendingQueue);

	/* First see if this request has been invalidated (because the
	 * main thread changed the frame list while we were working).
	 * If so, there's nothing to do with the image.
	 */
	if (job->requestNumber <= this->mCache->mValidRequestThreshold) {
      CACHEDEBUG("Destroying useless job frame %d, because job request number %d is less than or equal to the cache valid request threshold %d", job->frameNumber, job->requestNumber, this->mCache->mValidRequestThreshold); 
      /* The job is useless.  Destroy the image, if we got one,
       * and go home.  We don't need to send an indicator to
       * anyone; the main thread doesn't care.
       */
      this->mCache->unlock("useless job", __FILE__, __LINE__); 
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
      this->mCache->mErrorQueue.push_back(job); 
      mCache->PrintJobQueue(mCache->mErrorQueue);
      this->mCache->unlock("error in job", __FILE__, __LINE__); 
      this->mCache->WakeAllJobDone("error in job",__FILE__, __LINE__); 
	}
	else if (!(imageSlot = mCache->GetCachedImageSlot(job->frameNumber))) {
      CACHEDEBUG("No place to put job frame %d!", job->frameNumber); 
      /* We have an image, but no place to put it.  This
       * is an error as well, that the main thread has
       * to be made aware of.  As usual, the error has
       * already been reported by here, so we don't
       * have to report another one.
       */
      this->mCache->mErrorQueue.push_back(job); 
      mCache->PrintJobQueue(mCache->mErrorQueue);

      this->mCache->unlock("no place for job", __FILE__, __LINE__); 
      this->mCache->WakeAllJobDone("no place for job",__FILE__, __LINE__);    
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
      imageSlot->lockCount = 0;

      this->mCache->unlock("job success", __FILE__, __LINE__);
      this->mCache->WakeAllJobDone("job success", __FILE__, __LINE__); 
 
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

//==================================================================
ImageCachePtr CreateImageCache(int numReaderThreads, int maxCachedImages, ImageFormat &required)
{
  ImageCachePtr newCache(new ImageCache(numReaderThreads, maxCachedImages, required));
  if (!newCache) {
	SYSERROR("cannot allocate image cache");
  }
  return newCache; 
}
//==================================================================
ImageCache::ImageCache(int numthreads, int numimages, ImageFormat &required): 
  mNumReaderThreads(numthreads), mMaxCachedImages(numimages), 
  mRequiredImageFormat(required), mRequestNumber(0), 
  mValidRequestThreshold(0), 
  mCachedImages(numimages), 
  mHighestFrameNumber(0), mPreloadFrames(0),  mCurrentPlayDirection(0),
  mCurrentStartFrame(0),  mCurrentEndFrame(0) {
  CACHEDEBUG("ImageCache constructor"); 
  
  register int i;
  CACHEDEBUG("CreateImageCache(mNumReaderThreads = %d, mMaxCachedImages = %d)", mNumReaderThreads, mMaxCachedImages);
  
  for (i = 0; i < mMaxCachedImages; i++) {
    mCachedImages[i].reset(new CachedImage()); 
  }
  if (mNumReaderThreads > 0) {
    
    /* Now create the threads.  If we fail to create a thread, we can still
     * continue with fewer threads or with none (in the single-threaded case).
     */
    for (i = 0; i < mNumReaderThreads; i++) {
      mThreads.push_back(CacheThreadPtr(new CacheThread(this))); 
      mThreads[i]->start(); 
    }
  }
  
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

	ClearJobQueue();

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
    mThreads.clear(); 
    
	/* Clear the remaining queues */
    ClearQueue(mPendingQueue);
	ClearQueue(mErrorQueue);

  }
    
  /* Any images in the cache must be released */
  ClearImages();

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
CachedImagePtr ImageCache::GetCachedImageSlot(uint32_t newFrameNumber)
{
  CachedImagePtr imageSlot;
#ifdef NEW_CACHE
  unsigned long maxDist = 0;
#endif
  CACHEDEBUG("GetCachedImageSlot(%d)", newFrameNumber); 

  for (vector<CachedImagePtr>::iterator cachedImage = mCachedImages.begin(); 
       cachedImage != mCachedImages.end();  cachedImage++) {
#ifdef NEW_CACHE
    /* Look for an empty slot, or a slot who's frame number is
     * furthest away from the one about to be loaded.
     */
    if ((*cachedImage)->lockCount == 0) {
      unsigned long dist;
      
      if ((*cachedImage)->requestNumber == 0) {
        imageSlot = *cachedImage;
        break;
      }
      
      dist = Distance((*cachedImage)->frameNumber, newFrameNumber,
                      mHighestFrameNumber);
      if (dist > maxDist){
        maxDist = dist;
        imageSlot = *cachedImage; 
      }
    }
#else
    /* Look for the best image to replace.  We're allowed to replace any
     * image that is not locked; if we have a choice, we'll choose one
     * that has not been loaded (and hence has a requestNumber of 0)
     * or that has not been used in the longest time (and hence has
     * the lowest request number).
     */
    if ((*cachedImage)->lockCount == 0 &&
        (!imageSlot ||
         (*cachedImage)->requestNumber < imageSlot->requestNumber ) ) {
      imageSlot = cachedImage;
    }
#endif
  }
  
  /* If we couldn't find an image slot, something's wrong. */
  if (!imageSlot) {
    ERROR("image cache is full, with all %d images locked",
          mMaxCachedImages);
    return imageSlot;
  }
    
  CACHEDEBUG("Removing frame %u to make room for %u  max %u", 
             imageSlot->frameNumber, newFrameNumber, mHighestFrameNumber);
  Print();

  /* Otherwise, if we found a slot that wasn't vacant, clear it out
   * before returning it.
   */
  if (imageSlot->loaded) {
    imageSlot->image.reset();
    imageSlot->loaded = 0;
  }

  return imageSlot;
}
//=============================================================
void ImageCache::ClearQueue(deque<ImageCacheJobPtr> &queue) {
  queue.clear(); 
}

//=============================================================
ImageCacheJobPtr ImageCache::
FindJobInQueue(deque<ImageCacheJobPtr> &queue,  unsigned int frameNumber,
               const Rectangle *region,  unsigned int levelOfDetail) {
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
void ImageCache::
RemoveJobFromQueue(deque<ImageCacheJobPtr> &queue, ImageCacheJobPtr job) {
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
  mCachedImages.clear(); 
  mCachedImages.resize(mMaxCachedImages); 
  for (int32_t i = 0; i < mMaxCachedImages; i++) {
    mCachedImages[i].reset(new CachedImage()); 
  }

  mHighestFrameNumber = 0;
}


//=============================================================
void ImageCache::ClearJobQueue(void)
{
  /* All pending work jobs must be cancelled */
  lock("clearJobQueue", __FILE__, __LINE__); 
  ClearQueue(mJobQueue); 
  /* All pending requests that have a request number below or equal
   * to the threshold will be discarded when complete.
   */
  mValidRequestThreshold = mRequestNumber;
  unlock("JobQueue cleared", __FILE__, __LINE__); 
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
    ClearJobQueue();
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
/* This routine looks for an image already in the image cache, and returns
 * the cached image slot itself, or an empty CachedImagePtr if not found.
 * Note: we don't care about the loaded image region at this point.
 */
  
CachedImagePtr ImageCache::FindImage(uint32_t frame, uint32_t lod) {
  CACHEDEBUG("FindImage frame %d", frame); 
  /* Search the cache to see whether an appropriate image already
   * exists within the cache.
   */
  for (vector<CachedImagePtr>::iterator cachedImage = mCachedImages.begin(); 
       cachedImage != mCachedImages.end();  cachedImage++) {
    if ((*cachedImage)->loaded &&
        (*cachedImage)->frameNumber == frame &&
        (*cachedImage)->levelOfDetail == lod) {
      CACHEDEBUG("Found frame number %d", frame); 
      return *cachedImage;
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
                              const Rectangle *newRegion, uint32_t levelOfDetail)
{

  ImagePtr image;
  CachedImagePtr cachedImage;
  CachedImagePtr imageSlot;
  //    int rv;
  Rectangle region = *newRegion;
  gCurrentFrame = frameNumber; 
  
  if (!mFrameList) {
    WARNING("frame %d requested from an empty cache",
            frameNumber);
    return ImagePtr();
  }
  if (frameNumber > mFrameList->numActualFrames()) {
    WARNING("image %d requested from a cache managing %d images",
            frameNumber, mFrameList->numActualFrames() + 1);
	return image;
  }

  CACHEDEBUG("ImageCache::GetImage frame %d\n",frameNumber); 
  
  /* Keep track of highest frame number.  We need it for cache
   * replacement.
   */
  if (frameNumber > mHighestFrameNumber)
    mHighestFrameNumber = frameNumber;
  
  /* This counts as an additional cache request; we keep track of such
   * things so we can decide which of the cache entries is the oldest.
   */
  mRequestNumber++;

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
    lock("checking for ready image", __FILE__, __LINE__);
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
		/* This image is appropriate.  Mark our interest in the frame
		 * (so that it is not pulled out from under us while we're
		 * still using it), and return it.
		 */
		bb_assert(cachedImage->image->imageFormat.rowOrder != ROW_ORDER_DONT_CARE);
        
		cachedImage->requestNumber = mRequestNumber;
		//cachedImage->lockCount++;
		if (mNumReaderThreads > 0) {
          cachedImage->lockCount = 1;
          unlock("found and locked interesting frame", __FILE__, __LINE__); 
		}
        CACHEDEBUG("Returning found image %d", frameNumber); 
		return cachedImage->image;
      }
      else {
        CACHEDEBUG("Frame %d does not fully match, so augment rectangle", frameNumber); 
        if (cachedImage->lockCount) cachedImage->lockCount=0;
        region = RectUnionRect(&cachedImage->image->loadedRegion, &region);
      }
	}
    CACHEDEBUG("Frame %d not found, look for it in queues", frameNumber); 
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
      CACHEDEBUG("Looking in pending queue for frame %d", frameNumber); 
      ImageCacheJobPtr job = 
        FindJobInQueue(mPendingQueue, frameNumber, &region,levelOfDetail);
      
      if (job) {
		/* We've got a job coming soon that will give us this image.
		 * Just wait for the reader thread to complete.  This should
		 * kick us back to the top of the loop when we return, with
		 * the imageCacheLock already re-acquired.
		 */
        WaitForJobDone("waiting for pending job", __FILE__, __LINE__); 
      }
      else {
		/* Look for a matching job in the work queue, one that may
		 * have been pre-loaded earlier.  We're a little less picky
		 * here; the job we find can refer to any region.  We're going
		 * to modify the job request, if necessary, to include the
		 * desired region.
		 */
        CACHEDEBUG("Looking in job queue for frame %d with other rectangle", frameNumber); 
        job = FindJobInQueue(mJobQueue, frameNumber, NULL, levelOfDetail);
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
          RemoveJobFromQueue(mJobQueue, job); 
          mJobQueue.push_front(job); 
          
          /* Now wait for an image to appear. */
          WaitForJobDone("waiting for modified pending job", __FILE__, __LINE__); 
		}
		else {
          /* No job in the pending queue nor in the work queue.  Check
           * the error queue to make sure the job hasn't been attempted
           * but has failed.
           */
          CACHEDEBUG("No job for image %d in the pending queue nor in the work queue", frameNumber); 
          job = FindJobInQueue(mErrorQueue, frameNumber,&region,levelOfDetail);
          if (job) {
			/* The error, hopefully, has already been reported.  We
			 * have to remove the job from the queue and return to
			 * the caller with an error.
			 */
			RemoveJobFromQueue(mErrorQueue, job);
            
            unlock("found error in job", __FILE__, __LINE__);
			return image;
          }
          else {
			/* No job anywhere.  Break out of the condition
			 * variable loop so the main thread can load the
			 * image itself.
			 */
            CACHEDEBUG("No job for image %d anywhere, main thread must do it or something, shrug", frameNumber); 
            unlock("no job found", __FILE__, __LINE__); 
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
  CACHEDEBUG("Have to load image %d in main thread", frameNumber); 
   
  /* The requested image is valid enough.  If we're single-threaded, load the 
   * image ourselves; if we're multi-threaded, add work to the work queue and 
   * wait for an image to become ready.
   */
  FrameInfoPtr fip = mFrameList->getFrame(frameNumber); 
  image = fip-> LoadAndConvertImage(frameNumber, 
                                    &mRequiredImageFormat, 
                                    &region, levelOfDetail);
  if (!image) {
	return image;
  }
  bb_assert(image->imageFormat.rowOrder != ROW_ORDER_DONT_CARE);

  /* We have an image; we'll add it to the cache ourselves.  Find
   * a slot we can stick it in.
   */
  if (mNumReaderThreads > 0) {
    lock("adding image to cache in main thread", __FILE__, __LINE__);
  }

  /* Either update the current image slot or find a new one */
  if (cachedImage) {
    imageSlot = cachedImage;
  }
  else {
#if 0 && DEBUG
	/* this image better not already be in the cache! */
	int i;
	CachedImagePtr img;
	for (i = 0, img = mCachedImages; i < mMaxCachedImages;
	     i++, img++) {
      if (img->lockCount > 0) {
		bb_assert(img->image != image);
      }
	}
#endif
    imageSlot = GetCachedImageSlot(frameNumber);
  }

  if (!imageSlot) {
	/* We have an image, but no place to put it!
	 * The error has been reported; destroy the image
	 * and return empty image.
	 */
	if (mNumReaderThreads > 0) {
      unlock("no place for image", __FILE__, __LINE__); 
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
  imageSlot->lockCount = 1;
  imageSlot->requestNumber = mRequestNumber;
  if (mNumReaderThreads > 0) {
    unlock("image stored and locked successfully", __FILE__, __LINE__); 
  }
    
  /* Steal Preload code right from Renderer */ 
  uint32_t preloadmax = mCurrentEndFrame-mCurrentStartFrame, 
    frame=frameNumber, preloaded=0;
  if (preloadmax > mPreloadFrames) {
    preloadmax = mPreloadFrames;
  }

  /* Check to see how much room we have in the cache and free up any if needed or just return without more preloading.  */
  // if ((mJobQueue.size() + mPendingQueue.size() + preloadmax) * mAverageFrameBytes > mMaxCacheBytes) {
      
  while (preloaded++ < preloadmax) {
    frame += mCurrentPlayDirection; 
    if (frame > mCurrentEndFrame) frame = mCurrentEndFrame; 
    if (frame < mCurrentStartFrame) frame = mCurrentStartFrame; 
    DEBUGMSG("Preload frame %d", frame); 
    PreloadImage(frame, newRegion, levelOfDetail);
  }
    
  return image;
}

// ======================================================================
/* 
 * I do not think this is needed any more because we are using 
 * boost::shared_ptrs to manage images etc.  
 * This routine is poorly named.  It unlocks the specified image, so that its
 * entry can be re-used, but only "if no one else is using it", 
 * whatever that means.  The entire cache needs a rewrite.  
 * Callers should not need to understand the cache internals to use it.  
 * Encapsulation is a problem here.  
 * Unfortunately, the actual image stays around; it will only actually be deleted
 * if the cache is destroyed or if the space is required
 * for another image.  This makes it hard to know when an image is actually released.  
 */
void ImageCache::DecrementLockCount(ImagePtr image)
{
  CACHEDEBUG("DecrementLockCount one image, frame %d", image->frameNumber); 
  
  // OLD CODE BELOW -- boost is probably doing fine with all this
  //int rv;
  /* Look for the given image in the cache. */
  if (mNumReaderThreads > 0) {
    lock("DecrementLockCount on an image", __FILE__, __LINE__);
  }
  for (vector<CachedImagePtr>::iterator cachedImage = mCachedImages.begin(); 
       cachedImage != mCachedImages.end();  cachedImage++) {
	if ((*cachedImage)->image == image) {
      CACHEDEBUG("Releasing frame %d from cache", (*cachedImage)->frameNumber); 
      /* Unlock it and return. */
      if ((*cachedImage)->lockCount == 0) {
        CACHEDEBUG("unlocking an unlocked image, frame %d",
                   (*cachedImage)->frameNumber);
        
      }
      else {
		(*cachedImage)->lockCount--;
        CACHEDEBUG("Image for frame %d has new lock count %d", (*cachedImage)->frameNumber, (*cachedImage)->lockCount); 
      }
      if (mNumReaderThreads > 0) {
        unlock("image released", __FILE__, __LINE__); 
      }
      return;
	}
  }
  
  /* If we get here, we couldn't find the image */
  unlock("no such image", __FILE__, __LINE__); 
  return;
}

// ======================================================================
/*!
  Releases all images associated with the given frame number.  Does not know about stereo, uses actual frame numbers, not stereo frame numbers.
*/ 
void ImageCache::DecrementLockCount(int frameNumber) {
  register int numreleased = 0, i=0;
  CACHEDEBUG("DecrementLockCount frame %d", frameNumber); 
  //int rv;
  /* Look for the given image in the cache. */
  if (mNumReaderThreads > 0) {
    lock("releasing an image", __FILE__, __LINE__);
  }
  for (vector<CachedImagePtr>::iterator cachedImage = mCachedImages.begin(); 
       cachedImage != mCachedImages.end();  cachedImage++, i++) {
	if ((int)(*cachedImage)->frameNumber == frameNumber) {
      CACHEDEBUG("Releasing frame slot %d from cache", i); 
      /* Unlock it and return. */
      if ((*cachedImage)->lockCount == 0) {
        CACHEDEBUG("unlocking an unlocked image, slot %d",i);
        
      }
      else {
		(*cachedImage)->lockCount = 0;
        numreleased++; 
        CACHEDEBUG("Image slot %d new lock count %d", i, (*cachedImage)->lockCount); 
      }
	}
  }
  CACHEDEBUG("decremented lockcount on %d images", numreleased); 

  if (mNumReaderThreads > 0) {
    unlock("image(s) decremented", __FILE__, __LINE__); 
  }
  return;
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
  lock("checking if image already exists", __FILE__, __LINE__);
  cachedImage = FindImage(frameNumber, levelOfDetail);
  if (cachedImage) {
    CACHEDEBUG("Found match for frame number %d in cache", frameNumber); 
    if (RectContainsRect(&cachedImage->image->loadedRegion, region)) {
      /* Image is already in cache - no need to preload again */
      unlock("image already in cache", __FILE__, __LINE__); 
      return;
    } else {
      CACHEDEBUG(QString("Hmm, region didn't match.  cached region = %1, region = %2").arg(cachedImage->image->loadedRegion.toString()).arg(region->toString())); 
    }
    
  }
  
  /* Look for the job in the PendingQueue and JobQueue */
  ImageCacheJobPtr job = FindJobInQueue(mPendingQueue, frameNumber, region, levelOfDetail);
  if (job == NULL) {
    job = FindJobInQueue(mJobQueue, frameNumber, region, levelOfDetail);
  }
  if (job) {
    unlock("job already in a queue", __FILE__, __LINE__); 
    CACHEDEBUG("The job exists already - no need to add a new one"); 
	return;
  }
  CACHEDEBUG ("Frame %d: not in cache or in any queue, creating new job", frameNumber); 
  /* If we get this far, there is no such image in the cache, and no such
   * job in the queue, so add a new one.
   */
  ImageCacheJobPtr newJob; 
  newJob.reset(new ImageCacheJob(frameNumber, region, 
                                 levelOfDetail, mRequestNumber, 
                                 mFrameList->getFrame(frameNumber)));
  
  /* This counts as an additional cache request; the job will take
   * on the request number, so we can determine which entries are the
   * oldest.  When the image is taken from the queue, it will be given
   * a more recent request number.
   */
  mRequestNumber++;
  
  /* We save the frameInfo information just in case the FrameList changes
   * while one of the reader threads is trying to read this frame.
   * The results of the image read will be discarded (via the
   * validRequestThreshold mechanism); this just keeps us from
   * dumping core on a bad pointer.
   */
  
  /* Add the job to the back of the work queue */
  //lock("adding new job to work queue", __FILE__, __LINE__);
  mJobQueue.push_back(newJob); 
  sort(mJobQueue.begin(), mJobQueue.end(), jobComparer);
  
  CACHEDEBUG(QString("Added new job for frame %1 and region %2 to job queue").arg(frameNumber).arg(region->toString())); 
  PrintJobQueue(mJobQueue); 

  unlock("new job added", __FILE__, __LINE__); 
  Print(); 
  /* If there's a worker thread that's snoozing, this will
   * wake him up.
   */
  WakeAllJobReady("new job added", __FILE__, __LINE__); 
  return; 
}


// ======================================================================
/* For debugging */
void ImageCache::Print(void)
{
  register int i, numlocked=0;
  QString msg; 
  CACHEDEBUG("Printing cache state."); 
  for (vector<CachedImagePtr>::iterator cachedImage = mCachedImages.begin(); 
       cachedImage != mCachedImages.end();  cachedImage++, i++) {
    msg = QString("  Slot %1: lockCount:%2  frame:%3 lod:%4  req:%5  ")
      .arg( i)
      .arg((*cachedImage)->lockCount)
      .arg((*cachedImage)->frameNumber)
      .arg((*cachedImage)->levelOfDetail)
      .arg((*cachedImage)->requestNumber);
    if ((*cachedImage)->image) {
      msg += QString("roi:%1").arg((*cachedImage)->image->loadedRegion.toString()); 
    }
    if ((*cachedImage)->lockCount) numlocked++; 
    CACHEDEBUG(msg); 
  }
  CACHEDEBUG("mHighest = %d, numlocked = %d", mHighestFrameNumber, numlocked); 
}

/*!
  You probably want to call the macro, PrintJobQueue(q)
*/
void ImageCache::__PrintJobQueue(QString name, deque<ImageCacheJobPtr>&q) {
  deque<ImageCacheJobPtr>::iterator pos = q.begin(), endpos = q.end(); 
  CACHEDEBUG(QString(" deque<ImageCacheJob *>%1 (length %2): ").arg(name).arg(q.size())); 
  while (pos != endpos){
    CACHEDEBUG((*pos)->toString()); 
    ++pos; 
  }
  return; 
}
