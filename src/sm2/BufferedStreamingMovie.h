#ifndef BUFFERED_STREAMING_MOVIE_H
#define BUFFERED_STREAMING_MOVIE_H

#include "StreamingMovie.h"
#include <boost/thread.hpp>
#include "CImg.h"

#define BUFFER_DEFAULT_SIZE (10*1000*1000)
//===============================================
/* 
   struct Image
   Useful base class for Rendered and Raw images, as well as queries
*/ 
struct Image {
  uint32_t mFrameNumber; 
  uint32_t mImageSize[2], mImagePos[2]; // what part of the frame is this image
}; 
//===============================================
/* 
   Contains everything needed to store a frame in the RenderBuffer, including the state that the Renderer was in when it was created. 
*/ 
struct RenderedImage:public Image {
  RenderedImage(){}
  ~RenderedImage(){}
  CImg<unsigned char> mCimg; // container for the data
  bool mValid; // 
}; 

//===============================================
/*
  IOBuffer:  a convenient encapsulation
*/ 
struct IOBuffer  {
  bool HasFrame(uint32_t framenum) {
    return (mNumFrames != 0) && 
      (framenum >= mFirstFrame) && 
      (framenum - mFirstFrame < mNumFrames); 
  }
  vector<unsigned char> mData; 
  uint32_t mFirstFrame, mNumFrames; 
};

//===============================================
/* 
   This class encapsulates the entire Streaming Movie system in a convenient container. 
   It contains the display, rendering and movie reading pieces, basically all of the "main thread" activities.  Contains and controls the buffers and threads.  
*/ 
class BufferedStreamingMovie:public StreamingMovie {
 public: 
  BufferedStreamingMovie() {
    init(); 
    return; 
  }
  BufferedStreamingMovie(std::string filename):StreamingMovie(filename) {
    init(); 
    return;
  }
  ~BufferedStreamingMovie(){
    return; 
  }

  void init(void ) {
    mFrontIOBuffer = &mIOBuffers[0]; 
    mBackIOBuffer = &mIOBuffers[1]; 
    mUserPrefs.mBufferSize = BUFFER_DEFAULT_SIZE; 
    mUserPrefs.mLOD = 0; 
    // buffers will be resized when movie information is known. 
  }
   

  bool ReadHeader(void); // virtual function

  void SetFileName(std::string filename) {    
    mFileName = filename; 
  }


  bool SetBufferSizes(uint32_t size) {
    if (mKeepReading || mKeepDecompressing) {
      cerr << "Warning:  cannot change buffer size while threads are active" << endl; 
      return false;  
    }
    mUserPrefs.mBufferSize = size; 
    return true; 
  }
  
  void SetNumThreads(int numthreads) {
    mNumDecompressorThreads = numthreads; 
  }

  void StartThreads(void);

  // buffer into back buffer from file. 
  void BufferFrames(int fd, uint32_t firstFrame);

  // this function lives in its own thread
  void ReaderThread();

  // this function lives in its own thread
  void DecompressorThread(int threadID);

  // use own CImgDisplay to show the frame -- no copy needed
  bool DisplayFrame(uint32_t framenum);

 private:
  boost::thread mReaderThread; 
  bool mKeepReading; 

  int mNumDecompressorThreads; 
  std::vector<boost::shared_ptr<boost::thread> > mDecompressorThreads; 
  bool mKeepDecompressing; 

  // Information from God (the user): 
  struct {
    uint64_t mBufferSize; // upper limit in bytes per buffer
    int mLOD; // defaults to 0
  } mUserPrefs; 

  // IO Buffering:
  IOBuffer mIOBuffers[2],  // raw data from disk, double buffered
    *mFrontIOBuffer, *mBackIOBuffer;
  boost::mutex mIOBufferMutex; 
  boost::condition_variable mIOBufferCondition; 

  // Decompressed frame Buffering (RenderBuffer):  
  vector<boost::shared_ptr<RenderedImage> > mRenderedFrameLookup; // quick lookup table
  vector<RenderedImage> mRenderedImages; // actual frame storage
  boost::mutex mRenderMutex; 
  boost::condition_variable mRenderCondition; 

  CImgDisplay mDisplayer; 
  // Current frame info: 
  Image mCurrentImage;  // what's being displayed right now
  Image mNextImage; // what is going to be displayed soon. 
}; 


#endif
