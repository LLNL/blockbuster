

void ReaderThread::run() {
  
}

FrameCache::FrameCache(int numthreads, FrameListPtr fl, ImageFormat &required) {
  // Figure out how many buffers we need.  Put 250 MB per buffer for now.  
  mBytesPerFrame = fl->mWidth * fl->mHeight * 3; 
  int64_t totalBytes = 
  // 
  return; 
}

