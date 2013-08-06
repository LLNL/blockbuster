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
  NewCacheThread(): mStop(false) {
    CACHEDEBUG("CacheThread constructor");     
    RegisterThread(this); 
  }
  ~NewCacheThread(){
    return; 
  }
  

  void run(void) {
    return; 
  }
  void stop(void) { mStop = true; }
  bool mStop; 
};


// ====================================================================
class NewImageCache {
  NewImageCache(int numthreads, int maximages,  ImageFormat &required):
    mNumThreads(numthreads), mMaxImages(maximages), mRequiredFormat(required) {
    return; 
  }
    
  ~NewImageCache() {
    return; 
  }
  
  void ManageFrameList(FrameListPtr frameList) ;       

  ImagePtr GetImage(uint32_t frameNumber, int playDirection, 
                  const Rectangle *newRegion, uint32_t levelOfDetail);
  
  FrameListPtr mFrameList; 

  uint32_t mNumThreads, mMaxImages; 
  ImageFormat mRequiredFormat; 
}; 

#endif
