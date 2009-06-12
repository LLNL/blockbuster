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
#include "canvas.h"
#include "queue.h"


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

#ifdef DEBUG
#define pthread_message(call,description) \
  CACHEDEBUG("starting %s...", description);                     \
  call;                                                          \
  CACHEDEBUG("done with %s", description);                     

#define pthread_mutex_init(arg1,arg2) \
  pthread_message(rv=pthread_mutex_init(arg1, arg2), "pthread_mutex_init");

#define pthread_mutex_lock(mutex)                                \
  pthread_message(rv=pthread_mutex_lock(mutex), "pthread_mutex_lock");

#define pthread_mutex_unlock(mutex) \
  pthread_message(rv=pthread_mutex_unlock(mutex), "pthread_mutex_unlock");

#define pthread_mutex_destroy(mutex)                                \
  pthread_message(rv=pthread_mutex_destroy(mutex), "pthread_mutex_destroy");

#define pthread_cond_init(arg1,arg2) \
  pthread_message(rv=pthread_cond_init(arg1, arg2), "pthread_cond_init");

#define pthread_cond_wait(arg1,arg2) \
  pthread_message(rv=pthread_cond_wait(arg1, arg2), "pthread_cond_wait");

#define pthread_cond_signal(arg1) \
  pthread_message(rv=pthread_cond_signal(arg1), "pthread_cond_signal");

#define pthread_cond_destroy(cond)                                \
  pthread_message(rv=pthread_cond_destroy(cond), "pthread_cond_destroy");



#endif

#define ERROR_STRING(x) (                       \
    (x==EINVAL)?"EINVAL":\
    (x==EDEADLK)?"EDEADLK":\
    (x==EBUSY)?"EBUSY":\
    (x==EPERM)?"EPERM":\
    (x==ETIMEDOUT)?"ETIMEDOUT":\
    (x==EINTR)?"EINTR":\
    "unknown")

/* Default/fallback routine for Canvas->Preload()
 */
void CachePreload(Canvas *canvas, uint32_t frameNumber, const Rectangle *imageRegion,
	uint32_t levelOfDetail)
{
    Rectangle lodROI;
    uint32_t localFrameNumber = 0;

    /* Adjust ROI for LOD! (very important) */
    lodROI.x = imageRegion->x >> levelOfDetail;
    lodROI.y = imageRegion->y >> levelOfDetail;
    lodROI.width = imageRegion->width >> levelOfDetail;
    lodROI.height = imageRegion->height >> levelOfDetail;

	
	if(canvas->frameList->stereo) {
	  localFrameNumber = frameNumber * 2;
	  PreloadImageIntoCache(canvas->imageCache, localFrameNumber++,
							&lodROI, levelOfDetail);
	  PreloadImageIntoCache(canvas->imageCache, localFrameNumber,
							&lodROI, levelOfDetail);
	}
	else {
	  localFrameNumber = frameNumber;
	  PreloadImageIntoCache(canvas->imageCache, localFrameNumber,
							&lodROI, levelOfDetail);
	}   
}

/* This utility function handles loading and converting images appropriately,
 * for both the single-threaded and multi-threaded cases.
 */
#define MAX_CONVERSION_WARNINGS 10
static Image *LoadAndConvertImage(FrameInfo *frameInfo, unsigned int frameNumber,
	Canvas *canvas, const Rectangle *region, int levelOfDetail)
{
    Image *image, *convertedImage;
    int rv;
    static int conversionCount = 0;

    if (!frameInfo->enable) return NULL;

    image = (Image *) calloc(1, sizeof(Image));
    if (!image) {
	ERROR("Out of memory in LoadAndConvertImage");
	return NULL;
    }
    image->ImageDeallocator = DefaultImageDeallocator;

    /* Call the file format module to load the image */
    rv = (*frameInfo->LoadImage)(image, frameInfo, canvas,
                                 region, levelOfDetail);

    if (!rv) {
	ERROR("could not load frame %d (frame %d of file name %s) for the cache",
	    frameNumber,
	    frameInfo->frameNumber,
	    frameInfo->filename
	);
	frameInfo->enable = 0;
	free(image);
	return NULL;
    }

    /* The file format module, which loaded the image for us, tried to
     * do as good a job as it could to match the format our canvas
     * requires, but it might not have succeeded.  Try to convert
     * this image to the correct format.  If the same image comes
     * back, no conversion was necessary; if a NULL image comes back,
     * conversion failed.  If any other image comes back, we can
     * discard the original, and use the conversion instead.
     */
    convertedImage = ConvertImageToFormat(image, canvas);
    if (convertedImage == image) {
	/* No conversion!  Great! */
    }
    else {
	/* We either converted, or failed to convert; in either
	 * case, the old image is useless.
	 */
	(*image->ImageDeallocator)(canvas, image);

	image = convertedImage;
	if (image == NULL) {
	    ERROR("failed to convert frame %d", frameNumber);
	}
	else {
	    conversionCount++;
	    if (conversionCount < MAX_CONVERSION_WARNINGS) {
		/* We'll issue a warning anyway, as conversion is an expensive
		 * process.
		 */
		WARNING("had to convert frame %d", frameNumber);

		if (conversionCount +1 == MAX_CONVERSION_WARNINGS) {
		    WARNING("(suppressing further conversion warnings)");
		}
	    }
	}
    }

    /* If we have a NULL image, the converter already reported it. */
    return image;
}

/* This is a cleanup function which will be called if and when
 * a reader thread is cancelled.  All it has to do is release
 * the mutex it was holding.
 */
/*void ReaderCleanup(void *)
{
    int rv;
    CACHEDEBUG("ReaderCleanup"); 
    if (rv != 0) WARNING("pthread_mutex_unlock returned %d (%s)",
	    rv, ERROR_STRING(rv));
}
*/

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

