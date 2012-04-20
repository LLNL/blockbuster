
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
  mCurrentImage.mFrameNumber = 0;
  mRenderedImages.resize(mUserPrefs.mBufferSize/(mFrameSizes[mUserPrefs.mLOD][0]*mFrameSizes[mUserPrefs.mLOD][1]*3)); 
  return retval; 
} 

/*
  Start the Reader thread and the given number of decompressors. 
 */ 
void BufferedStreamingMovie::StartThreads(int numthreads){

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


bool BufferedStreamingMovie::DisplayFrame(uint32_t framenum){

  bool found=false; 
  mCurrentImage.mFrameNumber = framenum; // no mutex required
 
  {   // if the render buffer has the image, then display it.  
    boost::lock_guard<boost::mutex> lock(mRenderMutex); 
    if (mRenderedFrameLookup[framenum]) {
      mDisplayer.display(mRenderedFrameLookup[framenum]->mCimg); 
      found=true; 
    } 
  }
  mRenderCondition.notify_all(); 
  mIOBufferCondition.notify_all(); 
  return found; 
}
