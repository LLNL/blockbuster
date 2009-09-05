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

    /*!
      ChooseFile: The returned string must be allocated with malloc(); it
      will be released with free().
    */
    char *(*ChooseFile)(const ProgramOptions *options);

  } ;

  /* UserInterface utilities from ui.c */
  extern MovieStatus MatchUserInterfaceToRenderer(
              QString userInterfaceName, QString rendererName, 
              UserInterface **matchedUserInterface, Renderer **matchedRenderer,
              int *matchedRendererIndex);

#endif