/*
 * Allocate a cache slot.  If the cache is full, an existing frame will have
 * to be discarded.
 * Input: newFrameNumber - indicates the number of the new frame we want to
 * insert into the cache.  We use this to determine which entry to discard
 * if the cache is full.
 */
static CachedImage *GetCachedImageSlot(ImageCache *cache,
                                       unsigned int newFrameNumber)
{
    register CachedImage *cachedImage;
    register int i;
    CachedImage *imageSlot = NULL;
#ifdef NEW_CACHE
    unsigned long maxDist = 0;
#endif

    for (
	i = 0, cachedImage = cache->cachedImages; 
	i < cache->maxCachedImages; 
	i++, cachedImage++
    ) {
#ifdef NEW_CACHE
	/* Look for an empty slot, or a slot who's frame number is
	 * furthest away from the one about to be loaded.
	 */
	if (cachedImage->lockCount == 0) {
	    unsigned long dist;

	    if (cachedImage->requestNumber == 0) {
		imageSlot = cachedImage;
		break;
	    }

	    dist = Distance(cachedImage->frameNumber, newFrameNumber,
                            cache->highestFrameNumber);
	    if (dist > maxDist){
		maxDist = dist;
		imageSlot = cachedImage;
	    }
	}
#else
	/* Look for the best image to replace.  We're allowed to replace any
	 * image that is not locked; if we have a choice, we'll choose one
	 * that has not been loaded (and hence has a requestNumber of 0)
	 * or that has not been used in the longest time (and hence has
	 * the lowest request number).
	 */
	if (
	    cachedImage->lockCount == 0 &&
	    (
		imageSlot == NULL ||
		cachedImage->requestNumber < imageSlot->requestNumber
	    )
	) {
	    imageSlot = cachedImage;
	}
#endif
    }

    /* If we couldn't find an image slot, something's wrong. */
    if (imageSlot == NULL) {
	ERROR("image cache is full, with all %d images locked",
		cache->maxCachedImages);
	return NULL;
    }

#if 0
    printf("Removing frame %u to make room for %u  max %u\n", 
           imageSlot->frameNumber, newFrameNumber, cache->highestFrameNumber);
    PrintCache(cache);
#endif

    /* Otherwise, if we found a slot that wasn't vacant, clear it out
     * before returning it.
     */
    if (imageSlot->loaded) {
	(*imageSlot->image->ImageDeallocator)(cache->canvas, imageSlot->image);
        imageSlot->image = NULL;
	imageSlot->loaded = 0;
    }

    return imageSlot;
}


/* This is the work function, which is used by child threads to 
 * "asynchronously" load images from disk.  These functions are not
 * used in the single-threaded case.
 */

#define POP_AND_EXECUTE 1

