#ifndef DMXRENDERER_H
#define DMXRENDERER_H yes

#include "Renderer.h" // not "Renderers.h"

class dmxRenderer: public NewRenderer {
 public:
  dmxRenderer(ProgramOptions *opt, Canvas *canvas):
    NewRenderer(opt, canvas, "dmx") {}
  virtual ~dmxRenderer() {}
  //   void HandleOptions(int &argc, char *argv[]);

  void Render(int frameNumber,
              const Rectangle *imageRegion,
              int destX, int destY, float zoom, int lod);
  
  void SetFrameList(FrameList *frameList);
  void Preload(uint32_t frameNumber,
               const Rectangle *imageRegion, uint32_t levelOfDetail);
   
  
} ;
#endif
