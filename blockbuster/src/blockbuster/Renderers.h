#ifndef BLOCKBUSTER_RENDERERS_H
#define BLOCKBUSTER_RENDERERS_H
#include "canvas.h"

  /* A Renderer is one of the two static "parents" of the dynamic Canvas
   * object.  (The other is the UserInterface.)
   * Supposedly, the Renderer is the set of routines that understands how
   * to render an image (in a specific format, as indicated by the Renderer
   * itself) to an already-prepared rendering surface (usually either a
   * window or a widget).  The scary thing is that the Renderer object has 
   * no Render command.  :-( 
   *
   * The Renderer is allowed to parse options to change its behavior; and
   * is called to "plug" itself into a Canvas being prepared by a 
   * UserInterface object.
   */

struct Renderer; 

extern Renderer x11Renderer, glRenderer, glRendererStereo, glTextureRenderer, dmxRenderer; 

struct Renderer {
  char *name;
  char *description;
  
  void (*HandleOptions)(int &argc, char *argv[]);
  MovieStatus (*Initialize)(struct Canvas *canvas, const ProgramOptions *options);
} ;

/*
   * The UserInterface also maintains a list of Renderers that it supports,
   * with appropriate "glue" routines for each renderer; the "glue" routines
   * are plugged in based on the choice of renderer. 
*/
struct RendererSpecificGlue; 
struct RendererGlue {
  Renderer *renderer;  
  RendererSpecificGlue * configurationData; 
  
} ;

#endif
