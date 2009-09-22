#ifndef BLOCKBUSTER_CACHE_H
#define BLOCKBUSTER_CACHE_H

//=======================================================
// if you don't want verbose cache messages, use this:
//#define CACHEDEBUG if (0) DEBUGMSG
// else use this:
#define CACHEDEBUG DEBUGMSG
//=======================================================

#include <vector>
#include "frames.h"
#include "QThread"
#include "QMutex"
#include "QWaitCondition"
#include "errmsg.h"
#include <deque>
using namespace std; 

struct ImageCache; 

//! ImageCacheJob:  a request for the workers to get an image
struct ImageCacheJob {
  ImageCacheJob(): frameNumber(0), levelOfDetail(0), requestNumber(0) {}
  ImageCacheJob(uint32_t frame, const Rectangle *reg, 
                uint32_t  lod, uint32_t reqnum): 
    frameNumber(frame),  levelOfDetail(lod), requestNumber(reqnum)
  { region = *reg; }
    
  ~ImageCacheJob() {}
  QString toString(void) {
    return QString("{ ImageCacheJob: frameNumber = %1, region = %2, LOD = %3, request = %4, frameInfo = %5 }").arg(frameNumber).arg(region.toString()).arg(levelOfDetail).arg(requestNumber).arg(frameInfo.toString());
  }

  FrameInfo frameInfo; /* Local copy needed in case FrameList changes while we're working */
  uint32_t frameNumber;
  Rectangle region;
  uint32_t levelOfDetail;
  uint32_t requestNumber;
} ;

//! CachedImage: a slot in the image cache
struct CachedImage{
  unsigned long requestNumber;
  int loaded;
  uint32_t lockCount;
  uint32_t frameNumber;
  uint32_t levelOfDetail;
  Image *image;
} ;

class CacheThread: public QThread {
  Q_OBJECT
    public:
  CacheThread(ImageCache *incache): 
    cache(incache), mStop(false) {
    CACHEDEBUG("CacheThread constructor");     
    RegisterThread(this); 
  }
  ~CacheThread(){}
  

  void run(void); 
  void stop(void) { mStop = true; }
  ImageCache *cache;
  bool mStop; 
};


 
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
  ImageCache(int numthreads, int numimages, Canvas *c);
  ~ImageCache(); 
  
  Image *GetImage(uint32_t frameNumber,
                  const Rectangle *newRegion, uint32_t levelOfDetail);
  void PreloadImage(uint32_t frameNumber, 
                    const Rectangle *region, uint32_t levelOfDetail);
  void ManageFrameList(FrameList *frameList);
  
  void ReleaseImage(Image *image);
 protected:
  CachedImage *GetCachedImageSlot(uint32_t newFrameNumber);
  
  void RemoveJobFromPendingQueue(ImageCacheJob *job) {
    RemoveJobFromQueue(mPendingQueue, job); 
  }
  
  void RemoveJobFromQueue(deque<ImageCacheJob*> &queue, ImageCacheJob *job);
  
  ImageCacheJob *FindJobInQueue
    (deque<ImageCacheJob*> &queue,  unsigned int frameNumber, 
     const Rectangle *region, unsigned int levelOfDetail); 
  void ClearQueue(deque<ImageCacheJob*> &queue); 
  void ClearImages(void); 
  void GetConfiguration(int &outNumThreads, int &outMaxImages) ;
  void ClearJobQueue(void);
  CachedImage *FindImage(uint32_t frame, uint32_t lod);
  void Print(void); 
#define PrintJobQueue(q) __PrintJobQueue(#q,q)
  void __PrintJobQueue(QString name, deque<ImageCacheJob *>&q); 
  void  lock(char *reason, char *file="unknown file", int line=0) {
    CACHEDEBUG("%s: %d: locking image cache (%s)", 
               file, line, reason); 
    imageCacheLock.lock(); 
    /* CACHEDEBUG("%s: %d: locked image cache (%s)", 
       file, line, reason); 
    */
  }
  void unlock(char *reason, char *file="unknown file", int line=0) {
    imageCacheLock.unlock(); 
    CACHEDEBUG("%s: %d: unlocked image cache (%s)", 
               file, line, reason); 
  }
  
  void WaitForJobReady(char *reason, char *file="unknown file", int line=0) {
    CACHEDEBUG("%s: %d: worker waiting job ready (%s)", 
               file, line, reason); 
    jobReady.wait(&imageCacheLock, 100); 
  }
  
  void WaitForJobDone(char *reason, char *file="unknown file", int line=0) {
    CACHEDEBUG("%s: %d: main thread waiting job done (%s)", 
               file, line, reason); 
    jobDone.wait(&imageCacheLock, 100); 
    CACHEDEBUG("%s: %d: main thread Woke up (%s)", file, line, reason); 
  }
  void WakeAllJobReady(char *reason, char *file="unknown file", int line=0) {
    CACHEDEBUG("%s: %d: main thread signaling job ready (%s)", 
               file, line, reason); 
    jobReady.wakeAll(); 
  }
  void WakeAllJobDone(char *reason, char *file="unknown file", int line=0) {
    CACHEDEBUG("%s: %d: worker thread signaling job done (%s)", 
               file, line, reason); 
    jobDone.wakeAll(); 
  }
  
  /* Configuration information */
  int mNumReaderThreads;
  int mMaxCachedImages;
  Canvas *mCanvas;
  
  /* Cache management details */
  FrameList *mFrameList;
  unsigned long mRequestNumber;
  unsigned long mValidRequestThreshold;
  CachedImage *mCachedImages;
  unsigned int mHighestFrameNumber;
  
  /* Thread management; none of these fields are used or examined
   * unless numReaderThreads is greater than 0.
   */
  std::vector<CacheThread *> mThreads;
  QMutex imageCacheLock; 
  QWaitCondition jobReady, jobDone; 
  deque<ImageCacheJob *> mJobQueue;
  deque<ImageCacheJob *> mPendingQueue;
  deque<ImageCacheJob *> mErrorQueue;
} ;

/* Image Cache management from cache.c.  These utilities are expected to be
 * used by Renderer modules to manage their images (because each renderer
 * is responsible for its own image management).
 */
ImageCache *CreateImageCache(int numReaderThreads, int maxCachedImages, Canvas *canvas);
void DestroyImageCache(Canvas *canvas);
void CachePreload(Canvas *canvas, uint32_t frameNumber, 
                  const Rectangle *imageRegion, uint32_t levelOfDetail);

#endif
