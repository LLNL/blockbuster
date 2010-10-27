#ifndef BLOCKBUSTER_SETTINGS_H
#define BLOCKBUSTER_SETTINGS_H
#include <QApplication>
#include "common.h"
class Renderer; 
struct RendererSpecificGlue; 

class ProgramOptions *GetGlobalOptions(void); 

void ConsumeArg(int &argc, char *argv[], int position); 

struct Setting {
  char *variable;
  char *origin;
  char *value;
  int changed;
  struct Setting *next;
} ;

struct Settings {
  Setting *head;
  Setting *tail;
} ;
  /* This defines the number of recently read files that will be stored */
#define NUM_RECENT_FILES 4

#define DEFAULT_X_FONT "fixed"

  /* This structure is used to pass around the options that control
   * the workings of the entire program.
   */
struct ProgramOptions {
  ProgramOptions():
#ifdef DEBUG
    messageLevelName("debug"),
#else
    messageLevelName("error"), 
#endif
    messageLevel(NULL),  mRenderer(NULL),
    frameCacheSize(8), readerThreads(-1), loopCountName("1"), 
    startFrame(0), endFrame(-1), loopCount(1), LOD(0),
    slaveLaunchMethod("rsh"), useMPI(0), 
    play(0), playExit(0),  speedTest(0), frameRate(0.0), 
    zoom(1.0), zoomFit(1), fullScreen(0), slaveMode(0), masterPort(0), 
    preloadFrames(4), noAutoRes(0), 
    drawInterface(1), splashScreen(0), gui(1), decorations(1), 
    suggestedTitle("blockbuster"), fontName(DEFAULT_X_FONT), 
    settings(NULL), allowIdleSlaves(1) {
    geometry.x = DONT_CARE;
    geometry.y = DONT_CARE;
    geometry.width = DONT_CARE;
    geometry.height = DONT_CARE;
    return;
  }
    
  QString executable; /* path to the backend blockbuster */ 
  QString messageLevelName;
  struct MessageLevel *messageLevel;
  QString logFile;  
  int rendererIndex;
  QString rendererName;
  RendererSpecificGlue *mRendererSpecificGlue; 
  Renderer *mRenderer; 
  QString backendRendererName;
  int frameCacheSize;
  int readerThreads;
  QString loopCountName;
  int32_t startFrame, endFrame; 
  int loopCount;
  int LOD; 
  QString slaveLaunchMethod; 
  int useMPI; 
  QString mpiScript; 
  QString mpiScriptArgs; 
  int mpiRank; // 0-based rank in MPI_COMM_WORLD
  QString masterHost;
  QString slavePorts; /* this is used by the slaves to find the server and is gotten from the command line options */ 
  QString sidecarHostPort; // used when launched with -sidecar
  int play, playExit;
  int speedTest; 
  float frameRate; 
  float zoom;
  int zoomFit, fullScreen;
  int slaveMode;
  int masterPort;
  int preloadFrames;
  int noAutoRes; 
  int drawInterface;
  int splashScreen;
  int gui;/* if a GUI is available */
  Rectangle geometry;
  int decorations; /* window borders, titlebar, etc? */
  QString displayName;
  QString suggestedTitle;
  QString fontName;
  int stereoView;
  Settings *settings; 
  int allowIdleSlaves; 
} ;

#define SETTINGS_FROM_PROGRAM   0x01
#define SETTINGS_FROM_FILE    0x02
#define SETTINGS_FROM_ALL_FILES   0x04
#define SETTINGS_CHANGED    0x08
void *CreateBlankSettings(void);
void DestroySettings(void *settings);
void ReadSettingsFromFile(void *settings, const char *filename);
void WriteSettingsToFile(void *settings, const char *filename, unsigned int flags);
void ChangeSetting(void *settings, const char *variable, const char *value);
 char *GetSetting(void *settings, const char *variable);
void SetRecentFileSetting(void *settings,  const char *filename);

#endif
