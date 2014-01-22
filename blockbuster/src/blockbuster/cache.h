#ifndef BLOCKBUSTER_CACHE_H
#define BLOCKBUSTER_CACHE_H

//=======================================================
// if youwant verbose cache messages, use these:
void enableCacheDebug(bool onoff); 
bool cacheDebug_enabled(void); 
#define CACHEDEBUG(...) if (cacheDebug_enabled()) DEBUGMSG( __VA_ARGS__)

// #define CACHEDEBUG if (0) DEBUGMSG
// else use this:
//#define CACHEDEBUG DEBUGMSG
//=======================================================

#include <vector>
#include "frames.h"
#include "QThread"
#include "QMutex"
#include "QWaitCondition"
#include "errmsg.h"
#include <deque>
using namespace std; 



typedef boost::shared_ptr<class ImageCache> ImageCachePtr; 
typedef boost::shared_ptr<class CacheThread> CacheThreadPtr; 


// =================================================================
//! ImageCacheJob:  a request for the workers to get an image
typedef boost::shared_ptr<struct ImageCacheJob> ImageCacheJobPtr; 

struct ImageCacheJob {
  ImageCacheJob(uint32_t frame, const Rectangle *reg, 
                uint32_t  lod, uint32_t reqnum, FrameInfoPtr fInfo): 
    frameInfo(fInfo), frameNumber(frame), region(*reg),  
    levelOfDetail(lod), requestNumber(reqnum) { 
    return; 
  }
    
  ~ImageCacheJob() {}
  QString toString(void) {
    return QString("{ ImageCacheJob: frameNumber = %1, region = %2, LOD = %3, request = %4, frameInfo = %5 }").arg(frameNumber).arg(region.toString()).arg(levelOfDetail).arg(requestNumber).arg(frameInfo->toString());
  }

  FrameInfoPtr frameInfo; 
  uint32_t frameNumber;
  Rectangle region;
  uint32_t levelOfDetail;
  uint32_t requestNumber;
} ;

// =================================================================
typedef boost::shared_ptr<struct CachedImage> CachedImagePtr; 
//! CachedImage: a slot in the image cache
struct CachedImage{
  CachedImage():requestNumber(0), loaded(0), frameNumber(0), levelOfDetail(0) {
    return; 
  }
  ~CachedImage() {
    return; 
  }
    
  unsigned long requestNumber;
  int loaded;
  uint32_t frameNumber;
  uint32_t levelOfDetail;
  ImagePtr image;
} ;

// ====================================================================
class CacheThread: public QThread {
  Q_OBJECT
    public:
  CacheThread(ImageCache *icache): 
    mCache(icache), mStop(false) {
    CACHEDEBUG("CacheThread constructor");     
    RegisterThread(this); 
  }
  ~CacheThread(){
    CACHEDEBUG("CacheThread destructor");     
  }
  

  void run(void); 
  void stop(void) { mStop = true; }
  ImageCache *mCache;
  bool mStop; 
};


// ====================================================================
/* An Image Cache is a collection of images, both read from the
 * disk (i.e. full images) and as yet unread (i.e., frame
 * specifications).  The Image Cache abstraction allows for images
 * to be preloaded, loaded upon request, retrieved quickly if they're
 * already loaded, etc.
 *
 * The ImageCache handles the basic threading operations required
 * by the application, by handing off work to child threads and
 * waiting on them appropriately.  (If no threads are specified,
 * the ImageCache can also run simply as an abstraction layer.)
 * Most of the information in here is owned by the "boss" thread,
 * if there's more than one thread.  It should not be accessed
 * or changed by the child threads, except where specifically
 * allowed, because access is not, in general, controlled by
 * mutexes (mutices? :-).
 */
class ImageCache {
  friend class CacheThread; 
 public:
  ImageCache(int numthreads, int numimages, ImageFormat &required);
  ~ImageCache(); 
  
  unsigned int Distance(unsigned int oldFrame, unsigned int newFrame);

  ImagePtr GetImage(uint32_t frameNumber,
                  const Rectangle *newRegion, uint32_t levelOfDetail);
  void ManageFrameList(FrameListPtr frameList);
  
  // I do not like that these are called outside of the cache.  This is a huge design flaw.  The cache needs to manage how items are cached!  

  // PreloadImage is called from Renderer
  void PreloadHint(uint32_t preloadFrames, int playDirection, 
                   uint32_t startFrame, uint32_t endFrame){
    mPreloadFrames = preloadFrames;
    mCurrentPlayDirection = playDirection; 
    mCurrentStartFrame = startFrame; 
    mCurrentEndFrame = endFrame; 
  } 
  void PreloadImage(uint32_t frameNumber, 
                    const Rectangle *region, uint32_t levelOfDetail);
 
