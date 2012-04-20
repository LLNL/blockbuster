
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

  return  StreamingMovie::ReadHeader(); 
 
} 

/*
  Start the Reader thread and the given number of decompressors. 
 */ 
void BufferedStreamingMovie::StartThreads(int numthreads){

  boost::lock_guard<boost::shared_mutex> lock(mRenderBuffer.mMutex); 
  mRenderBuffer.SetFrameSize(mFrameSizes[mLOD][0]*mFrameSizes[mLOD][1]*3); 

  // start reading frames... 
  mReaderThread = boost::thread(&BufferedStreamingMovie::ReaderThread, this); 
  int id = 0; 
  while (id < mNumDecompressorThreads) {
    //boost::thread theThread = ;
    mDecompressorThreads.push_back(boost::make_shared<boost::thread>(&BufferedStreamingMovie::DecompressorThread, this, id) ); 
    id++; 
  }

}


bool BufferedStreamingMovie::DisplayFrame(uint32_t framenum){

  bool found=false; 
  mCurrentImage.mFrameNumber = framenum; // no mutex required

  {   // if the render buffer has the image, then display it.  
    boost::lock_guard<boost::shared_mutex> lock(mRenderBuffer.mMutex); 
    if (mRenderBuffer.mFrameSlots[framenum]) {
      mDisplayer.display(mRenderBuffer.mFrameSlots[framenum]->mCimg); 
      found=true; 
    } 
  }
  mRenderBuffer.mCondition.notify_all(); 
  mDoubleIOBuffer.mCondition.notify_all(); 
  return found; 
}
