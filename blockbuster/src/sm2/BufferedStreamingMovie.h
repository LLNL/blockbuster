#ifndef BUFFERED_STREAMING_MOVIE_H
#define BUFFERED_STREAMING_MOVIE_H

#include "StreamingMovie.h"
#include <boost/thread.hpp>
#include "CImg.h"

#define BUFFER_SIZE (10*1000*1000)
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
struct RenderBuffer {
 public:
  RenderBuffer() {
    SetBufferSize(2*BUFFER_SIZE);
  }
  ~RenderBuffer() {}

  void SetBufferSize(uint64_t numbytes) {
    mBufferSize = numbytes; 
  }

  void SetFrameSize(uint32_t numbytes) {
    mFrameSize = numbytes; 
  }

  int mLOD; // level of detail
  uint64_t mBufferSize; // maximum size in bytes
  uint32_t mFrameSize; // computed bytes per frame at mLOD
  vector<boost::shared_ptr<RenderedImage> > mFrameSlots; // quick lookup table
  vector<RenderedImage> mRenderedImages; // actual frame storage
  boost::shared_mutex mMutex; 
  boost::condition_variable mCondition; 
}; 
 
//===============================================
/* 
   IOBuffer:  Buffers for frames.  
*/ 
//===============================================
/*
  IO Buffer
  No mutex here because that's taken care of in the DoubleIOBuffer class
*/ 

struct IOBuffer {
  IOBuffer():mNumFrames(0) {}
  ~IOBuffer() {}

  void init(void) {
    mRawData.resize(BUFFER_SIZE); 
  }
  
  int mLOD; // level of detail
  vector<uint32_t> mFrameOffsets; // offsets into our raw data to find frames
  uint32_t mFirstFrameNum, mNumFrames; // describes what the FrameReader put here
  uint32_t mNextFrameNum; // Tells us how much work has been taken by Decompressors
  vector<unsigned char> mRawData; // raw data from disk

}; 

//===============================================
/* 
   Encapsulates the IO Buffers and their mutexes
*/ 
struct DoubleIOBuffer {
 public:
  DoubleIOBuffer(){}
  ~DoubleIOBuffer(){}

  /* Set the size of both buffers in the double buffer */ 
  void SetBufferSizes(uint32_t size) {
    boost::lock_guard<boost::shared_mutex> lock(mMutex); 
    mDoubleBuffer[0].mRawData.resize(size); 
    mDoubleBuffer[1].mRawData.resize(size); 
  }

  IOBuffer mDoubleBuffer[2]; 
  IOBuffer *mFrontBuffer, *mBackBuffer; 
  boost::shared_mutex mMutex; 
  boost::condition_variable mCondition; 

};



//===============================================
  class Renderer {
 public:
  Renderer() {}
  ~Renderer() {}
  
  boost::shared_ptr <RenderedImage> GetImage(uint32_t framenum); 
  
 private:

}; 


//===============================================
/* 
   This class encapsulates the entire Streaming Movie system in a convenient container. 
   It contains the display, rendering and movie reading pieces, basically all of the "main thread" activities.  Contains and controls the buffers and threads.  
*/ 
class BufferedStreamingMovie:public StreamingMovie {
 public: 
  BufferedStreamingMovie() {
    return; 
  }
  BufferedStreamingMovie(std::string filename):StreamingMovie(filename) {
    return;
  }
  ~BufferedStreamingMovie(){
    return; 
  }

  bool ReadHeader(void); // virtual function

  void SetFileName(std::string filename) {    
    mFileName = filename; 
  }

  void SetBufferSizes(uint32_t bufsize) {
    mDoubleIOBuffer.SetBufferSizes(bufsize); 
    mRenderBuffer.SetBufferSize(bufsize*2); 
  }

  // this function lives in its own thread
  void ReaderThread(){
    mKeepReading=true; 
    while (mKeepReading) {
      ; 
    }
  } 

  // this function lives in its own thread
  void DecompressorThread(int threadID) {
    mKeepDecompressing = true; 
    while (mKeepDecompressing) {
      ; 
    }
  }


  void StartThreads(int numthreads) ;

  // use own CImgDisplay to show the frame -- no copy needed
  bool DisplayFrame(uint32_t framenum);

 private:
  boost::thread mReaderThread; 
  bool mKeepReading; 

  int mNumDecompressorThreads; 
  std::vector<boost::shared_ptr<boost::thread> > mDecompressorThreads; 
  bool mKeepDecompressing; 

  DoubleIOBuffer mDoubleIOBuffer; 
  RenderBuffer mRenderBuffer; 
  
  CImgDisplay mDisplayer; 
  // Current frame info: 
  Image mCurrentImage;  // what's being displayed right now
  
}; 


#endif