 protected:
  CachedImagePtr GetCachedImageSlot(uint32_t newFrameNumber);
  
  void RemoveJobFromPendingQueue(ImageCacheJobPtr job) {
    RemoveJobFromQueue(mPendingQueue, job); 
  }
  
  void RemoveJobFromQueue(deque<ImageCacheJobPtr> &queue, ImageCacheJobPtr job);
  
  ImageCacheJobPtr FindJobInQueue
    (deque<ImageCacheJobPtr> &queue,  unsigned int frameNumber, 
     const Rectangle *region, unsigned int levelOfDetail); 
  void ClearQueue(deque<ImageCacheJobPtr> &queue); 
  void ClearImages(void); 
  void ClearJobQueue(void);
  CachedImagePtr FindImage(uint32_t frame, uint32_t lod);
  void Print(string where); 
#define PrintJobQueue(q) __PrintJobQueue(#q,q)
  void __PrintJobQueue(QString name, deque<ImageCacheJobPtr>&q); 
  void  lock(string reason, string file="unknown file", int line=0) {
    CACHEDEBUG("%s: %d: locking image cache (%s)", 
               file.c_str(), line, reason.c_str()); 
    imageCacheLock.lock(); 
    /* CACHEDEBUG("%s: %d: locked image cache (%s)", 
       file, line, reason); 
    */
  }
  void unlock(string reason, string file="unknown file", int line=0) {
    imageCacheLock.unlock(); 
    CACHEDEBUG("%s: %d: unlocked image cache (%s)", 
               file.c_str(), line, reason.c_str()); 
  }
  
  void WaitForJobReady(string reason, string file="unknown file", int line=0) {
    CACHEDEBUG("%s: %d: worker waiting job ready (%s)", 
               file.c_str(), line, reason.c_str()); 
    jobReady.wait(&imageCacheLock, 100); 
  }
  
  void WaitForJobDone(string reason, string file="unknown file", int line=0) {
    CACHEDEBUG("%s: %d: main thread waiting job done (%s)", 
               file.c_str(), line, reason.c_str()); 
    jobDone.wait(&imageCacheLock, 100); 
    CACHEDEBUG("%s: %d: main thread Woke up (%s)", file.c_str(), line, reason.c_str()); 
  }
  void WakeAllJobReady(string reason, string file="unknown file", int line=0) {
    CACHEDEBUG("%s: %d: main thread signaling job ready (%s)", 
               file.c_str(), line, reason.c_str()); 
    jobReady.wakeAll(); 
  }
  void WakeAllJobDone(string reason, string file="unknown file", int line=0) {
    CACHEDEBUG("%s: %d: worker thread signaling job done (%s)", 
               file.c_str(), line, reason.c_str()); 
    jobDone.wakeAll(); 
  }
  
  /* Configuration information */
  int mNumReaderThreads;
  int mMaxCachedImages;

  ImageFormat mRequiredImageFormat; 
   
  /* Cache management details */
  FrameListPtr mFrameList;
  unsigned long mRequestNumber;

  /* The validRequestThreshold is the number of the last request that
   * we shold ignore.  It starts at 0 (so we don't ignore any requests);
   * if something should happen that would invalidate pending work 
   * requests (say, by changing the FrameList while reader threads are
   * still reading images from the old FrameList), this will keep the
   * invalidated images from entering the image queue.
   */
  unsigned long mValidRequestThreshold;
  vector<CachedImagePtr> mCachedImages;
  unsigned int mHighestFrameNumber;
  
  bool mSuppressStereo; // for playing stereo movies in mono -- do not preload odd frames.
  /* New approach:  start managing preloads from the cache */ 
  uint32_t mPreloadFrames;
  int32_t mCurrentPlayDirection; 
  uint32_t mCurrentStartFrame; // the first frame the user wants played
  uint32_t mCurrentEndFrame;  // the last frame the user wants played

  uint64_t mMaxCacheBytes; 
  uint64_t mCurrentCacheBytes; 

  /* Thread management; none of these fields are used or examined
   * unless numReaderThreads is greater than 0.
   */
  std::vector<CacheThreadPtr> mThreads;
  QMutex imageCacheLock; 
  QWaitCondition jobReady, jobDone; 
  deque<ImageCacheJobPtr> mJobQueue; // jobs ready for the workers to take
  deque<ImageCacheJobPtr> mPendingQueue; // jobs being worked on by a worker
  deque<ImageCacheJobPtr> mErrorQueue;  // FAILs
} ;


#endif
