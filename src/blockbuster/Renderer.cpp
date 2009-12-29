#include "Renderer.h"
#include "common.h"
#include "glRenderer.h"
#include "x11Renderer.h"
#include "dmxRenderer.h"
#include "settings.h"
#include "canvas.h"

void CreateRenderer(ProgramOptions *opt, Canvas *canvas) {
  QString name = opt->rendererName; 
  NewRenderer *renderer = NULL; 
  if (name == "x11") renderer = new x11Renderer(opt, canvas); 
  if (name == "gl") renderer = new glRenderer(opt, canvas); 
  if (name == "gl_stereo") renderer = new glStereoRenderer(opt, canvas); 
  if (name == "gltexture") renderer = new glTextureRenderer(opt, canvas); 
  if (name == "dmx") renderer = new dmxRenderer(opt, canvas); 
  
  opt->mRenderer = renderer;
  canvas->mRenderer = renderer; 
  return ;
  
}

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
