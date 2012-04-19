
#include "BufferedStreamingMovie.h"

using namespace boost; 

bool BufferedStreamingMovie::ReadHeader(void) {  
  StreamingMovie::ReadHeader(); 
  mRenderBuffer.SetFrameSize(mFrameSizes[mLOD][0]*mFrameSizes[mLOD][1]*3); 
}


bool BufferedStreamingMovie::DisplayFrame(uint32_t framenum){
  // update the current image: 
  mCurrentImage.mFrameNumber = framenum; 

  // if the render buffer has the image, then display it.   
  if (mRenderBuffer.mFrameSlots[framenum]) {
    mDisplayer.display(mRenderBuffer.mFrameSlots[framenum]->mCimg); 
  }
  // else, 
  return false; 
}
