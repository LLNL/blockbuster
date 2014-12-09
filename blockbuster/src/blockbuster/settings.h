#ifndef BLOCKBUSTER_SETTINGS_H
#define BLOCKBUSTER_SETTINGS_H
#include <QApplication>
#include "common.h"
#include <string>
#include <vector>
#include <fstream>
class Renderer; 
struct RendererSpecificGlue; 

struct ProgramOptions *GetGlobalOptions(void); 

void ConsumeArg(int &argc, char *argv[], int position); 


// =====================================================================
struct Setting {
  std::string variable, origin, value; 
  /* char *variable;
  char *origin;
  char *value;
  */
  int changed;
  struct Setting *next;
} ;

// =====================================================================
struct Settings {
  Setting *head;
  Setting *tail;
} ;
  /* This defines the number of recently read files that will be stored */
#define NUM_RECENT_FILES 4

#define DEFAULT_X_FONT "fixed"

// =====================================================================
  /* This structure is used to pass around the options that control
   * the workings of the entire program.
   */
struct ProgramOptions {
  ProgramOptions():
    mBackgroundColor(3), 
#ifdef DEBUG
    messageLevelName("debug"),
#else
    messageLevelName("error"), 
#endif
    messageLevel(NULL),  mCacheDebug(false),     
    mTraceEvents(1),mTraceEventsFile(NULL), 
    //mRenderer(NULL),
    readerThreads(-1), repeatCountName("0"),
    startFrame(0), currentFrame(0), endFrame(-1), repeatCount(0), LOD(0),
    slaveLaunchMethod("rsh"), useMPI(0), 
    play(0), playExit(0),  speedTest(0), frameRate(0.0), 
    fpsSampleFrequency(2.0), 
    zoom(0.0), zoomToFit(1), fullScreen(0), noSmallWindows(0), 
    slaveMode(0), masterPort(0), 
    preloadFrames(40), mMaxCachedImages(100), mDMXStereo(0), noAutoRes(0), 
    drawInterface(1), splashScreen(0), noscreensaver(0), 
    gui(1), decorations(1), 
    suggestedTitle("blockbuster"), fontName(DEFAULT_X_FONT), 
    stereoSwitchDisable(0), settings(NULL), allowIdleSlaves(1) {
    geometry.x = DONT_CARE;
    geometry.y = DONT_CARE;
    geometry.width = DONT_CARE;
    geometry.height = DONT_CARE;
    return;
  }
  std::vector<float> mBackgroundColor; /* "background" movie color, passed to glClearColor() in glRenderer.cpp */ 
  QString executable; /* path to the backend blockbuster */ 
  QString messageLevelName;
  struct MessageLevel *messageLevel;
  int mCacheDebug; 
  QString mReplayEventsFilename;
  int mTraceEvents;
  FILE *mTraceEventsFile; 
  std::string mTraceEventsFilename; 
  QString logFile;  
  QString mScript; 
  int rendererIndex;
  QString rendererName;
  //  Renderer *mRenderer; 
  QString backendRendererName;
  int readerThreads;
  QString repeatCountName;
  int32_t startFrame, currentFrame, endFrame; 
  int repeatCount;
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
  float fpsSampleFrequency; // time between fps calculations.
  float zoom;
  int zoomToFit, fullScreen;
  int noSmallWindows; // prevent windows being resized below full movie size -- this is used to work around Window manager bug that resizes to one monitor size when launched on powerwalls.  
  int slaveMode;
  int masterPort;
  int preloadFrames;
  int mMaxCachedImages;
  int mDMXStereo; 
  int noAutoRes; 
  int drawInterface;
  int splashScreen;
  int noscreensaver;
  int gui;/* if a GUI is available */
  Rectangle geometry;
  int decorations; /* window borders, titlebar, etc? */
  QString displayName;
  QString suggestedTitle;
  QString fontName;
  int stereoSwitchDisable;
  Settings *settings; 
  int allowIdleSlaves; 
} ;

// =====================================================================
#define SETTINGS_FROM_PROGRAM   0x01
#define SETTINGS_FROM_FILE    0x02
#define SETTINGS_FROM_ALL_FILES   0x04
#define SETTINGS_CHANGED    0x08
void *CreateBlankSettings(void);
void DestroySettings(void *settings);
void ReadSettingsFromFile(void *settings, const char *filename);
void WriteSettingsToFile(void *settings, std::string filename, unsigned int flags);
void ChangeSetting(void *settings, std::string variable,std::string value);
std::string GetSetting(void *settings, std::string variable);
void SetRecentFileSetting(void *settings,  const char *filename);

#endif
