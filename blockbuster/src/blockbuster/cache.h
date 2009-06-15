#ifndef BLOCKBUSTER_CACHE_H
#define BLOCKBUSTER_CACHE_H

//=======================================================
// if you don't want verbose cache messages, use this:
#define CACHEDEBUG if (0) DEBUGMSG
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


struct ImageCacheJob {
  ImageCacheJob(): frameNumber(0), levelOfDetail(0), requestNumber(0) {}
  ImageCacheJob(uint32_t frame, const Rectangle *reg, 
                uint32_t  lod, uint32_t reqnum): 
    frameNumber(frame),  levelOfDetail(lod), requestNumber(reqnum)
  { region = *reg; }
    
  ~ImageCacheJob() {}

  FrameInfo frameInfo; /* needed in case FrameList changes while we're working */
  uint32_t frameNumber;
  Rectangle region;
  uint32_t levelOfDetail;
  uint32_t requestNumber;
} ;


class CacheThread: public QThread {
  Q_OBJECT
    public:
  CacheThread(int threadNum, ImageCache *incache): 
    threadNumber(threadNum), cache(incache), mStop(false) {
    CACHEDEBUG("CacheThread constructor"); 
  }
  ~CacheThread(){}
  

  void run(void); 
  void stop(void) { mStop = true; }
  //pthread_t threadId;
  int threadNumber;
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
   */


   struct CachedImage{
    unsigned long requestNumber;
    int loaded;
    uint32_t lockCount;
    uint32_t frameNumber;
    uint32_t levelOfDetail;
    Image *image;
  } ;

   struct ImageCacheThreadInfo{
    pthread_t threadId;
    int threadNumber;
    struct ImageCache *imageCache;
  } ;

  /* Most of the information in here is owned by the "boss" thread,
   * if there's more than one thread.  It should not be accessed
   * or changed by the child threads, except where specifically
   * allowed, because access is not, in general, controlled by
   * mutexes (mutices? :-).
   */
   class ImageCache {
   public:
     ImageCache(int numthreads, int numimages, Canvas *c);
     ~ImageCache(); 

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
     void ManageFrameList(FrameList *frameList);
     CachedImage *FindImage(uint32_t frame, uint32_t lod);
     Image *GetImage(uint32_t frameNumber,
                     const Rectangle *newRegion, uint32_t levelOfDetail);
     void ReleaseImage(Image *image);
     void PreloadImage(uint32_t frameNumber, 
                       const Rectangle *region, uint32_t levelOfDetail);
     void Print(void); 

   void  lock(char *file="unknown file", int line=0) {
      CACHEDEBUG("%s: %d: locking image cache", file, line); 
      imageCacheLock.lock(); 
    }
    void unlock(char *file="unknown file", int line=0) {
      CACHEDEBUG("%s: %d: unlocking image cache", file, line); 
      imageCacheLock.unlock(); 
      CACHEDEBUG("%s: %d: unlocked image cache", file, line); 
    }

    void WaitForJobReady(char *file="unknown file", int line=0) {
      CACHEDEBUG("%s: %d: worker waiting job ready", file, line); 
      jobReady.wait(&imageCacheLock, 100); 
    }
    
    void WaitForJobDone(char *file="unknown file", int line=0) {
      CACHEDEBUG("%s: %d: main thread waiting job done", file, line); 
      jobDone.wait(&imageCacheLock, 100); 
    }
    void WakeAllJobReady(char *file="unknown file", int line=0) {
      CACHEDEBUG("%s: %d: main thread signaling job ready", file, line); 
      jobReady.wakeAll(); 
    }
    void WakeAllJobDone(char *file="unknown file", int line=0) {
      CACHEDEBUG("%s: %d: worker thread signaling job done", file, line); 
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

/*
  void ManageFrameListInCache(ImageCache *cache, FrameList *frameList);
  Image *GetImageFromCache(ImageCache *cache, uint32_t frameNumber,
  const Rectangle *region, uint32_t levelOfDetail);
  void ReleaseImageFromCache(ImageCache *cache, Image *image);
  void PreloadImageIntoCache(ImageCache *cache, uint32_t frameNumber, 
  const Rectangle *region, uint32_t levelOfDetail);
  void PrintCache(ImageCache *cache);
  void GetCacheConfiguration(ImageCache *cache, int *numReaderThreads, int *maxCachedImages);
*/ 
#endif