void CacheThread::run() {
  //static void *DoReaderThreadWork(void *data)
  //ImageCacheThreadInfo *threadInfo = (ImageCacheThreadInfo *)data;
  //    register ImageCache *cache = threadInfo->imageCache;
    Image *image;
    //    int rv;
    ImageCacheJob *job;
    CachedImage *imageSlot;
    CACHEDEBUG("CacheThread::run() (thread = %p)", QThread::currentThread()); 

    //pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

    /* Repeat forever, until the thread is cancelled by the main thread */
    while (1) {

	/* Try to get a job off the work queue.  We want to make sure
	 * we're the only ones trying to do so at this particular
	 * momemt, so grab the mutex.
	 */
      cache->lock(__FILE__, __LINE__);
 
	/* We could be cancelled at any time; we wouldn't want to be
	 * cancelled while holding the mutex, so install a cancellation
	 * cleanup function.
	 */
    //	pthread_cleanup_push(&ReaderCleanup, (void *) cache);

	/* Wait for a job to appear on the work queue (which we can do
	 * because we currently hold the mutex for the queue).  This is
	 * in a "while" loop because sometimes a thread can be woken up
	 * spuriously (by signal) or unwittingly (if all threads are woken
	 * up on the work ready condition, but one has already grabbed the
	 * work).
	 */
	while ((job = GetJobFromQueue(&this->cache->jobQueue)) == NULL) {
	    /* Wait for work ready condition to come in. */
      this->cache->WaitForJobReady(__FILE__, __LINE__); 
      if (mStop) {
        CACHEDEBUG("Thread %p terminating", QThread::currentThread()); 
        cache->unlock(__FILE__, __LINE__); 
        return; 
      }
      // rv = pthread_cond_wait(&cache->jobReady, &cache->imageCacheLock);
	  //  if (rv != 0) WARNING("pthread_cond_wait returned %d (%s)",
      //	rv, ERROR_STRING(rv));
	}

	/* Link the job removed from the job queue to the pending queue,
	 * so that the main thread can tell the job exists.
	 */
	AddJobToQueue(&this->cache->pendingQueue, job);

	/* We don't need the mutex (or the cancellation function) any more;
	 * this call will both clear the cancellation function, and cause
	 * it to be called to unlock the mutex, more or less simultaneously.
	 */
	// pthread_cleanup_pop(POP_AND_EXECUTE);
    cache->unlock(__FILE__, __LINE__); 

	/* Do the work.  Note that this function could cause warning and
	 * error output, and could still return a NULL image.  Even if a
	 * NULL image comes back, we have to record it, so that the 
	 * boss thread can return.
	 */
    CACHEDEBUG("Worker thread calling LoadAndConvertImage"); 
	image = LoadAndConvertImage(&job->frameInfo,
	    job->frameNumber, this->cache->canvas, &job->region, job->levelOfDetail);

	/* With image in hand (or possibly a NULL), we're ready to start
	 * modifying caches and queues.  We always will have to remove
	 * the job from the pendingQueue.
	 */
	/* rv = pthread_mutex_lock(&cache->imageCacheLock);
       if (rv != 0) WARNING("pthread_mutex_lock returned %d (%s)",
       rv, ERROR_STRING(rv));
    */ 
    this->cache->lock(__FILE__, __LINE__); // this should already be locked here
	RemoveJobFromQueue(&this->cache->pendingQueue, job);

	/* First see if this request has been invalidated (because the
	 * main thread changed the frame list while we were working).
	 * If so, there's nothing to do with the image.
	 */
	if (job->requestNumber <= this->cache->validRequestThreshold) {
	    /* The job is useless.  Destroy the image, if we got one,
	     * and go home.  We don't need to send an indicator to
	     * anyone; the main thread doesn't care.
	     */
      /* rv = pthread_mutex_unlock(&cache->imageCacheLock);
         if (rv != 0) WARNING("pthread_mutex_unlock returned %d (%s)",
         rv, ERROR_STRING(rv));
      */ 
      this->cache->unlock(__FILE__, __LINE__); 
	    if (image) {
		(*image->ImageDeallocator)(this->cache->canvas, image);
                image = NULL;
	    }
	}
	else if (image == NULL) {
	    /* The main thread has to be told that there was
	     * an error here; otherwise, it may be waiting forever
	     * for an image that never appears.  So put it on
	     * the error queue (which the main thread will
	     * check) and signal that the job is done.
	     */
	    AddJobToQueue(&this->cache->errorQueue, job);
        /*
          rv = pthread_mutex_unlock(&cache->imageCacheLock);
          if (rv != 0) WARNING("pthread_mutex_unlock returned %d (%s)",
          rv, ERROR_STRING(rv));
          rv = pthread_cond_signal(&cache->jobDone);
          if (rv != 0) WARNING("pthread_cond_signal returned %d (%s)",
          rv, ERROR_STRING(rv));
        */ 
        this->cache->unlock(__FILE__, __LINE__); 
        this->cache->WakeAllJobDone(__FILE__, __LINE__); 
	}
	else if ((imageSlot = GetCachedImageSlot(this->cache, job->frameNumber)) == NULL) {
	    /* We have an image, but no place to put it.  This
	     * is an error as well, that the main thread has
	     * to be made aware of.  As usual, the error has
	     * already been reported by here, so we don't
	     * have to report another one.
	     */
	    AddJobToQueue(&this->cache->errorQueue, job);
        /*
          rv = pthread_mutex_unlock(&cache->imageCacheLock);
          if (rv != 0) WARNING("pthread_mutex_unlock returned %d (%s)",
          rv, ERROR_STRING(rv));
          rv = pthread_cond_signal(&cache->jobDone);
          if (rv != 0) WARNING("pthread_cond_signal returned %d (%s)",
          rv, ERROR_STRING(rv));
        */ 
        this->cache->unlock(__FILE__, __LINE__); 
        this->cache->WakeAllJobDone(__FILE__, __LINE__); 
	    (*image->ImageDeallocator)(this->cache->canvas, image);
            image = NULL;
	}
	else {
	    /* Hey, things actually went okay; we have an image
	     * and a place to put it.  We can lose the job,
	     * and store the image.
	     */
	    imageSlot->frameNumber = job->frameNumber;
	    imageSlot->levelOfDetail = job->levelOfDetail;
	    imageSlot->image = image;
	    imageSlot->loaded = 1;
	    imageSlot->requestNumber = job->requestNumber;
	    imageSlot->lockCount = 0;
        /*
          rv = pthread_mutex_unlock(&cache->imageCacheLock);
          if (rv != 0) WARNING("pthread_mutex_unlock returned %d (%s)",
          rv, ERROR_STRING(rv));
          rv = pthread_cond_signal(&cache->jobDone);
          if (rv != 0) WARNING("pthread_cond_signal returned %d (%s)",
          rv, ERROR_STRING(rv));
        */ 
        this->cache->unlock(__FILE__, __LINE__);
        this->cache->WakeAllJobDone(__FILE__, __LINE__); 
	    free(job);

            /* Fairness among threads is greatly improved by yielding here.
             * Without this, threads can frequently starve for a while and
             * that causes prefetching to get out of order.
             * XXX this probably isn't portable.
             */
        CACHEDEBUG("sched_yield"); 
        sched_yield();
	}

    } /* loop forever, until the thread is cancelled */

    //    return NULL; /* this will never actually happen */
    return; 
}

