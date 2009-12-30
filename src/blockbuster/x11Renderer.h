#ifndef X11RENDERER_H
#define X11RENDERER_H yes

#include "Renderer.h" // not "Renderers.h"

//====================================================================
// x11Renderer CLASS
//======================================================================
class x11Renderer: public NewRenderer {
 public:
  x11Renderer(ProgramOptions *opt, Canvas *canvas):
    NewRenderer(opt, canvas, "x11") {
   return; 
  }

  virtual ~x11Renderer() {}
  // this is a no-op for x11, no options are handled
  //  void HandleOptions(int &argc, char *argv[]);
  
  void Render(int frameNumber,
              const Rectangle *imageRegion,
              int destX, int destY, float zoom, int lod);
  
   
   
} ;

#endif
