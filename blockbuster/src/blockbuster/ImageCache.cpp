
#include "ImageCache.h"


FrameStatusPtr NewCacheThread::FindJob(void) {
  // Find a FrameStatus that is not claimed.  Claim it.  Then:
  // If there room in the cache, return true. 
  // If not, but we can release a less important frame, return true
  // If not, unclaim the job and return false. 
  for (FrameStatusVector::iterator job = mCacheState->mFrameStatuses->begin(); 
       job != mCacheState->mFrameStatuses->end(); job++) {
    if (0) return *job; 
  }
  return FrameStatusPtr(); 
}

// ====================================================================
void NewCacheThread::run() {
  // find a job in the FrameList: 
  while (true) {
    if (mStop) {
      return; 
    } 

    FrameStatusPtr job = FindJob(); 
    if (!job) {
      // If not, go to sleep and try again later. 
      msleep(100); 
    } else {
      job->mFrameInfo->LoadAndConvertImage(mCacheState->mRequiredFormat, 
                                           mCacheState->mRegionOfInterest, 
                                           mCacheState->mLOD); 
      job->mComplete = true;       
    } 
    job.reset(); 
  }
}

// ====================================================================
NewImageCache::NewImageCache(int numthreads, int maximages,  
                             ImageFormatPtr required, FrameListPtr frameList):
  mCacheState(new CacheState), mNumThreads(numthreads){

  mCacheState->mMaxImages = maximages; 
  mCacheState->mRequiredFormat = required; 
  mCacheState->mFrameList = frameList; 
  mCacheState->mFrameStatuses.reset(new FrameStatusVector(frameList->mFrames.size())); 

  for (uint32_t i = 0; i < mNumThreads; i++) {
    mThreads.push_back(NewCacheThreadPtr(new NewCacheThread(mCacheState, i))); 
    mThreads[i]->start(); 
  }


  return; 
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

// ====================================================================
ImagePtr NewImageCache::GetImage(uint32_t , int , 
                                 RectanglePtr , uint32_t ) {
  ImagePtr image; 
  // make sure the cache threads know this is important:
  
  return image; 
}

//===============================================================
/*void NewImageCache::ReleaseFramesFromCache(void) {
  for (vector<FrameProgressPtr>::iterator pos = mFrameProgress.begin(); pos != mFrameProgress.end(); pos++) {
    // mAbort ensures we don't get false negatives when checking mOwnerThread
    (*pos)->mAbort.store(true); 
    if ((*pos)->mOwnerThread >= 0) {
      pos->reset((*pos)->Clone()); // get the proper subclass
      (*pos)->mComplete.store(false); 
      (*pos)->mOwnerThread = -1; 
    }
    (*pos)->mAbort.store(false); 
  }
  mCacheState->mFramesInProgress.store(0); 
  mCacheState->mFramesCompleted.store(0); 
  mCacheState->mCacheDirection = 0; 
  return; 
}

*/
