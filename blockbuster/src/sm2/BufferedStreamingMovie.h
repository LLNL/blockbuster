#ifndef BUFFERED_STREAMING_MOVIE_H
#define BUFFERED_STREAMING_MOVIE_H

#include "StreamingMovie.h"
#include <boost/thread.hpp>

//===============================================
/* 
   Intended to allow atomic encapsulation of view state for image creation, decompression and viewing. 
   The atomicity is enforced by the SMRenderer.  
 */ 
struct RenderState {
  uint32_t mCurrentDisplayFrame;
  // also will include image size, LOD, image location, etc. 
}; 

//===============================================
/* 
   Contains everything needed to store a frame in the RenderBuffer, including the view that it was created in. 
*/ 
struct RenderedFrame {
  RenderedFrame(){}
  ~RenderedFrame(){}
  RenderState mRenderState; 
}; 

//===============================================
class RenderBuffer {
 public:
  RenderBuffer() {}
  ~RenderBuffer() {}

  // mutexed function to allow renderer to change the view
  void SetState(const RenderState &view); 

  // grab a shared lock and get a copy of the current render view: 
  void GetRenderState(RenderState &query) const; 

  // mutexed function.  Returns false if the current view doesn't match the frame or the frame is otherwise undesirable.  
  bool DepositFrame(RenderedFrame &frame); 
 private: 
  RenderState mRenderState; 

  boost::mutex mMutex; 
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
#define BUFFER_SIZE (10*1000*1000)

struct IOBuffer {
  IOBuffer() {}
  ~IOBuffer() {}

  void init(void) {
    mRawData.resize(BUFFER_SIZE); 
  }
  
  int mLOD; // level of detail
  uint32_t mFirstFrameNum, mNumFrames; // describes what the Reader put here
  uint32_t mNextFrameNum; // Tells us how much work has been taken by Decompressors
  vector<unsigned char> mRawData; // raw data from disk

}; 

//===============================================
/* 
   Encapsulates the IO Buffers and their mutexes
*/ 
class DoubleIOBuffer {
 public:
  DoubleIOBuffer(){}
  ~DoubleIOBuffer(){}

  /* Set the size of both buffers in the double buffer */ 
  void SetBufferSizes(uint32_t size) {
    boost::lock_guard<boost::shared_mutex> lock(mMutex); 
    mDoubleBuffer[0].mRawData.resize(size); 
    mDoubleBuffer[1].mRawData.resize(size); 
  }

  
 private: 
  IOBuffer mDoubleBuffer[2]; 
  IOBuffer *mFrontBuffer, *mBackBuffer; 
  boost::shared_mutex mMutex; 

};


//===============================================
class Renderer {
 public:
  Renderer() {}
  ~Renderer() {}
  
  RenderState GetRenderState(void) {
    boost::shared_lock<boost::shared_mutex> lock(mFrameViewMutex);
    return mRenderState;
  }

 private:
  void SetCurrentDisplayFrame(uint32_t f) {
    boost::lock_guard<boost::shared_mutex> lock(mFrameViewMutex);
    mRenderState.mCurrentDisplayFrame = f; 
  }

  // Current frame info: 
  RenderState mRenderState; 

  boost::shared_mutex mFrameViewMutex; 
}; 

//===============================================
/*
  Producer half of the producer/consumer I/O model. 
  Read the movie in multi-megabyte chunks, buffer it. 
*/ 

class Reader {
 public:
  Reader(){}
  Reader(std::string filename):mFileName(filename){}
  ~Reader(){}

  void SetFileName(std::string filename) {
    mFileName = filename; 
  }

  // this function starts a new thread
  void run(); 
  
 private:
  std::string mFileName; 

};

//===============================================
/*
  Consumes raw frames and produces decompressed frames
*/ 
class Decompressor { 
 public:
  Decompressor() {}
  ~Decompressor() {
    // stop thread and clean up memory
    return; 
  }

  // this function starts a new thread
  void run() {
  }
  
};

//===============================================
/* 
   This class encapsulates the entire Streaming Movie system in a convenient container. 
*/ 
class BufferedStreamingMovie:public StreamingMovie {
 public: 
  BufferedStreamingMovie() {
    return; 
  }
  BufferedStreamingMovie(std::string filename):StreamingMovie(filename),mReader(filename) {
    return;
  }
  ~BufferedStreamingMovie(){
    return; 
  }

  void SetFileName(std::string filename) {    
    mReader.SetFileName(filename); 
  }

  void SetBufferSizes(uint32_t bufsize) {
    mDoubleIOBuffer.SetBufferSizes(bufsize); 
  }

 private:
    Reader mReader; 
    vector<Decompressor> mDecompressors; 
    DoubleIOBuffer mDoubleIOBuffer; 
    RenderBuffer mRenderBuffer; 

}; 


#endif
