#ifndef BLOCKBUSTER_IMAGECACHE_H
#define BLOCKBUSTER_IMAGECACHE_H

#include "QThread"
#include "QMutex"
#include "QWaitCondition"
#include "errmsg.h"
#include "boost/shared_ptr.hpp"
#include "frames.h"

#ifndef CACHEDEBUG
#define CACHEDEBUG if (0) DEBUGMSG
#endif

typedef boost::shared_ptr<class NewImageCache> NewImageCachePtr; 
typedef boost::shared_ptr<class NewCacheThread> NewCacheThreadPtr; 

/*!
 * ====================================================================
 * CacheElementProgress:
 * Per-frame information about the progress in converting a frame.  
 * I could store this in each FrameInfo but it's only needed by the cache, 
 * so I prefer to manage a separate vector for this purpose, shared among 
 * cache threads. 
 */ 
typedef boost::shared_ptr<struct FrameStatus> FrameStatusPtr;
typedef std::vector<FrameStatusPtr> FrameStatusVector; 
typedef boost::shared_ptr<FrameStatusVector> FrameStatusVectorPtr;
struct FrameStatus {
  FrameStatus(): mComplete(false), mAbort(false), mOwnerThread(-1) {
    return; 
  }
  /* Cache management fields */ 
  FrameInfoPtr mFrameInfo; 
  boost::atomic<bool> mComplete, mAbort; // atomic so they can be used as barriers
  boost::atomic<int> mOwnerThread; // a worker owns this if nonnegative
  RectanglePtr mRegionOfInterest; 
  int mLOD; 
}; 

/*!
 * ====================================================================
 * CacheInfo:
 * Shared info about the state of the cache, to communicate between threads.  
 */
typedef boost::shared_ptr<struct CacheState> CacheStatePtr; 
struct CacheState {

  ImageFormatPtr mRequiredFormat; 
  FrameListPtr mFrameList; 

  RectanglePtr mRegionOfInterest; 
  uint32_t mMaxImages; 
  int mLOD; 
  int mCacheDirection; 
  uint32_t mCurrentFrame; // the last frame requested
  uint32_t mFirstAvailable; // the last frame not marked as mInProgress -- not strict but close
  uint32_t mStartFrame; // play limits as set by user
  uint32_t mEndFrame; // play limits as set by user -- influences distance
  uint32_t mMaxFrames; // max allowed in cached as set by user
  boost::atomic<uint32_t> mFramesInProgress, mFramesCompleted; 
  FrameStatusVectorPtr mFrameStatuses; 

} ;

// ====================================================================
class NewCacheThread: public QThread {
  Q_OBJECT
    public:
  NewCacheThread(CacheStatePtr s, int threadnum): 
    mStop(false), mCacheState(s) {
    CACHEDEBUG("CacheThread constructor");     
    RegisterThread(this, threadnum); 
  }
  ~NewCacheThread(){
    return; 
  }
  
  FrameStatusPtr FindJob(void); 

  void run(void);
  void stop(void) { mStop = true; }
  bool mStop; 
  CacheStatePtr mCacheState; 
};


// ====================================================================
class NewImageCache {
  NewImageCache(int numthreads, int maximages,  
                ImageFormatPtr required, FrameListPtr frameList);
    
    ~NewImageCache();
  
  ImagePtr GetImage(uint32_t frameNumber, int playDirection, 
                    RectanglePtr newRegion, uint32_t levelOfDetail);
  

  std::vector<NewCacheThreadPtr> mThreads;
  boost::shared_ptr<CacheState> mCacheState; 
  uint32_t mNumThreads; 
}; 

#endif
