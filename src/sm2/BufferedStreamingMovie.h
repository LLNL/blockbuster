#ifndef BUFFERED_STREAMING_MOVIE_H
#define BUFFERED_STREAMING_MOVIE_H

#include "StreamingMovie.h"
#include <boost/thread.hpp>
#include "CImg.h"
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
  RenderBuffer() {}
  ~RenderBuffer() {}

  vector<RenderedImage> mRenderedImages; 
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
  Producer half of the producer/consumer I/O model. 
  Read the movie in multi-megabyte chunks, buffer it. 
*/ 

class FrameReader {
 public:
  FrameReader(){ }
  FrameReader(std::string filename):mFileName(filename){}
  ~FrameReader(){}

  void SetFileName(std::string filename) {
    mFileName = filename; 
  }

  void StartThread(void) {
    mThread = boost::thread(&FrameReader::run, this); 
  }

 
  // this function starts a new thread
  void run(){
    mKeepRunning=true; 
    while (mKeepRunning) {
      ; 
    }
  } 
  
  bool mKeepRunning; 
  boost::thread mThread; 
  std::string mFileName; 
};

//===============================================
/*
  Consumes raw frames and produces decompressed frames
  This is a functor to work with 
*/ 
class Decompressor { 
 public:
  Decompressor(){
    return; 
  }
  Decompressor(const Decompressor &other) {
    *this = other; 
  }

  Decompressor &operator =(const Decompressor &other) {
    this->mThreadID = other.mThreadID; 
    this->mKeepRunning = other.mKeepRunning; 
    // this->mThread = other.mThread; // NOT ALLOWED
    return *this; 
  }

  ~Decompressor() {
    // stop thread and clean up memory
    return; 
  }
  
  void StartThread(int threadID) {
    mThreadID = threadID; 
    mThread = boost::thread(&Decompressor::run, this); 
  }

  void run(){
    mKeepRunning=true; 
    while (mKeepRunning) {
      ; 
    }
  } 
  
  bool mKeepRunning; 
  int mThreadID; 
  boost::thread mThread; 
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
  BufferedStreamingMovie(std::string filename):StreamingMovie(filename),mFrameReader(filename) {
    return;
  }
  ~BufferedStreamingMovie(){
    return; 
  }

  void SetFileName(std::string filename) {    
    mFrameReader.SetFileName(filename); 
  }

  void SetBufferSizes(uint32_t bufsize) {
    mDoubleIOBuffer.SetBufferSizes(bufsize); 
  }

  void StartThreads (int numthreads) {
    mDecompressors.clear(); 
    //mDecompressors.resize(numthreads); 
    int i=numthreads; 
    while (i<numthreads) {
      mDecompressors.push_back(Decompressor()) ;
      mDecompressors[i].StartThread(i); 
    }
    mFrameReader.StartThread(); 
    return; 
  }

  // use own CImgDisplay to show the frame -- no copy needed
  bool DisplayFrame(uint32_t framenum);

 private:
    FrameReader mFrameReader; 
    std::vector<Decompressor> mDecompressors; 
    DoubleIOBuffer mDoubleIOBuffer; 
    RenderBuffer mRenderBuffer; 

    CImgDisplay mDisplay; 
    // Current frame info: 
    Image mCurrentImage;  // what's being displayed right now
    
    boost::shared_mutex mFrameViewMutex; 
}; 


#endif
