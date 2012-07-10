
#include "BufferedStreamingMovie.h"
#include <boost/make_shared.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "iodefines.h"

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

//=======================================================
/*
  BufferFrames():  
  buffer from firstFrame to end or maximum capacity.
  Always buffer to back buffer
*/ 
void BufferedStreamingMovie::BufferFrames(int fd, uint32_t firstFrame) { 
  uint32_t virtualFrame = mLOD * mNumFrames + firstFrame, 
    lastFrameToRead = (mLOD+1) * mNumFrames - 1;
  uint64_t endOfLOD = mFrameOffsets[mNumFrames*(mLOD+1)]; // always OK, no segfault
  uint64_t bytesToRead = endOfLOD - mFrameOffsets[virtualFrame]; 
  
  while (bytesToRead > mBackIOBuffer->mData.size()) {
    bytesToRead -= mFrameOffsets[lastFrameToRead--]; 
  }
  
  uint64_t pos = LSEEK64(fd, mFrameOffsets[virtualFrame], SEEK_SET);

  read(fd, &mBackIOBuffer->mData[0], bytesToRead); 
  mBackIOBuffer->mFirstFrame = firstFrame; 
  mBackIOBuffer->mNumFrames = lastFrameToRead - firstFrame + 1; 
  return; 
}

// this function lives in its own thread
void BufferedStreamingMovie::ReaderThread(){
  mKeepReading=true; 
  int IOfd = open(mFileName.c_str(), O_RDONLY);
  
  // PRIMARY GOAL: PLACE DATA FOR mNextFrame INTO THE FRONT BUFFER
  // SECONDARY GOAL:  FILL THE BACK BUFFER
  
  while (mKeepReading) {
    // first see if all is well, in which case we can sleep until needed 
    // we mutex this one because a mistake could cause deadlock
    { 
      boost::unique_lock<boost::mutex> lock(mIOBufferMutex); 
      if (mFrontIOBuffer->HasFrame(mNextImage.mFrameNumber)) {
	uint32_t backBufferFirstFrame = mFrontIOBuffer->mFirstFrame + mFrontIOBuffer->mNumFrames; 
	if (backBufferFirstFrame == mNumFrames) {
	  backBufferFirstFrame = 0; 
	}
	if (mBackIOBuffer->HasFrame(backBufferFirstFrame)) {
	  mIOBufferCondition.wait(lock); 
	}
      }
    }
    // Someone woke us up or we noticed a buffer needs repair.  
    while (!mFrontIOBuffer->HasFrame(mNextImage.mFrameNumber)) {
      // If the front IO buffer doesn't have the next frame, then it couldn't be decompressed.  
      while (!mBackIOBuffer->HasFrame(mNextImage.mFrameNumber)) {
	// This is the worst case.  We must read the needed frame right now.  
	// The good thing is no mutexes are needed.  :-) 
	// Assume no special pipeline is needed for just the first frame -- spend the time to read the whole buffer rather than try to do one at a time.  

	BufferFrames(IOfd, mNextImage.mFrameNumber); 
      }
      // now it's in the back buffer -- we need to swap buffers
      boost::unique_lock<boost::mutex> lock(mIOBufferMutex); 
      IOBuffer *tmp = mFrontIOBuffer; 
      mFrontIOBuffer= mBackIOBuffer; 
      mBackIOBuffer = tmp; 
    } 
    // we have the next frame in the front buffer, so wake up decompressors if they are waiting
    mIOBufferCondition.notify_all();  
    
    // Now let's take care of the back buffer while we're awake
    uint32_t nextFrame = mFrontIOBuffer->mFirstFrame + mFrontIOBuffer->mNumFrames; 
    if (nextFrame == mNumFrames) {
      nextFrame = 0; 
    }
    if (!mBackIOBuffer->HasFrame(nextFrame)) {
      BufferFrames(IOfd, nextFrame); 
    } 
  }
    
  close(IOfd); 
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
