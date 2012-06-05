#include "canvas.h"
#include "Renderer.h"
#include "common.h"
#include "glRenderer.h"
#ifndef NO_DMX
#include "dmxRenderer.h"
#endif
#include "settings.h"

Renderer * Renderer::CreateRenderer(ProgramOptions *opt, Canvas *canvas, Window parentWindow) {
  QString name = opt->rendererName; 
  Renderer *renderer = NULL; 

  INFO("CreateRenderer creating renderer of type \"%s\"\n", name.toStdString().c_str()); 

  if (name == "gl" || name == "") renderer = new glRenderer(opt, canvas, parentWindow); 
  if (name == "gl_stereo") renderer = new glStereoRenderer(opt, canvas, parentWindow); 
  if (name == "gltexture") renderer = new glTextureRenderer(opt, canvas, parentWindow); 
#ifdef USE_DMX
  if (name == "dmx") renderer = new dmxRenderer(opt, canvas, parentWindow); 
#endif  
  // this has to be called after ChooseVisual() virtual functions are in place
  renderer->FinishInit(opt, canvas, parentWindow); 
  return renderer;
  
}

Renderer::Renderer(ProgramOptions *opt, Canvas *canvas, Window parentWindow, QString name):
  // replaces Initialize() from file module (e.g. gl.cpp)
  XWindow(opt, canvas, parentWindow), 
  mName(name), mCanvas(canvas), mOptions(opt){  

  mCache = CreateImageCache(mOptions->readerThreads,
                            mOptions->frameCacheSize, mCanvas);
  return; 
} 

void Renderer::FinishInit(ProgramOptions *opt, Canvas *canvas, Window parentWindow) {
  ECHO_FUNCTION(5); 
  visInfo = ChooseVisual(); 
  FinishXWindowInit(opt, canvas, parentWindow); 
  FinishRendererInit(opt, canvas, parentWindow); 
  return;
}

void Renderer::SetFrameList(FrameList *frameList) {
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

void Renderer::Preload(uint32_t frameNumber,
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

//=====================================================================
/* 
   Preload a length of frames
   startFrame -- 
*/ 
void Renderer::Preload(uint32_t frameNumber, uint32_t preloadFrames, 
                       int playDirection, 
                       uint32_t minFrame, uint32_t maxFrame,
                       const Rectangle *imageRegion, 
                       uint32_t levelOfDetail) {
  
  uint32_t preloadmax = maxFrame-minFrame, 
    frame=frameNumber, preloaded=0;
  if (preloadmax > preloadFrames) {
    preloadmax = preloadFrames;
  }
  while (preloaded++ < preloadmax) {
    frame += playDirection; 
    if (frame > maxFrame) frame = minFrame; 
    if (frame < minFrame) frame = maxFrame; 
    DEBUGMSG("Preload frame %d", frame); 
    this->Preload(frame, imageRegion, levelOfDetail);
  }
  return; 
}
