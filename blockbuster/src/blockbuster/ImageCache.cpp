
#include "ImageCache.h"
#include "ImageCacheElement.h"


void NewImageCache::ManageFrameList(FrameListPtr frameList) {
  mFrameList = frameList; 
  mFrameList->ReleaseFramesFromCache(); 
  return; 
}
