#include "Renderers.h"
#include "blockbuster_x11.h"
#include "blockbuster_gl.h"
#include "gltexture.h"
#include "dmxglue.h"

Renderer x11Renderer = {
  X11_NAME,
  X11_DESCRIPTION,
  x11_Initialize
};


Renderer glRenderer = {
  GL_NAME,
  GL_DESCRIPTION,
  gl_Initialize
};

Renderer glRendererStereo = {
  GL_NAME_STEREO,
  GL_DESCRIPTION_STEREO,
  gl_InitializeStereo
};

Renderer glTextureRenderer = {
  GLTEXTURE_NAME,
  GLTEXTURE_DESCRIPTION,
  gltexture_Initialize
};


#ifdef USE_DMX
Renderer dmxRenderer = {
  "dmx",
  "backend renderer",
  dmx_Initialize
};

#endif
