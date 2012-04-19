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
}; 

//===============================================
class RenderBuffer {
 public:
  RenderBuffer() {}
  ~RenderBuffer() {}

  // synchronous mutexed function to store request and get an image back
  bool RequestImage(const Image &image, CImg<unsigned char> &cimg); 

  // grab a shared lock find out what is needed next in this buffer
  void NextRequestedImage(Image &query) const; 


  // mutexed function.  Returns false if the current view doesn't match the frame or the frame is otherwise undesirable.  
  bool DepositFrame(RenderedImage &frame); 
 private: 
  Image mRequestedImage; // what the Renderer last requested -- each complete frame will have its own state based on what mRequestedState was when it was begun.  

  boost::mutex mMutex; 
  boost::condition_variable mWaitingForImage; 
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

    void RequestImage(const Image &image); 

 private: 
  Image mRequestedImage; // what the Renderer last requested

  IOBuffer mDoubleBuffer[2]; 
  IOBuffer *mFrontBuffer, *mBackBuffer; 
  boost::shared_mutex mMutex; 

};



//===============================================
  class Renderer {
 public:
  Renderer() {}
  ~Renderer() {}
  

 private:

  // Current frame info: 
  Image mCurrentImage;  // what's being displayed right now
  
  boost::shared_mutex mFrameViewMutex; 
}; 

//===============================================
/*
  Producer half of the producer/consumer I/O model. 
  Read the movie in multi-megabyte chunks, buffer it. 
*/ 

class Reader {
 public:
  Reader(){ }
  Reader(std::string filename):mFileName(filename){}
  ~Reader(){}

  void SetFileName(std::string filename) {
    mFileName = filename; 
  }

  void StartThread(void) {
    mThread = boost::thread(&Reader::run, this); 
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

  void StartThreads (int numthreads) {
    mDecompressors.clear(); 
    //mDecompressors.resize(numthreads); 
    int i=numthreads; 
    while (i<numthreads) {
      mDecompressors.push_back(Decompressor()) ;
      mDecompressors[i].StartThread(i); 
    }
    mReader.StartThread(); 
    return; 
  }

  // use own CImgDisplay to show the frame -- no copy needed
  bool DisplayFrame(uint32_t framenum){
    return false; 
  }

 private:
    Reader mReader; 
    std::vector<Decompressor> mDecompressors; 
    DoubleIOBuffer mDoubleIOBuffer; 
    RenderBuffer mRenderBuffer; 

    CImgDisplay mDisplay; 
}; 


#endif
