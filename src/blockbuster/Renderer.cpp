#include "Renderer.h"
#include "common.h"

void NewRenderer::Preload(uint32_t frameNumber,
                     const Rectangle *imageRegion, uint32_t levelOfDetail) {
  /* code stolen from cache.cpp CachePreload() 
     This is the appropriate code for all but DMX renderers. 
  */ 
  Rectangle lodROI;
  uint32_t localFrameNumber = 0;
  
  /* Adjust ROI for LOD! (very important) */
  lodROI.x = imageRegion->x >> levelOfDetail;
  lodROI.y = imageRegion->y >> levelOfDetail;
  lodROI.width = imageRegion->width >> levelOfDetail;
  lodROI.height = imageRegion->height >> levelOfDetail;
  
  
  if(mFrameList->stereo) {
    localFrameNumber = frameNumber * 2;
    mCache->PreloadImage( localFrameNumber++,
                          &lodROI, levelOfDetail);
    mCache->PreloadImage(localFrameNumber,
                         &lodROI, levelOfDetail);
  }
  else {
    localFrameNumber = frameNumber;
    mCache->PreloadImage(localFrameNumber,
                         &lodROI, levelOfDetail);
  }   
  return; 
}