ImageCache *CreateImageCache(int numReaderThreads, int maxCachedImages, Canvas *canvas)
{
    ImageCache *newCache;
    register int i;
    DEBUGMSG("CreateImageCache(numReaderThreads = %d, maxCachedImages = %d, canvas)", numReaderThreads, maxCachedImages);
    newCache = new ImageCache; //(ImageCache *)calloc(1, sizeof(ImageCache));
    if (newCache == NULL) {
	SYSERROR("cannot allocate image cache");
	return NULL;
    }

    newCache->cachedImages = (CachedImage *)calloc(1, maxCachedImages * sizeof(CachedImage));
    if (newCache->cachedImages == NULL) {
	ERROR("cannot allocate %d cached images", maxCachedImages);
	delete newCache;
	return NULL;
    }
    for (i = 0; i < maxCachedImages; i++) {
	newCache->cachedImages[i].requestNumber = 0;
	newCache->cachedImages[i].lockCount = 0;
	newCache->cachedImages[i].loaded = 0;
	newCache->cachedImages[i].image = NULL;
    }
    newCache->maxCachedImages = maxCachedImages;

    if (numReaderThreads > 0) {
      //      int rv;
      
      /* Lots of thread-specific initialization.  Start off with the queues,
       * which must be empty initially.
       */
      InitializeQueue(&newCache->jobQueue);
      InitializeQueue(&newCache->pendingQueue);
      InitializeQueue(&newCache->errorQueue);
      
      /* The mutexes (mutices? :-) and condition variables are next;
       * they must be initialized before the threads that need them
       * are created.
       */
      /*
        rv = pthread_mutex_init(&newCache->imageCacheLock, NULL);
        if (rv != 0) WARNING("pthread_mutex_init returned %d (%s)",
        rv, ERROR_STRING(rv));
        rv = pthread_cond_init(&newCache->jobReady, NULL);
        if (rv != 0) WARNING("pthread_cond_init returned %d (%s)",
        rv, ERROR_STRING(rv));
        rv = pthread_cond_init(&newCache->jobDone, NULL);
        if (rv != 0) WARNING("pthread_cond_init returned %d (%s)",
        rv, ERROR_STRING(rv));
      */
      /* Now create the threads.  If we fail to create a thread, we can still
       * continue with fewer threads or with none (in the single-threaded case).
       */
      /*
        newCache->mThreads = (ImageCacheThreadInfo *)calloc(1, numReaderThreads * sizeof(ImageCacheThreadInfo));
        if (newCache->mThreads == NULL) {
	    WARNING("cannot allocate %d thread info structures - reverting to single-threaded case", numReaderThreads);
	    numReaderThreads = 0;
        }
        else {
      */ 
      //        int rv;
        for (i = 0; i < numReaderThreads; i++) {
          newCache->mThreads.push_back( new CacheThread(i, newCache)); 
          newCache->mThreads[i]->start(); 
          /* newCache->threads[i].threadNumber = i;
             newCache->threads[i].imageCache = newCache;
             rv = pthread_create(&newCache->threads[i].threadId, NULL, 
             &DoReaderThreadWork, &newCache->threads[i]);
             CACHEDEBUG("Created thread %d with ID %d and rv=%d", i, newCache->threads[i].threadId, rv); 
             if (rv != 0) {
             SYSERROR("pthread_create returned %d", rv);
             if (i == 0) {
             // First thread failed - lose them all 
             WARNING("reverting to single-threaded case");
             free(newCache->threads);
             newCache->threads = NULL;
             pthread_mutex_destroy(&newCache->imageCacheLock);
             pthread_cond_destroy(&newCache->jobReady);
             pthread_cond_destroy(&newCache->jobDone);
             numReaderThreads = 0;
             }
             else {
             // We at least got some threads.  Keep the ones we have. 
             WARNING("only using %d threads (%d requested",
             i, numReaderThreads);
             numReaderThreads = i;
             }
             // In any case, on error stop trying to create more. 
             break;
             }
          */ 
        }
        //      }
    }
    /* else {
       newCache->threads = NULL;
       }
    */
    newCache->numReaderThreads = numReaderThreads;
    
    /* Compared to the above, these are easy to set */
    newCache->canvas = canvas;
    newCache->frameList = NULL;
    /* The validRequestThreshold is the number of the last request that
     * we shold ignore.  It starts at 0 (so we don't ignore any requests);
     * if something should happen that would invalidate pending work 
     * requests (say, by changing the FrameList while reader threads are
     * still reading images from the old FrameList), this will keep the
     * invalidated images from entering the image queue.
     */
    newCache->validRequestThreshold = 0;

    newCache->highestFrameNumber = 0;

    return newCache;
}

void GetCacheConfiguration(ImageCache *cache, int *numReaderThreads, int *maxCachedImages)
{
    if (numReaderThreads != NULL) 
	*numReaderThreads = cache!=NULL?cache->numReaderThreads:0;
    if (maxCachedImages != NULL) 
	*maxCachedImages = cache!=NULL?cache->maxCachedImages:0;
}

/* This function clears out all images still residing in the image
 * cache.  It is used when the cache is destroyed, or when its
 * intrinsic FrameList is changed (thus invalidating all frame
 * numbers and potentially the images as well).
 */
void ClearImageCache(ImageCache *cache)
{
    register int i;

    CACHEDEBUG("ClearImageCache"); 
    /* Any images in the cache must be released */
    for (i = 0; i < cache->maxCachedImages; i++) {
	/* There shouldn't be any locked images.  If there are,
	 * override the lock, but give a warning.
	 */
	if (cache->cachedImages[i].lockCount > 0) {
	    WARNING(
		"cached image %d forcibly unlocked while clearing cache",
		i);
	}
	if (cache->cachedImages[i].loaded) {
	    (cache->cachedImages[i].image->ImageDeallocator)(
		cache->canvas,
		cache->cachedImages[i].image
	    );
	    cache->cachedImages[i].image = NULL;
	    cache->cachedImages[i].loaded = 0;
	}
    }
    cache->highestFrameNumber = 0;
}

void ClearCacheWorkQueue(ImageCache *cache)
{
  //    int rv;
  CACHEDEBUG("ClearCacheWorkQueue"); 
    /* All pending work jobs must be cancelled */
    /*
      rv = pthread_mutex_lock(&cache->imageCacheLock);
      if (rv != 0) WARNING("pthread_mutex_lock returned %d (%s)",
      rv, ERROR_STRING(rv));
    */
    cache->lock(__FILE__, __LINE__); 
    ClearQueue(&cache->jobQueue);
    /* All pending requests that have a request number below or equal
     * to the threshold will be discarded when complete.
     */
    cache->validRequestThreshold = cache->requestNumber;
    /*
      rv = pthread_mutex_unlock(&cache->imageCacheLock);
      if (rv != 0) WARNING("pthread_mutex_unlock returned %d (%s)",
      rv, ERROR_STRING(rv));
    */ 
    cache->unlock(__FILE__, __LINE__); 
}

