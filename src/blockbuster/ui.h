#ifndef BLOCKBUSTER_UI_H
#define BLOCKBUSTER_UI_H
#include "Renderers.h"

   struct UserInterface {
    char *name;
    char *description;
    RendererGlue **supportedRenderers;

    void (*HandleOptions)(int &argc, char *argv[]);
    MovieStatus (*Initialize)(struct Canvas *canvas, const ProgramOptions *options,
            qint32 uiData, const RendererSpecificGlue *configurationData);

    /* This routine is only called if the main program is called
     * without any movie arguments.  If unimplemented or if it
     * returns NULL, the main program will exit.  Otherwise,
     * it will continue as though it had been passed the
     * returned string.
     *
     * The returned string must be allocated with malloc(); it
     * will be released with free().
     */
    char *(*ChooseFile)(const ProgramOptions *options);

  } ;

  /* UserInterface utilities from ui.c */
  extern MovieStatus MatchUserInterfaceToRenderer(
              QString userInterfaceName, QString rendererName, 
              UserInterface **matchedUserInterface, Renderer **matchedRenderer,
              int *matchedRendererIndex);

#endif
