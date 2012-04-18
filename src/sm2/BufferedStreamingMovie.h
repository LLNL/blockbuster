#ifndef BUFFERED_STREAMING_MOVIE_H
#define BUFFERED_STREAMING_MOVIE_H

#include "StreamingMovie.h"
#include <boost/thread.hpp>


//===============================================
/* 
   Buffers for frames.  
*/ 
//===============================================

#define BUFFER_SIZE (10*1000*1000)

struct SMIOBuffer {
  SMIOBuffer() {}
  ~SMIOBuffer() {}

  void init(void) {
    mRawData.resize(BUFFER_SIZE); 
  }
  
  int mLOD; // level of detail
  uint32_t mFirstFrameNum, mNumFrames; // describes what the SMReader put here
  uint32_t mNextFrameNum; // Tells us how much work has been taken by SMDecompressors
  vector<unsigned char> mRawData; // raw data from disk

}; 


//===============================================
struct RenderInfo {
  uint32_t mCurrentDisplayFrame;
}; 

//===============================================
class SMRenderer {
 public:
  SMRenderer() {}
  ~SMRenderer() {}
  
  RenderInfo CurrentRenderInfo(void) {
    boost::shared_lock<boost::shared_mutex> lock(mFrameViewMutex);
    return mRenderInfo;
  }

 private:
  void SetCurrentDisplayFrame(uint32_t f) {
    boost::lock_guard<boost::shared_mutex> lock(mFrameViewMutex);
    mRenderInfo.mCurrentDisplayFrame = f; 
  }

  // Current frame info: 
  RenderInfo mRenderInfo; 

  boost::shared_mutex mFrameViewMutex; 
}; 

//===============================================
/*
  Producer half of the producer/consumer I/O model. 
  Read the movie in multi-megabyte chunks, buffer it. 
*/ 

class SMReader:public StreamingMovie {
 public:
  SMReader(string filename):StreamingMovie(filename){}
  ~SMReader(){}

  // this function starts a new thread
  void run(); 

  bool GetNextFrameRawData(uint32_t framenum, int lod, vector<unsigned char>&data) {
    
    boost::shared_lock<boost::shared_mutex> lock(mBufferMutex); 
    if (!mFrontBuffer) return false; 
    return true; 
  }
  
 private:
  
  SMIOBuffer mDoubleBuffer[2]; 
  SMIOBuffer *mFrontBuffer, *mBackBuffer; 
  boost::shared_mutex mBufferMutex; 
};

//===============================================
/*
  Consumes raw frames and produces decompressed frames
*/ 
class SMDecompressor { 
  SMDecompressor() {}
  ~SMDecompressor() {
    // stop thread and clean up memory
    return; 
  }

  // Called from the main thread; alerts the thread to a new buffer choice
  void SetBuffer(SMIOBuffer *fb) {    
    mPendingFrameBuffer = fb; 
  }

  // this function starts a new thread
  void run() {
  }
  
  SMIOBuffer *mCurrentFrameBuffer, 
    *mPendingFrameBuffer; 
};


 

#endif
