#include "Renderers.h"
#include "blockbuster_x11.h"
#include "blockbuster_gl.h"
#include "gltexture.h"
#include "dmxglue.h"

Renderer x11Renderer = {
  X11_NAME,
  X11_DESCRIPTION,
  x11_HandleOptions,
  x11_Initialize
};


Renderer glRenderer = {
  GL_NAME,
  GL_DESCRIPTION,
  gl_HandleOptions,
  gl_Initialize
};

Renderer glRendererStereo = {
  GL_NAME_STEREO,
  GL_DESCRIPTION_STEREO,
  gl_HandleOptions,
  gl_InitializeStereo
};

Renderer glTextureRenderer = {
  GLTEXTURE_NAME,
  GLTEXTURE_DESCRIPTION,
  gltexture_HandleOptions,
  gltexture_Initialize
};


#ifdef USE_DMX
Renderer dmxRenderer = {
  DMX_NAME,
  DMX_DESCRIPTION,
  NULL,
  dmx_Initialize
};

#endif
