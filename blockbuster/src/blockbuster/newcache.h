#ifndef NEWCACHE_H
#define NEWCACHE_H -42

/* 
   IOBuffers contain a read buffer for the reader and a decompressed buffer for frame data at a given resolution.  
   The read buffer is a contiguous chunk of memory read straight from disk.  
   Need to track first frame an
*/ 
typedef boost::shared_ptr<struct MovieBuffer> MovieBufferPtr; 
struct MovieBuffer {
  int mLOD; 
  int mFirstFrame; 
  int mNumFrames; 
  vector<int64_t> mRawOffsets; 
  boost::shared_ptr<char> mRawBytes; 
  boost::shared_ptr<char> mDecompressedFrames; 
}; 

typedef boost::shared_ptr<class ReaderThread> ReaderThreadPtr; 
class ReaderThread {
  void run(); 
}; 

class DecompressorThread {
  void run(); 

}; 

class FrameCache {
  FrameCache(int numthreads, int numimages, ImageFormat &required);
  ~FrameCache(); 
  
  ImagePtr GetImage(uint32_t frameNumber,
                  const Rectangle *newRegion, uint32_t levelOfDetail);

  // This also seems bad.  We should pass a list of files to the cache and let it manage how to make images out of them.  
  void ManageFrameList(FrameListPtr frameList);
  
    return; 
  }


  
}; 


#endif
