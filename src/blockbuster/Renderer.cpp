#include "Renderer.h"
#include "common.h"
#include "glRenderer.h"
#include "dmxRenderer.h"
#include "settings.h"
#include "canvas.h"

NewRenderer * NewRenderer::CreateRenderer(ProgramOptions *opt, Canvas *canvas, Window parentWindow) {
  QString name = opt->rendererName; 
  NewRenderer *renderer = NULL; 
  if (name == "gl" || name == "") renderer = new glRenderer(opt, canvas, parentWindow); 
  if (name == "gl_stereo") renderer = new glStereoRenderer(opt, canvas, parentWindow); 
  if (name == "gltexture") renderer = new glTextureRenderer(opt, canvas, parentWindow); 
  if (name == "dmx") renderer = new dmxRenderer(opt, canvas, parentWindow); 
  
  return renderer;
  
}

NewRenderer::NewRenderer(ProgramOptions *opt, Canvas *canvas, Window parentWindow, QString name):
  // replaces Initialize() from file module (e.g. gl.cpp)
  XWindow(canvas, opt, parentWindow), 
  mName(name), mCanvas(canvas), mOptions(opt){  
  mCache = CreateImageCache(mOptions->readerThreads,
                            mOptions->frameCacheSize, mCanvas);
  return; 
} 

void NewRenderer::SetFrameList(FrameList *frameList) {
  if (!mCache) {
    mCache = CreateImageCache(mCanvas->threads, mCanvas->cachesize, mCanvas);
  }
  if (mCache == NULL) {
    ERROR("could not recreate image cache when changing frame list");
    exit(1); 
  }
  
  /* Tell the cache to manage the new frame list.  This will
   * clear everything out of the cache that's already in there.
   */
  mCache->ManageFrameList(frameList); 
  mFrameList = frameList; 
  mCanvas->frameList = frameList; 
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