void DestroyImageCache(Canvas *canvas)
{
  ImageCache *cache = canvas->imageCache; 
  //    int rv;

    /* If we have reader threads, we have to clear the cache
     * work queue and cancel all the reader threads.
     */
    if(cache == NULL)
      return;

    CACHEDEBUG("DestroyImageCache");
    if (cache->numReaderThreads > 0) {
	register int i;

	/* Clear the work queue so no more work is taken up */

	ClearCacheWorkQueue(cache);

	/* Cancel all the threads, stopping all the work being done */
    CACHEDEBUG("Canceling %d threads", cache->numReaderThreads);
    for (i = 0; i < cache->numReaderThreads; i++) {
      CACHEDEBUG("terminating thread %d", i);
      /*
        int rv = pthread_cancel(cache->threads[i].threadId);
        if (rv != 0) WARNING("pthread_cancel returned %d (%s)",
        rv, ERROR_STRING(rv));
        rv = pthread_join(cache->threads[i].threadId, NULL);
        if (rv != 0) WARNING("pthread_cancel returned %d (%s)",
        rv, ERROR_STRING(rv));
      */ 
      // cache->mThreads[i]->terminate(); 
      cache->mThreads[i]->stop();  // more gentle;  give up mutexes, etc. 
    }
    
    for (i = 0; i < cache->numReaderThreads; i++) {
      cache->mThreads[i]->wait();    
      delete cache->mThreads[i]; 
	}
    cache->mThreads.clear(); 
    //	free(cache->threads);
    
    //    usleep(999); // sleep a millisecond


	/* Clear the remaining queues */
	ClearQueue(&cache->pendingQueue);
	ClearQueue(&cache->errorQueue);

	/* Destroy the mutex and condition variables as well. */
    /*
      rv = pthread_mutex_destroy(&cache->imageCacheLock);
      if (rv != 0) WARNING("pthread_mutex_destroy returned %d (%s)",
      rv, ERROR_STRING(rv));
      rv = pthread_cond_destroy(&cache->jobReady);
      if (rv != 0) WARNING("pthread_cond_destroy returned %d (%s)",
      rv, ERROR_STRING(rv));
      rv = pthread_cond_destroy(&cache->jobDone);
      if (rv != 0) WARNING("pthread_cond_destroy returned %d (%s)",
      rv, ERROR_STRING(rv));
    */ 
    }
    
    /* Any images in the cache must be released */
    ClearImageCache(cache);

    /* Free the slots that have been holding the cached images */
    CACHEDEBUG("free(cache->cachedImages)");
    free(cache->cachedImages);
    cache->cachedImages = NULL; 
    /* Finally, we can free the cache itself. The caller is
     * expected to handle the FrameList appropriately.
     */
    CACHEDEBUG("delete cache"); 
    delete cache; //free(cache);

    canvas->imageCache = NULL; 
}

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
void ManageFrameListInCache(ImageCache *cache, FrameList *frameList)
{
    /* If we've got worker threads, there may already be work that
     * is being done with the current framelist.  Clear all the
     * outstanding work requests, and make sure that any jobs
     * in progress will not enter the image cache.
     */
    if (cache->numReaderThreads > 0) {
	ClearCacheWorkQueue(cache);
    }

    /* If we've already cached images, they are now all out-of-date.
     * Free them.
     */
    ClearImageCache(cache);

    /* Put the new frame list in place. */
    cache->frameList = frameList;

    /* The caller should already have updated the canvas' frame list. */
}


/* This routine looks for an image already in the image cache, and returns
 * the cached image slot itself, or NULL.
 * Note: we don't care about the loaded image region at this point.
 */
static CachedImage *
FindImageInCache(ImageCache *cache, unsigned int frameNumber,
		 uint levelOfDetail)
{
    register CachedImage *cachedImage;
    register int i;

    /* Search the cache to see whether an appropriate image already
     * exists within the cache.
     */
    cachedImage = cache->cachedImages;
    for (i = 0, cachedImage = cache->cachedImages; 
	i < cache->maxCachedImages; 
	i++, cachedImage++) {
	if (cachedImage->loaded &&
	    cachedImage->frameNumber == frameNumber &&
	    cachedImage->levelOfDetail == levelOfDetail) {
	    return cachedImage;
	}
    }

    return NULL;
}



/* This routine gets an image from the cache, specified by frameNumber
 * and levelOfDetail.
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
 */
