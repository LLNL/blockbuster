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

// ====================================================================
class NewCacheThread: public QThread {
  Q_OBJECT
    public:
  NewCacheThread(FrameListPtr frameList, ImageFormatPtr format): 
    mStop(false), mFrameList(frameList), mRequiredFormat(format) {
    CACHEDEBUG("CacheThread constructor");     
    RegisterThread(this); 
  }
  ~NewCacheThread(){
    return; 
  }
  
  FrameInfoPtr FindJob(void); 

  void run(void);
  void stop(void) { mStop = true; }
  bool mStop; 
  FrameListPtr mFrameList; 
  ImageFormatPtr mRequiredFormat; 
  RectanglePtr mRegionOfInterest; 
  int mLOD; 
};


// ====================================================================
class NewImageCache {
  NewImageCache(int numthreads, int maximages,  
                ImageFormatPtr required, FrameListPtr frameList):
    mNumThreads(numthreads), mMaxImages(maximages), 
    mFrameList(frameList), mRequiredFormat(required)  {

    for (uint32_t i = 0; i < mNumThreads; i++) {
      mThreads.push_back(NewCacheThreadPtr(new NewCacheThread(mFrameList, mRequiredFormat))); 
      mThreads[i]->start(); 
    }

    return; 
  }
    
    ~NewImageCache();
  
  ImagePtr GetImage(uint32_t frameNumber, int playDirection, 
                    RectanglePtr newRegion, uint32_t levelOfDetail);
  

  uint32_t mNumThreads, mMaxImages; 
  std::vector<NewCacheThreadPtr> mThreads;
  FrameListPtr mFrameList; 
  ImageFormatPtr mRequiredFormat; 
  RectanglePtr mRegionOfInterest; 
  int mLOD; 
}; 

#endif
