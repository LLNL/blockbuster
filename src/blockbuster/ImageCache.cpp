
#include "ImageCache.h"


FrameInfoPtr NewCacheThread::FindJob(void) {
  FrameInfoPtr job; 
  // Find a job that needs doing. 
  // If we have a job: is there room in the cache?  If so, return the job.
  // If not, can we release a less important frame?  If so, return the job. 
  // If not, release the job and return empty pointer. 
  return job; 
}

// ====================================================================
void NewCacheThread::run() {
     // find a job in the FrameList: 
  while (true) {
    if (mStop) {
      return; 
    } 

    FrameInfoPtr job = FindJob(); 
    if (!job) {
     // If not, go to sleep and try again later. 
       msleep(100); 
    } else {
      job->LoadAndConvertImage(mRequiredFormat, mRegionOfInterest, mLOD); 
      job->mComplete = true;       
    } 
    job.reset(); 
  }
}

// ====================================================================
NewImageCache::~NewImageCache() {
  for (uint32_t i = 0; i < mNumThreads; i++) {
    CACHEDEBUG("terminating thread %d", i);
    // mThreads[i]->terminate(); 
    mThreads[i]->stop();  //  gentle;  give up mutexes, etc. 
  }
  
  for (uint32_t i = 0; i < mNumThreads; i++) {
    mThreads[i]->wait();    
  }
  mThreads.clear(); 
}

