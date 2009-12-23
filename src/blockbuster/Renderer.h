#ifndef BB_RENDERER_H
#define BB_RENDERER_H
#include "cache.h"
#include "settings.h"
struct Canvas; 
/* Base class for all other renderers, defining the API required */ 
class NewRenderer {
 public:
  NewRenderer(ProgramOptions *opt, Canvas *canvas): 
    // replaces Initialize() from file module (e.g. gl.cpp)
    name(NULL), description(NULL), mCanvas(canvas), mOptions(opt) {
    return; 
  } 

  virtual ~NewRenderer() {
    return; 
  } // replaces DestroyRenderer from Canvas


  virtual void HandleOptions(int &argc, char *argv[]) = 0; // from Renderer structs in file module (e.g. gl.cpp)
  
  
  /* Functions that are function pointers in the Canvas class right now */ 
  /* The fundamental operation of the Renderer is to render.        This might be assigned gl_Render (gl.cpp, gl_Initialize), x11_Render (x11.cpp: x11_initialize()), or dmx_Render (dmxglue.cpp, dmx_Initialize()).  The assignment is done 
   */
  // from Canvas class 
  virtual void Render(int frameNumber,
                      const Rectangle *imageRegion,
                      int destX, int destY, float zoom, int lod) = 0;
  // from Canvas class 
  virtual void SetFrameList(FrameList *frameList) {
    mFrameList = frameList; 
  }
    
  // from Canvas class 
  virtual void Preload(uint32_t frameNumber,
                       const Rectangle *imageRegion, uint32_t levelOfDetail);


  //void *rendererPrivateData; // from Canvas class -- shouldn't need this
  
  /* This becomes the constructor: */ 
  //virtual MovieStatus Initialize(struct Canvas *canvas, const ProgramOptions *options) = 0; */
  /*  This becomes the destructor: 
   */
  // virtual void DestroyRenderer(struct Canvas *canvas) = 0;
  
 protected:   
  /* If the renderer needs a handle on which to hang any privately
   * allocated data, this is the place to do it.
   */
  char *name; // from Renderer structs in file module (e.g. gl.cpp)
  char *description;// from Renderer structs in file module (e.g. gl.cpp)
  
  // these are good to have around to reduce arguments: 
  Canvas *mCanvas; 
  ProgramOptions *mOptions; 


  FrameList *mFrameList;
  ImageCache *mCache; // if not using DMX
} ;

#endif
