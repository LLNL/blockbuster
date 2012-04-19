
#include "BufferedStreamingMovie.h"

using namespace boost; 

// Called by Render thread when it needs an image
bool RenderBuffer::RequestImage(const Image &image, CImg<unsigned char> &cimg) {
  lock_guard<mutex> lock(mMutex); 
  mRequestedImage = image; 
  
  // find image in buffer (or sleep)
  /* while (!haveImage(image)) {
    mWaitingForImage.wait(lock); 
  }
  */
  return false; 
}


bool RenderBuffer::DepositFrame(RenderedImage &image) {
  {
    lock_guard<mutex> lock(mMutex); // need exclusive access for writing
    // AddImageToBuffer(image); 
  }
  mWaitingForImage.notify_one();// only the Renderer might be waiting 
  return false; 
}




void DoubleIOBuffer::RequestImage(const Image &image) {
    lock_guard<shared_mutex> lock(mMutex); 
    mRequestedImage = image; 
    return; 
}