Image *GetImageFromCache(ImageCache *cache, uint32_t frameNumber,
	const Rectangle *newRegion, uint32_t levelOfDetail)
{
    Image *image;
    CachedImage *cachedImage = NULL;
    CachedImage *imageSlot = NULL;
    //    int rv;
    Rectangle region = *newRegion;

    /* fprintf(stderr,"GetImageFromCache y = %d\n",newRegion->y); */

    /* Keep track of highest frame number.  We need it for cache
     * replacement.
     */
    if (frameNumber > cache->highestFrameNumber)
        cache->highestFrameNumber = frameNumber;

    /* This counts as an additional cache request; we keep track of such
     * things so we can decide which of the cache entries is the oldest.
     */
    cache->requestNumber++;

    /* Loop until we find the request.  There are three ways we can
     * find the request; it can either be:
     *  - in the cache (in which case we return the cached image)
     *  - in the jobQueue (in which case we wait until an image
     *    is ready, and check again; note that the jobQueue is
     *    always empty in the single-threaded case)
     *  - in the pendingQueue (in which case we also wait until
     *    an image is ready, and check again; this queue is also
     *    always empty in the single-threaded case)
     *  - in the errorQueue (in which case we return with
     *    a NULL image; like the other queues, the errorQueue
     *    is always empty in the single-threaded case)
     *  - or not in any of the above (in which case we either
     *    load the image ourselves, in the single-threaded case,
     *    modify a job in the jobQueue, if a job with the same
     *    frame exists there; or add a new job to the jobQueue,
     *    and wait for an image to be ready)
     */

    if (cache->numReaderThreads > 0) {
      /*
        rv = pthread_mutex_lock(&cache->imageCacheLock);
        if (rv != 0) WARNING("pthread_mutex_lock returned %d (%s)",
		rv, ERROR_STRING(rv));
      */ 
      cache->lock(__FILE__, __LINE__);
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
	cachedImage = FindImageInCache(cache, frameNumber, levelOfDetail);
	
	if (cachedImage) {
      if (cachedImage->frameNumber != frameNumber || cachedImage->levelOfDetail != levelOfDetail) {
        ERROR("cachedImage->frameNumber != frameNumber || cachedImage->levelOfDetail != levelOfDetail"); 
        exit(1); 
      }
	    if (RectContainsRect(&cachedImage->image->loadedRegion, &region)) {
		/* This image is appropriate.  Mark our interest in the frame
		 * (so that it is not pulled out from under us while we're
		 * still using it), and return it.
		 */
		bb_assert(cachedImage->image->imageFormat.rowOrder != ROW_ORDER_DONT_CARE);

		cachedImage->requestNumber = cache->requestNumber;
		cachedImage->lockCount++;
		if (cache->numReaderThreads > 0) {
          /*
            rv = pthread_mutex_unlock(&cache->imageCacheLock);
		    if (rv != 0)
            WARNING("pthread_mutex_unlock returned %d (%s)",
            rv, ERROR_STRING(rv));
          */ 
          cache->unlock(__FILE__, __LINE__); 
		}
		return cachedImage->image;
	    }
	    else {
		/* load union of what's cached and what's requested */
               /*
		printf("  old loaded = %d, %d .. %d, %d\n",
		       cachedImage->image->loadedRegion.x,
		       cachedImage->image->loadedRegion.y,
		       cachedImage->image->loadedRegion.x +
		       cachedImage->image->loadedRegion.width,
		       cachedImage->image->loadedRegion.y +
		       cachedImage->image->loadedRegion.height);
		printf("  new request = %d, %d .. %d, %d\n", region.x,
		       region.y, region.x + region.width,
		       region.y + region.height);
               */
		region = RectUnionRect(&cachedImage->image->loadedRegion, &region);
		
		/*
		printf("  new region = %d, %d .. %d, %d\n",
		region.x, region.y,
		region.x + region.width, region.y + region.height);
		
		*/
	    }
	}

	/* It's not in cache already, darn.  We need to check to see if a 
	 * job for this frame is already in one of the work queues or the
	 * error queue.  This, of course, can only happen in the multi-
	 * threaded case.
	 */
	if (cache->numReaderThreads > 0) {
	    ImageCacheJob *job;

	    /* Remember that we still have the imageCacheLock from above.
	     * Look for a pending job (one that is being worked on) that
	     * matches our requirements.
	     */
	    job = FindJobInQueue(&cache->pendingQueue, frameNumber, &region,
				 levelOfDetail);
	    if (job != NULL) {
		/* We've got a job coming soon that will give us this image.
		 * Just wait for the reader thread to complete.  This should
		 * kick us back to the top of the loop when we return, with
		 * the imageCacheLock already re-acquired.
		 */
          /*
            rv = pthread_cond_wait(&cache->jobDone, &cache->imageCacheLock);
            if (rv != 0) WARNING("pthread_cond_wait returned %d (%s)",
			rv, ERROR_STRING(rv));
          */
          cache->WaitForJobDone(__FILE__, __LINE__); 
	    }
	    else {
		/* Look for a matching job in the work queue, one that may
		 * have been pre-loaded earlier.  We're a little less picky
		 * here; the job we find can refer to any region.  We're going
		 * to modify the job request, if necessary, to include the
		 * desired region.
		 */
		job = FindJobInQueue(&cache->jobQueue, frameNumber, NULL,
				     levelOfDetail);
		if (job != NULL) {
		    /* Add our region of interest to the job; if a request had
		     * been made earlier for a different region of the same
		     * frame, the request will be modified to use the union
		     * of the two regions.
		     */
		 
		    job->region = RectUnionRect(&job->region, &region);
		    job->requestNumber = cache->requestNumber;
		    /* Move the job to the head of the work queue, in case
		     * it was behind a few other preload jobs; it's more important
		     * now because the main thread is waiting on it.
		     */
		    MoveJobToHeadOfQueue(&cache->jobQueue, job);

		    /* Now wait for an image to appear. */
            /*
              rv = pthread_cond_wait(&cache->jobDone, &cache->imageCacheLock);
              if (rv != 0) WARNING("pthread_cond_wait returned %d (%s)",
              rv, ERROR_STRING(rv));
            */ 
            cache->WaitForJobDone(__FILE__, __LINE__); 
		}
		else {
		    /* No job in the pending queue nor in the work queue.  Check
		     * the error queue to make sure the job hasn't been attempted
		     * but has failed.
		     */
		    job = FindJobInQueue(&cache->errorQueue, frameNumber,
					 &region, levelOfDetail);
		    if (job != NULL) {
			/* The error, hopefully, has already been reported.  We
			 * have to remove the job from the queue and return to
			 * the caller with an error.
			 */
			RemoveJobFromQueue(&cache->errorQueue, job);
            /*
              rv = pthread_mutex_unlock(&cache->imageCacheLock);
              if (rv != 0) WARNING(
              "pthread_mutex_unlock returned %d (%s)",
              rv, ERROR_STRING(rv));
            */
            cache->unlock(__FILE__, __LINE__);
			return NULL;
		    }
		    else {
			/* No job anywhere.  Break out of the condition
			 * variable loop so the main thread can load the
			 * image itself.
			 */
              /*
                rv = pthread_mutex_unlock(&cache->imageCacheLock);
                if (rv != 0) WARNING("pthread_mutex_unlock returned %d (%s)",
				rv, ERROR_STRING(rv));
              */
              cache->unlock(__FILE__, __LINE__); 
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

    if (cache->frameList == NULL) {
	WARNING("frame %d requested from an empty cache",
		frameNumber);
	return NULL;
    }

    if (frameNumber > cache->frameList->numActualFrames()) {
	WARNING("image %d requested from a cache managing %d images",
		frameNumber, cache->frameList->numActualFrames() + 1);
	return NULL;
    }

    /* The requested image is valid enough.  If we're single-threaded, load the 
     * image ourselves; if we're multi-threaded, add work to the work queue and 
     * wait for an image to become ready.
     */
    image = LoadAndConvertImage(cache->frameList->getFrame(frameNumber),
	frameNumber, cache->canvas, &region, levelOfDetail);
    if (image == NULL) {
	/* Error has already been reported */
	return NULL;
    }
    bb_assert(image->imageFormat.rowOrder != ROW_ORDER_DONT_CARE);

    /* We have an image; we'll add it to the cache ourselves.  Find
     * a slot we can stick it in.
     */
    if (cache->numReaderThreads > 0) {
      /*
        rv = pthread_mutex_lock(&cache->imageCacheLock);
        if (rv != 0) WARNING("pthread_mutex_lock returned %d (%s)",
		rv, ERROR_STRING(rv));
      */
      cache->lock(__FILE__, __LINE__);
    }

    /* Either update the current image slot or find a new one */
    if (cachedImage) {
       imageSlot = cachedImage;
    }
    else {
#if DEBUG
	/* this image better not already be in the cache! */
	int i;
	CachedImage *img;
	for (i = 0, img = cache->cachedImages; i < cache->maxCachedImages;
	     i++, img++) {
	    if (img->lockCount > 0) {
		bb_assert(img->image != image);
	    }
	}
#endif
       imageSlot = GetCachedImageSlot(cache, frameNumber);
    }

    if (imageSlot == NULL) {
	/* We have an image, but no place to put it!
	 * The error has been reported; destroy the image
	 * and return NULL.
	 */
	if (cache->numReaderThreads > 0) {
      /*
	    rv = pthread_mutex_unlock(&cache->imageCacheLock);
	    if (rv != 0) WARNING("pthread_mutex_unlock returned %d (%s)",
        rv, ERROR_STRING(rv));
      */ 
      cache->unlock(__FILE__, __LINE__); 
	}
	(*image->ImageDeallocator)(cache->canvas, image);
        image = NULL;
	return NULL;
    }

    /* if there's an image in this slot, free it! */
    if (imageSlot->image) {
        (*image->ImageDeallocator)(cache->canvas, imageSlot->image);
        imageSlot->image = NULL;
    }

    /* Otherwise, we're happy.  Store the image away. */
    imageSlot->frameNumber = frameNumber;
    imageSlot->levelOfDetail = levelOfDetail;
    imageSlot->image = image;
    imageSlot->loaded = 1;
    imageSlot->lockCount = 1;
    imageSlot->requestNumber = cache->requestNumber;
    if (cache->numReaderThreads > 0) {
      /*
        rv = pthread_mutex_unlock(&cache->imageCacheLock);
        if (rv != 0) WARNING("pthread_mutex_unlock returned %d (%s)",
		rv, ERROR_STRING(rv));
      */ 
      cache->unlock(__FILE__, __LINE__); 
    }
    return image;
}

/* This routine unlocks the specified image, so that its
 * entry can be re-used (if no one else is using it).  The
 * actual image stays around; it will only actually be deleted
 * if the cache is destroyed or if the space is required
 * for another image.
 */
void ReleaseImageFromCache(ImageCache *cache, Image *image)
{
    register int i;
    register CachedImage *cachedImage;
    //int rv;

    /* Look for the given image in the cache. */
    if (cache->numReaderThreads > 0) {
      /*
        rv = pthread_mutex_lock(&cache->imageCacheLock);
        if (rv != 0) WARNING("pthread_mutex_lock returned %d (%s)",
		rv, ERROR_STRING(rv));
      */
      cache->lock(__FILE__, __LINE__);
    }
    cachedImage = cache->cachedImages;
    for (
	i = 0, cachedImage = cache->cachedImages; 
	i < cache->maxCachedImages; 
	i++, cachedImage++
    ) {
	if (cachedImage->image == image) {
	    /* Unlock it and return. */
	    if (cachedImage->lockCount == 0) {
		WARNING("unlocking an unlocked image, frame %d",
			cachedImage->frameNumber);
	    }
	    else {
		cachedImage->lockCount--;
	    }
	    if (cache->numReaderThreads > 0) {
          /*
            rv = pthread_mutex_unlock(&cache->imageCacheLock);
            if (rv != 0) WARNING("pthread_mutex_unlock returned %d (%s)",
			rv, ERROR_STRING(rv));
          */
          cache->unlock(__FILE__, __LINE__); 
	    }
	    return;
	}
    }

    /* If we get here, we couldn't find the image */
    /*
      rv = pthread_mutex_unlock(&cache->imageCacheLock);
      if (rv != 0) WARNING("pthread_mutex_unlock returned %d (%s)",
      rv, ERROR_STRING(rv));
      WARNING("tried to release an image not in cache");
    */
    cache->unlock(__FILE__, __LINE__); 
    return;
}

/* This routine is informatory; it notifies the cache that a particular frame
 * will likely be of interest soon, so that the cache can start loading the
 * image.  It is of utility once the main program becomes threaded; for now,
 * we can ignore it.
 *
 * It should only be called from the main thread, since it accesses the
 * frameList field (which is unprotected).
 */
void PreloadImageIntoCache(ImageCache *cache, uint32_t frameNumber, 
	const Rectangle *region, uint32_t levelOfDetail)
{
    ImageCacheJob *job, *newJob;
    CachedImage *cachedImage;
    //int rv;

    /* Keep track of highest frame number.  We need it for cache
     * replacement.
     */
    if (frameNumber > cache->highestFrameNumber)
        cache->highestFrameNumber = frameNumber;

    /* This entry point is only useful if there are threads in the system. */
    if (cache->numReaderThreads == 0) {
	return;
    }

    if (frameNumber >= cache->frameList->numActualFrames()) {
	ERROR("trying to preload non-existent frame (%d of %d)",
	      frameNumber, cache->frameList->numActualFrames());
	return;
    }

    /* If the image is already in cache, or if a job to load it already 
     * exists, don't bother adding a new job.
     */
    /*
      rv = pthread_mutex_lock(&cache->imageCacheLock);
      if (rv != 0) WARNING("pthread_mutex_lock returned %d (%s)",
      rv, ERROR_STRING(rv));
    */ 
    cache->lock(__FILE__, __LINE__);
    cachedImage = FindImageInCache(cache, frameNumber, levelOfDetail);
    if (cachedImage != NULL
	&& RectContainsRect(&cachedImage->image->loadedRegion, region)
        && cachedImage->levelOfDetail == levelOfDetail) {
	/* Image is already in cache - no need to preload again */
      /*
        rv = pthread_mutex_unlock(&cache->imageCacheLock);
        if (rv != 0) WARNING("pthread_mutex_unlock returned %d (%s)",
	    rv, ERROR_STRING(rv));
      */
      cache->unlock(__FILE__, __LINE__); 
	return;
    }

    /* Look for the job in the pendingQueue and workQueue */
    job = FindJobInQueue(&cache->pendingQueue, frameNumber, region, levelOfDetail);
    if (job == NULL) {
	job = FindJobInQueue(&cache->jobQueue, frameNumber, region, levelOfDetail);
    }
    /*
      rv = pthread_mutex_unlock(&cache->imageCacheLock);
      if (rv != 0) WARNING("pthread_mutex_unlock returned %d (%s)",
      rv, ERROR_STRING(rv));
    */
    cache->unlock(__FILE__, __LINE__); 
    if (job != NULL) {
	/* The job exists already - no need to add a new one */
	return;
    }

    /* If we get this far, there is no such image in the cache, and no such
     * job in the queue, so add a new one.
     */
    newJob = (ImageCacheJob *)calloc(1, sizeof(ImageCacheJob));
    if (newJob == NULL) {
	ERROR("cannot allocate new reader job");
	return;
    }

    /* This counts as an additional cache request; the job will take
     * on the request number, so we can determine which entries are the
     * oldest.  When the image is taken from the queue, it will be given
     * a more recent request number.
     */
    cache->requestNumber++;

    newJob->frameNumber = frameNumber;
    newJob->region = *region;
    newJob->levelOfDetail = levelOfDetail;
    newJob->requestNumber = cache->requestNumber;
    /* We save the frameInfo information just in case the FrameList changes
     * while one of the reader threads is trying to read this frame.
     * The results of the image read will be discarded (via the
     * validRequestThreshold mechanism); this just keeps us from
     * dumping core on a bad pointer.
     */
    newJob->frameInfo = *(cache->frameList->getFrame(frameNumber));
    
    /* Add the job to the back of the work queue */
    /*
      rv = pthread_mutex_lock(&cache->imageCacheLock);
      if (rv != 0) WARNING("pthread_mutex_lock returned %d (%s)",
      rv, ERROR_STRING(rv));
    */ 
    cache->lock(__FILE__, __LINE__);
    AddJobToQueue(&cache->jobQueue, newJob);
    cache->unlock(__FILE__, __LINE__); 
    /*
      rv = pthread_mutex_unlock(&cache->imageCacheLock);
      if (rv != 0) WARNING("pthread_mutex_unlock returned %d (%s)",
      rv, ERROR_STRING(rv));
    */
    
    /* If there's a worker thread that's snoozing, this will
     * wake him up.
     */
    /*
      rv = pthread_cond_signal(&cache->jobReady);
      if (rv != 0) WARNING("pthread_cond_signal returned %d (%s)",
      rv, ERROR_STRING(rv));
    */
    cache->WakeAllJobReady(__FILE__, __LINE__); 
}


/* For debugging */
void PrintCache(ImageCache *cache)
{
    register CachedImage *cachedImage;
    register int i;

    for (
	i = 0, cachedImage = cache->cachedImages; 
	i < cache->maxCachedImages; 
	i++, cachedImage++
    ) {
	printf("  Slot %d: locked:%d  frame:%d  lod:%d  req:%d  ", i,
               cachedImage->lockCount,
               cachedImage->frameNumber,
               cachedImage->levelOfDetail,
	       (int) cachedImage->requestNumber);
	if (cachedImage->image) {
	    printf("roi:%d,%d .. %d, %d\n",
                   cachedImage->image->loadedRegion.x,
                   cachedImage->image->loadedRegion.y,
                   cachedImage->image->loadedRegion.x +
                   cachedImage->image->loadedRegion.width,
                   cachedImage->image->loadedRegion.y +
		   cachedImage->image->loadedRegion.height);
	}
	else
	    printf("\n");
    }
}
