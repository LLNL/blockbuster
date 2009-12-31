#include "Renderers.h"
#include "blockbuster_x11.h"
#include "blockbuster_gl.h"
#include "gltexture.h"
#include "dmxglue.h"

OldRenderer x11Renderer = {
  "x11",
  "x11 renderer (does nothing)",
  NULL
};



OldRenderer glRenderer = {
  GL_NAME,
  GL_DESCRIPTION,
  gl_Initialize
};

OldRenderer glRendererStereo = {
  GL_NAME_STEREO,
  GL_DESCRIPTION_STEREO,
  gl_InitializeStereo
};

OldRenderer glTextureRenderer = {
  GLTEXTURE_NAME,
  GLTEXTURE_DESCRIPTION,
  gltexture_Initialize
};


#ifdef USE_DMX
OldRenderer dmxRenderer = {
  "dmx",
  "backend renderer",
  dmx_Initialize
};

OldRenderer *GetRendererByName(QString name) {
  if (name == "")  return &glRenderer; 

  if (name == "x11") {
    fprintf(stderr, "Error:  x11 renderer is no longer supported.\n"); 
    exit(1); 
  }

  if (name == "gl") return &glRenderer; 
  if (name == "gl_stereo") return &glRendererStereo; 
  if (name == "gltexture") return &glTextureRenderer; 
  if (name == "dmx") return &dmxRenderer; 
  return NULL; 
}

#endif
