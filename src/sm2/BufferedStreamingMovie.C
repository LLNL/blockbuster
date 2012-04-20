
#include "BufferedStreamingMovie.h"
#include <boost/make_shared.hpp>

// using namespace boost; 

bool BufferedStreamingMovie::ReadHeader(void) {  
  // remove all instructions to do any work from the IOBuffer
  mKeepDecompressing = false; 
  mKeepReading = false; 

  if (mReaderThread.get_id() != boost::thread::id()) {
    mReaderThread.join(); 
  } 
 
  int id = 0; 
  while (id < mDecompressorThreads.size()) {
    if (mDecompressorThreads[id]->get_id() != boost::thread::id()) {
      mDecompressorThreads[id]->join(); 
    }
    ++id; 
  } 
  mDecompressorThreads.clear(); 

  bool retval =  StreamingMovie::ReadHeader(); 

  mRenderedFrameLookup.clear(); 
  mRenderedFrameLookup.resize(mNumFrames);
  mRenderedImages.resize(mUserPrefs.mBufferSize/(mFrameSizes[mUserPrefs.mLOD][0]*mFrameSizes[mUserPrefs.mLOD][1]*3));
 
  mCurrentImage.mFrameNumber = 0; // the cake is a lie -- what should I do?  
  mNextImage.mFrameNumber = 0;

  return retval; 
} 


/*
  Start the Reader thread and the given number of decompressors. 
 */ 
void BufferedStreamingMovie::StartThreads(void){


  boost::lock_guard<boost::mutex> lock(mRenderMutex);   

  // start reading frames... 
  mReaderThread = boost::thread(&BufferedStreamingMovie::ReaderThread, this); 
  int id = 0; 
  while (id < mNumDecompressorThreads) {
    //boost::thread theThread = ;
    mDecompressorThreads.push_back(boost::make_shared<boost::thread>(&BufferedStreamingMovie::DecompressorThread, this, id) ); 
    id++; 
  }
  return; 
}

// this function lives in its own thread
void BufferedStreamingMovie::ReaderThread(){
  mKeepReading=true; 
  while (mKeepReading) {
    // Find out if the buffer states make sense
    boost::shared_lock<boost::shared_mutex> lock(mIOBufferMutex); 
    // Check to see if there is anything in the IOFrontBuffer
    ; 
  }
} 

// this function lives in its own thread
void BufferedStreamingMovie::DecompressorThread(int threadID) {
  mKeepDecompressing = true; 
  while (mKeepDecompressing) {
    ; 
  }
}


bool BufferedStreamingMovie::DisplayFrame(uint32_t framenum){

  mNextImage.mFrameNumber = framenum; 
  
  bool found=false; 
  if (!mKeepReading) {
    StartThreads(); 
  }
  
  // CRITICAL SECTION --------------------------
  {   // if the render buffer has the image, then display it.  
    boost::unique_lock<boost::mutex> lock(mRenderMutex); 
    while (!mRenderedFrameLookup[framenum]) {
      mRenderCondition.wait(lock); 
    }
    mDisplayer.display(mRenderedFrameLookup[framenum]->mCimg); 
    mCurrentImage.mFrameNumber = framenum; // no mutex required
    mNextImage.mFrameNumber = framenum+1; 
    found=true;  
  }

  mRenderCondition.notify_all(); 
  mIOBufferCondition.notify_all(); 
  return found; 
}
