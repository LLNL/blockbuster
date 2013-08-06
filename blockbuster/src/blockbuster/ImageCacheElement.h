#ifndef BLOCKBUSTER_IMAGECACHE_ELEMENT_H
#define BLOCKBUSTER_IMAGECACHE_ELEMENT_H
#include "boost/atomic.hpp"
#include "boost/shared_ptr.hpp"
#include <vector> 

/*!
   This header exists to avoid circular dependencies in 
   ImageCache.h and frames.h 
   Implementations are in ImageCache.cpp
*/ 
typedef boost::shared_ptr<struct ImageCacheElement> ImageCacheElementPtr; 

// ====================================================================
struct ImageCacheElement {
  ImageCacheElement(): 
    mComplete(false), mAbort(false), mOwnerThread(-1) {
    return; 
  }

  bool mComplete, mAbort; 
  boost::atomic<int> mOwnerThread; // a worker owns this if nonnegative
  

}; 

// ====================================================================
struct ImageCacheElementManager {
  ImageCacheElementManager(): 
    mCacheDirection(0), // we don't skip frames when caching
    mCurrentFrame(0), mStartFrame(0), mEndFrame(0),
    mMaxFrames(0),  mFramesInProgress(0), mFramesCompleted(0) {
    return; 
  }
  int mCacheDirection; 
  uint32_t mCurrentFrame, mStartFrame, mEndFrame, mMaxFrames; 
  boost::atomic<uint32_t> mFramesInProgress, mFramesCompleted; 
  
}; 

#endif

