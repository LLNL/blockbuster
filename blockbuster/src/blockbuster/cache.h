#ifndef BLOCKBUSTER_CACHE_H
#define BLOCKBUSTER_CACHE_H

#include <vector>
#include "frames.h"
#include "QThread"
#include "QMutex"
#include "QWaitCondition"
#include "errmsg.h"

struct ImageCache; 



//#define CACHEDEBUG if (0) DEBUGMSG

#define CACHEDEBUG DEBUGMSG

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

   struct ImageCacheJob {
    FrameInfo frameInfo; /* needed in case FrameList changes while we're working */
    unsigned int frameNumber;
    unsigned long requestNumber;
    Rectangle region;
    unsigned int levelOfDetail;
    struct ImageCacheJob *next;
    struct ImageCacheJob *prev;
  } ;

   struct ImageCacheQueue{
    ImageCacheJob *head;
    ImageCacheJob *tail;
  } ;

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
     ImageCache(): numReaderThreads(0), maxCachedImages(0), canvas(NULL) {
       CACHEDEBUG("ImageCache constructor"); 
     }
     ~ImageCache() {}
    /* Configuration information */
    int numReaderThreads;
    int maxCachedImages;
    Canvas *canvas;
    
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


    /* Cache management details */
    FrameList *frameList;
    unsigned long requestNumber;
    unsigned long validRequestThreshold;
    CachedImage *cachedImages;
    unsigned int highestFrameNumber;

    /* Thread management; none of these fields are used or examined
     * unless numReaderThreads is greater than 0.
     */
     /*   ImageCacheThreadInfo *threads; */
     /* pthread_mutex_t imageCacheLock;
        pthread_cond_t jobReady;
        pthread_cond_t jobDone;
     */ 
     std::vector<CacheThread *> mThreads;
     QMutex imageCacheLock; 
     QWaitCondition jobReady, jobDone; 
     ImageCacheQueue jobQueue;
    ImageCacheQueue pendingQueue;
    ImageCacheQueue errorQueue;

  } ;

  /* Image Cache management from cache.c.  These utilities are expected to be
   * used by Renderer modules to manage their images (because each renderer
   * is responsible for its own image management).
   */
ImageCache *CreateImageCache(int numReaderThreads, int maxCachedImages, Canvas *canvas);
   void ClearImageCache(ImageCache *cache);
   void DestroyImageCache(Canvas *canvas);
   void ManageFrameListInCache(ImageCache *cache, FrameList *frameList);
   Image *GetImageFromCache(ImageCache *cache, uint32_t frameNumber,
          const Rectangle *region, uint32_t levelOfDetail);
   void ReleaseImageFromCache(ImageCache *cache, Image *image);
   void PreloadImageIntoCache(ImageCache *cache, uint32_t frameNumber, 
            const Rectangle *region, uint32_t levelOfDetail);
   void PrintCache(ImageCache *cache);
   void GetCacheConfiguration(ImageCache *cache, int *numReaderThreads, int *maxCachedImages);
   void CachePreload(Canvas *canvas, uint32_t frameNumber, 
                     const Rectangle *imageRegion, uint32_t levelOfDetail);
#endif
