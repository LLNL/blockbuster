/*
  #
  # $RCSfile: main.cpp,v $
  # $Name:  $
  # 
  # ASCI Visualization Project 
  #
  # Lawrence Livermore National Laboratory
  # Information Management and Graphics Group
  # P.O. Box 808, Mail Stop L-561
  # Livermore, CA 94551-0808
  #
  # For information about this project see:
  # 	http://www.llnl.gov/sccd/lc/img/ 
  #
  # 	or contact: asciviz@llnl.gov
  #
  # For copyright and disclaimer information see:
  #	$(ASCIVIS_ROOT)/copyright_notice_1.txt
  #
  # $Id: main.cpp,v 1.37 2009/05/22 02:48:17 wealthychef Exp $
  #
  # Abstract: 
  #  Contains the main function for blockbuster. 
  #
  #   Author: Rich Cook
  #
  # This work performed under the auspices of the U.S. Department of Energy by Lawrence Livermore National Laboratory under Contract DE-AC52-07NA27344.
  # This document was prepared as an account of work sponsored by an agency of the United States government. Neither the United States government nor Lawrence Livermore National Security, LLC, nor any of their employees makes any warranty, expressed or implied, or assumes any legal liability or responsibility for the accuracy, completeness, or usefulness of any information, apparatus, product, or process disclosed, or represents that its use would not infringe privately owned rights. Reference herein to any specific commercial product, process, or service by trade name, trademark, manufacturer, or otherwise does not necessarily constitute or imply its endorsement, recommendation, or favoring by the United States government or Lawrence Livermore National Security, LLC. The views and opinions of authors expressed herein do not necessarily state or reflect those of the United States government or Lawrence Livermore National Security, LLC, and shall not be used for advertising or product endorsement purposes.
  #
*/

#include "blockbuster_qt.h"
#include "common.h"

#ifdef USE_MPI
#include "mpi.h"
#endif

#include <signal.h>
#include "QMessageBox"
#include "errmsg.h"
#include <version.h>
#include "settings.h"
#include <X11/Xlib.h>
#include "ipc.h"
#include <iostream> 
#include "movie.h"
#include "ui.h" 
#include <sys/stat.h>
#include "dmxglue.h"
#include "frames.h"
#include "slave.h"
#include "util.h"
#include "SidecarServer.h"
#include "sm.h"

// global for Qt use: 
QApplication *gCoreApp=NULL; 
BlockbusterInterface *gMainWindow = NULL; 

QThread *gMainThread = NULL; 

/* This utility is used to get the number of processors
 * available on a system.  If the number of processors is
 * greater than 1, threads will be enabled by default.
 * If the number cannot be determined with any validity,
 * 0 will be returned.
 */
static int GetNumProcessors(void)
{
  FILE *f;
  int count = 0;
  char buffer[BLOCKBUSTER_PATH_MAX], *s;
  int rv;

  f = fopen("/proc/cpuinfo", "r");
  if (f == NULL) return 0;

  while ((s = fgets(buffer, BLOCKBUSTER_PATH_MAX, f)) != NULL) {
    if (strncmp(s, "processor", 9) == 0 && !isalpha(s[9])) {
      count++;
    }
  }

  rv = fclose(f);
  if (rv != 0) {
    SYSERROR("could not close /proc/cpuinfo (%d)", rv);
  }
    
  return count;
}


void version(void) {
  fprintf(stderr, "Blockbuster version "BLOCKBUSTER_VERSION"  (c) Tungsten Graphics, Inc. with modifications by IMG group at Lawrence Livermore National Laboratory (from movie.cpp rev. $Revision: 1.37 $)\n\n");
  return; 
}

void usage(void) {
  version(); 
  fprintf(stderr, "Usage: blockbuster [options] [-- -<renderer-specific options>] [file]\n");
  fprintf(stderr, "Play a movie from a file or series of files\n");
  fprintf(stderr, "'file' is an optional file name, or a prefix of a set of file names\n");
  fprintf(stderr, "\nOptions:\n");
  fprintf(stderr, "   Note: substrings are also matched, so -h is the same as -help, but beware of non-unique prefixes!\n");
  fprintf(stderr, "-cache <num> specifies how many frames to cache\n");
  fprintf(stderr, "-DecorationsDisable (or -no-decorations): same as -fullscreen\n");
  fprintf(stderr, "-display <display> specifies X display\n");
  fprintf(stderr, "-font <fontname> specifies X font\n");
  fprintf(stderr, "-framerate (or -Framerate) <rate> sets the initial frame rate target for the movie [30.0 or movie-specified]\n");
  fprintf(stderr, "-fullscreen: turns off window decorations and goes to full-screen display\n"); 
  fprintf(stderr, "-geometry <geometrystring> specifies X window geometry\n");
  fprintf(stderr, "-help displays this help message\n");
  fprintf(stderr, "-keyhelp:  display list of keyboard controls\n");
  fprintf(stderr, "-lod num: specifies a starting level of detail for the given movie\n");
  fprintf(stderr, "-loops <loops> specifies how many times to loop (number or 'forever')\n");
  fprintf(stderr, "-messageLevel sets the message level, in order of chattiness:  quiet, syserr, error, warning, info, debug\n"); 
  fprintf(stderr, "-no-controls (or -withoutControls) turns off the control window (if defined for the user interface)\n");
  fprintf(stderr, "-no-splash (or -S) suppresses display of splash screen\n");
  fprintf(stderr, "-play (or -Play) automatically starts the movie after loading\n");
  fprintf(stderr, "-preload <num> specifies how many frames to preload\n");
  fprintf(stderr, "-renderer <name> specifies the method used to render images\n");
  fprintf(stderr, "\tgl: Render using OpenGL glDrawPixels to an X11 window\n");
  fprintf(stderr, "\t    (supported in 'gtk', 'x11' user interfaces)\n");
  fprintf(stderr, "\tgltexture: Render using OpenGL texture mapping to an X11 window\n");
  fprintf(stderr, "\t    (supported in 'gtk', 'x11' user interfaces)\n");
  fprintf(stderr, "\tx11: Render using X11 rendering routines to an X11 window\n");
  fprintf(stderr, "\t    (supported in 'gtk', 'x11' user interfaces)\n");
  fprintf(stderr, "\tgl_stereo: Render using OpenGL glDrawPixels to an X11 window in stereo\n");
  fprintf(stderr, "\t    (supported in 'gtk', 'x11' user interfaces)\n");
  fprintf(stderr, "\tdmx: Render on back-end using DMX: Use -R to specify  backend renderer\n");
  fprintf(stderr, "\t    (supported in 'gtk', 'x11' user interfaces)\n");
  
  fprintf(stderr, "-stereo: short for -r gl_stereo, unless -dmx is given, in which case it is short for -R gl_stereo.\n");
  fprintf(stderr, "-dmxstereo: short for -r dmx -R gl_stereo.\n");
  fprintf(stderr, "-threads <num> specifies how many threads to use for reading from disk.\n");
  fprintf(stderr, "-userInterface <name> specifies the user interface to use:\n");
  fprintf(stderr, "\tgtk: Hybrid GTK/X11 user interface using a GTK toolbar and an X11 window\n");
  fprintf(stderr, "\t(supports gl, gltexture, x11, gl_stereo, dmx renderers)\n");
  fprintf(stderr, "\tx11: Simple X11 user interface using keypresses and mouse movements\n");
  fprintf(stderr, "\t(supports gl, gltexture, x11, gl_stereo, dmx renderers)\n");  
  fprintf(stderr, "-verbose num: sets verbosity, with 0=quiet, 1=system, 2=error, 3=warning, 4=info, 5=debug.  Same behavior as -messageLevel but with numbers, basically.\n");
  fprintf(stderr, "-version prints the current blockbuster version\n");
  fprintf(stderr, "-zoom <zoom> sets the initial zoom ['auto' tracks window size,\n");
  fprintf(stderr, "                          0 is 'use default'] ['auto']\n");
  
  fprintf(stderr, "\nSLAVE OPTIONS\n"); 
  fprintf(stderr, "-slave: Run blockbuster as a backend slave\n");
  fprintf(stderr, "-ports <\"ports\"> specifies, in a quoted space-delimited list as a single argument, the port that each slave will use to communicate back to the master\n");
  fprintf(stderr, "-backend-renderer <renderer> (or RendererOnBackend <renderer>) specifies the renderer for the backend slaves to use (e.g., gl)\n");
  fprintf(stderr, "-backend-path <renderer> specifies the path to the backend slaves (defaults to \"blockbuster\"\n");
  fprintf(stderr, "-mpi:  Instead of connecting to each backend node and running blockbuster, connect to one backend node and launch $BLOCKBUSTER_MPI_SCRIPT on that host with args given in the string $BLOCKBUSTER_MPI_SCRIPT_ARGS\n");  
  fprintf(stderr, "-mpiscript script argstring:  Same as -mpi, but instead of running $BLOCKBUSTER_MPI_SCRIPT, run \"script\" with args given in \"argstring\"\n"); 
  fprintf(stderr, "-speedTest: DEBUG ONLY: (consider using with -play option) when playing the movie, let the slaves run as fast as they can and stop responding. Does not by itself begin playing the movie.\n");
  
  fprintf(stderr, "The following file formats are supported:\n");
  fprintf(stderr, "    PNG: Single-frame image in a PNG file\n");
  fprintf(stderr, "    PNM: Single-frame image in a PNM (PBM/PGM/PPM) file\n");
  fprintf(stderr, "    TIFF: Single-frame image in a TIFF file\n");
  fprintf(stderr, "    SM: Multiple frames in an SM (Streaming Movie) file\n");
  fprintf(stderr, "    SGI RGB: Single-frame image in an SGI RGB file\n");

  return ;
} 

/*!
  =====================================================================  
  Convenience Functions for parsing arguments follow
  =====================================================================
*/
/*!
  Check to make sure there is another pointer in argv to assign or else print out usage and exit
*/ 
void checkarg(int argc, const char *argname) {
  if (argc < 2) {
	fprintf(stderr, "Error: option %s requires an argument.", argname); 
    exit(1); 
  }
  return;
}

bool  CHECK_STRING_ARG(const char *flag, int &argc, char *argv[], QString &str)	{

  char *found = strstr(flag, argv[1]); 
  if (found != flag) return false;

  checkarg(argc, flag); // exits on error-- bad but no time to fix
  ConsumeArg(argc, argv, 1); 

  str = argv[1];
  ConsumeArg(argc, argv, 1); 

  return true;
}

bool  CHECK_ATOF_ARG(const char *flag, int &argc, char *argv[], float &flt)	{
  if (strstr(flag, argv[1]) == flag) {
    ConsumeArg(argc, argv, 1); 
	checkarg(argc, flag);
	flt = atof(argv[1]);
    ConsumeArg(argc, argv, 1); 
	return true; 
  }
  return false; 
}

  
bool  CHECK_ATOI_ARG(const char *flag, int &argc, char *argv[], int &intgr)	{
  if (strstr(flag, argv[1]) == flag) {
    ConsumeArg(argc, argv, 1); 
	checkarg(argc, flag);
	intgr = atoi(argv[1]);
    ConsumeArg(argc, argv, 1); 
	return true; 
  }
  return false; 
}

bool  SET_BOOL_ARG(const char *flag, int &argc, char *argv[], int &barg, int val, bool consume=true)	{
  if (strstr(flag, argv[1]) == flag) {
	barg = val;
    if (consume) {
      ConsumeArg(argc, argv, 1); 
    }
	return true;
  }
  return false; 
}
  
/*
 * Parse argv[] options and set flags in <opt>.
 */
static void ParseOptions(int &argc, char *argv[], ProgramOptions *opt)
{
  int numProcessors;
  MovieStatus status;
  char *value;
  /* defaults */
  opt->executable = gCoreApp->applicationFilePath(); 
  value = GetSetting(opt->settings, "splashScreen");
  if (value != NULL && atoi(value) == 0) {
    opt->splashScreen = 0;
  }
 
  /* These depend on the number of processors available;
   * there's not a lot of sense in setting up threads
   * if they'll steal time from the main thread.
   */
  opt->preloadFrames = 4;
  numProcessors = GetNumProcessors();
  if (numProcessors > 1) {
    opt->readerThreads = 2;
  }

  /* Figure out which user interface and renderer we're
   * using right now (the defaults), so we can give
   * appropriate help messages.
   */

  int help = 0, doStereo = 0; 
  while (argc > 1) {
	QString zoomString; 
	QString geometryString; 
	if (CHECK_ATOI_ARG("-cache", argc, argv, opt->frameCacheSize)) continue; 
	else if (SET_BOOL_ARG("-no-decorations", argc, argv, opt->decorations, 0) || 
             SET_BOOL_ARG("-DecorationsDisable", argc, argv, opt->decorations, 0) || 
             SET_BOOL_ARG("-fullscreen", argc, argv, opt->decorations, 0)) {
	  opt->zoomFit=1;
	  continue;
	} 
	else if (CHECK_STRING_ARG("-display", argc, argv, opt->displayName)) {
      continue;
    }
	else if (CHECK_STRING_ARG("-font", argc, argv, opt->fontName)) continue;
    else if (CHECK_ATOF_ARG("-Framerate", argc, argv,  opt->frameRate) || 
             CHECK_ATOF_ARG("-framerate", argc, argv,  opt->frameRate)) continue; 
    else if (CHECK_STRING_ARG("-geometry", argc, argv, geometryString)) {
	  unsigned int w = DONT_CARE, h = DONT_CARE;
      int mask = XParseGeometry((const char*)geometryString.toStdString().c_str(),
                                &opt->geometry.x,
                                &opt->geometry.y,
                                &w, &h);
	  opt->geometry.width = w;
	  opt->geometry.height = h;
      if ((mask&XNegative) || (mask&YNegative))
          WARNING("Negative positions not yet supported.");
	}
	else if (SET_BOOL_ARG("-help", argc, argv, help, 1)) {
	  usage(); 
	  exit(MOVIE_HELP);	   	
	}
	else if (SET_BOOL_ARG("-keyhelp", argc, argv, help, 1)) {
      PrintKeyboardControls(); 
	  exit(MOVIE_HELP);	   	
	}
	else if (CHECK_ATOI_ARG("-lod", argc, argv,  opt->LOD)){
      if (opt->LOD > 0) {
        // user thinks LOD 1 is lowest level... 
        opt->LOD--; 
      }
    continue;
    }
	else if (CHECK_STRING_ARG("-loops", argc, argv,  opt->loopCountName)) continue;
	else if (CHECK_STRING_ARG("-messageLevel", argc, argv, opt->messageLevelName))  {
      if ((opt->messageLevel = FindMessageLevel(opt->messageLevelName)) == NULL) {
        QString errmsg("no such message level '%1'.  Use -h for help."); 
        ERROR(errmsg.arg( opt->messageLevelName));
        exit(MOVIE_BAD_FLAG);
      }
      dbprintf(0, QString("Setting messageLevel to \"%1\"\n").arg(opt->messageLevelName)); 
      
      maxMessageLevel = opt->messageLevel->messageLevel;
      set_verbose(maxMessageLevel); 
      //if (maxMessageLevel == 4) enable_dbprintf(); 
      continue;
    }
	else if (SET_BOOL_ARG("-withoutControls", argc, argv, opt->drawInterface, 0) || 
             SET_BOOL_ARG("-no-controls", argc, argv, opt->drawInterface, 0)) continue;
	else if (SET_BOOL_ARG("-no-splash", argc, argv,  opt->splashScreen, 0) || 
             SET_BOOL_ARG("-SplashDisable",  argc, argv, opt->splashScreen, 0)) continue; 
	else if (SET_BOOL_ARG("-Play", argc, argv, opt->play, 1)||
             SET_BOOL_ARG("-play", argc, argv, opt->play, 1)) continue; 
	else if (CHECK_ATOI_ARG("-preload", argc, argv,  opt->preloadFrames)) continue;
	else if (CHECK_STRING_ARG("-renderer", argc, argv, opt->rendererName)) continue;
	else if (CHECK_STRING_ARG("-sidecar", argc, argv, opt->sidecarHostPort)) {
      gSidecarServer->PromptForConnections(false); 
      continue;
    }
	else if (SET_BOOL_ARG("-speedTest", argc, argv,  opt->speedTest, 1)) {
      cerr << "speedTest detected, speedTest is " << opt->speedTest << endl; 
      continue;
    }
	else if (SET_BOOL_ARG("-dmxstereo", argc, argv, doStereo, 1)) {
      opt->rendererName = "dmx"; 
      continue;
    }
    else if (CHECK_ATOI_ARG("-threads", argc, argv,  opt->readerThreads)) continue; 
	else if (CHECK_STRING_ARG("-userInterface", argc, argv, opt->userInterfaceName)) continue;
	else if (CHECK_ATOI_ARG("-verbose", argc, argv,maxMessageLevel))  {
      opt->messageLevel = FindMessageLevel(maxMessageLevel);
      set_verbose(maxMessageLevel); 
      continue;
    }
	else if (SET_BOOL_ARG("-version", argc, argv, help, 1)) {
	  version(); 
	  exit (0); 
	}

	else if (CHECK_STRING_ARG("-zoom", argc, argv, zoomString)) {
	  if (zoomString != "auto" && zoomString != "0") {		
		opt->zoom = zoomString.toFloat();
		opt->zoomFit = 0;
	  }	else {
		opt->zoomFit = 1;
	  }
	}

    //=====================================================

	else if (CHECK_STRING_ARG("-slave", argc, argv, opt->masterHost)) {
      continue;
    }
    else if (CHECK_STRING_ARG("-ports", argc, argv, opt->slavePorts)) {
      cerr << "-ports is obsolete" << endl;
      // note MPI is now detected farther down -- search for "hostTokens[2]"  
      continue;
    }
    // SLAVE OPTIONS
	else if (CHECK_STRING_ARG("-backend-renderer", argc, argv,  opt->backendRendererName) || 
             CHECK_STRING_ARG("-RendererOnBackend", argc, argv,  opt->backendRendererName)) continue;
    else if (CHECK_STRING_ARG("-backend-path", argc, argv, opt->executable) || 
        CHECK_STRING_ARG("-BackendPath", argc, argv, opt->executable)) continue;
	else if (SET_BOOL_ARG("-mpi", argc, argv, opt->useMPI, 1)){
      cerr << "mpi flags given, useMPI is " << opt->useMPI << endl; 
      char *envp = getenv("BLOCKBUSTER_MPI_SCRIPT");
      if (envp) {
        opt->mpiScript = envp; 
      }
      envp = getenv("BLOCKBUSTER_MPI_SCRIPT_ARGS");
      if (envp) {
        opt->mpiScriptArgs = envp;
      }
      DEBUGMSG(QString("Using backend command: \"%1 %2\"").arg( opt->mpiScript).arg( opt->mpiScriptArgs)); 
      opt->slaveLaunchMethod = "mpi"; 
#ifndef USE_MPI
      WARNING("You are running the slaves in MPI mode, but the master is not compiled with MPI.  If the slaves are not compiled with MPI (using USE_MPI=1 when running make), then they will not work."); 
#endif
      continue;
    }
	else if (CHECK_STRING_ARG("-mpiscript", argc, argv,  opt->mpiScript)) {
      opt->mpiScriptArgs = argv[1];      
      ConsumeArg(argc, argv, 1); 
      opt->slaveLaunchMethod = "mpi"; 
#ifndef USE_MPI
      WARNING("You are running the slaves in MPI mode, but the master is not compiled with MPI.  If the slaves are not compiled with MPI (using USE_MPI=1 when running make), then they will not work."); 
#endif
      continue;
    }
	// an unparsed arg means just return the number of args parsed...
	else break; 
  }

  if (doStereo) {
    if (opt->rendererName == "dmx") {
      opt->backendRendererName = "gl_stereo";
    } else {
      opt->rendererName = "gl_stereo"; 
    }
  }

  if (opt->masterHost != "") {
	opt->slaveMode = 1;
    QStringList hostTokens=opt->masterHost.split(":"); 
    opt->masterHost = hostTokens[0]; 
    /*    if (opt->slavePorts != "") {      
          QStringList ports = opt->slavePorts.split(" ", QString::SkipEmptyParts); 
          opt->masterPort = ports[opt->mpiRank].toInt();     
          } else {
    */
    if (hostTokens.size() > 1) {
      opt->masterPort = hostTokens[1].toInt();
      if (hostTokens.size() == 3 && hostTokens[2] == "mpi") {
#ifdef USE_MPI      
        DEBUGMSG("Slave MPI mode... initializing MPI"); 
        opt->useMPI = true; 
        MPI_Init(&argc, &argv);       
        int commsize = 0;       
        MPI_Comm_size(MPI_COMM_WORLD, &commsize); 
        MPI_Comm_rank(MPI_COMM_WORLD, &opt->mpiRank); 
        MPI_Barrier(MPI_COMM_WORLD); 
        DEBUGMSG("OK, we're rank %d of %d", opt->mpiRank, commsize); 
#else
        ERROR("This program was not compiled with mpi support.  Please recompile with USE_MPI=1 or do not use the -ports command."); 
        exit(1); 
#endif
      }   
    }
    else {
      opt->masterPort = DEFAULT_MASTER_PORT;
    }
    // }
    DEBUGMSG(QString("SlaveHost: %1 Port: %2\n")
          .arg(opt->masterHost).arg(opt->masterPort));  
  }
  
  if (opt->preloadFrames >= opt->frameCacheSize - 1)
    opt->preloadFrames = opt->frameCacheSize - 2;

  if (opt->preloadFrames < 0)
    opt->preloadFrames = 0;

  /* We've read all the command line options, so everything is set.
   * Pull out some of the information we'll need later.
   */
  if (opt->loopCountName == "forever") {
    opt->loopCount = LOOP_FOREVER;
  }
  else {
    opt->loopCount = opt->loopCountName.toInt();
  }

  /* Here's a big trick.  Find a nice match between the specified UserInterface
   * (if any) and the specified Renderer (again, if any).  A NULL UserInterface
   * name or Renderer name matches the first available UserInterface and/or
   * first listed Renderer in a UserInterface.
   */
  status = MatchUserInterfaceToRenderer(opt->userInterfaceName,
                                        opt->rendererName,
                                        &opt->userInterface, NULL,
                                        &opt->rendererIndex);
  if (status == MovieFailure) {
    /* The reason why has already been listed; just give the help message */
    ERROR("Use -h for help.");
    exit(MOVIE_BAD_FLAG);
  }

  return  ;  /* last unparsed arg */
}

void printargs(char *description, char *args[], int argc) {
  QString arglist = QString("PRINTARGS: %1: ").arg(description); 
  int argnum=0; 
  while (argnum < argc) {
    arglist += QString(" %1").arg(args[argnum]); 
    ++argnum; 
  }
  DEBUGMSG(arglist); 
}

/*!
  ================================================
  Handle exit, by whatever means, whether signal or Quit.
*/
void ExitHandler(void) {
  DEBUGMSG("ExitHandler called.\n");
  dmx_AtExitCleanup();
  return; 
}
  
/*!
  ================================================
  Handle exit by signal
*/
void
InterruptHandler(int)
{
  DEBUGMSG("Interrupt signal caught.  Forceably terminating.\n"); 
  ExitHandler(); 
  exit(1);
}

int main(int argc, char *argv[])
{
   
  ProgramOptions opt;
  Renderer *renderer;
  char localSettingsFilename[BLOCKBUSTER_PATH_MAX];
  char homeSettingsFilename[BLOCKBUSTER_PATH_MAX];
  int homeSettingsFileExists = 0, localSettingsFileExists = 0;
  char *envHOME;
  struct stat statBuf;
  int rv;
  char ** args = argv; 
  gMainThread = QThread::currentThread(); 

 
  /*! 
	If we are running dmx, then kill the slaves before exiting the program
  */ 
  atexit(ExitHandler); 

  /* Grab the INT signal so we can clean up */
  signal(SIGINT, InterruptHandler);

  /* This hack should prevent Qt 4.2.3 from segfaulting: */
  putenv("QT_NO_GLIB=1"); 

  /* This is the master list of frames, and some stats about the list */
  FrameList *allFrames = NULL;
  
  /* We might be saving settings files.  Look for them
   * in the user's home directory, and in the current
   * directory.
   */
  opt.settings = (Settings*)CreateBlankSettings();
  envHOME = getenv("HOME");
  if (envHOME != NULL) {
    sprintf(homeSettingsFilename, "%s/.blockbusterrc", envHOME);
    rv = stat(homeSettingsFilename, &statBuf);
    if (rv == 0 && S_ISREG(statBuf.st_mode)) {
      homeSettingsFileExists = 1;
      ReadSettingsFromFile(opt.settings, homeSettingsFilename);
    }
  }
  sprintf(localSettingsFilename, "./.blockbusterrc");
  rv = stat(localSettingsFilename, &statBuf);
  if (rv == 0 && S_ISREG(statBuf.st_mode)) {
    localSettingsFileExists = 1;
    ReadSettingsFromFile(opt.settings, localSettingsFilename);
  }

  /* The UserInterface gets the next chance to parse options.  For
   * both the UserInterface and the Renderer, any options passed
   * change variables managed inside those modules, that affect
   * their behavior; they don't get to change the 
   * ProgramOptions structure.
   */
  //gCoreApp = new QCoreApplication(argc, args); 
  gCoreApp = new QApplication(argc, args); 

  gMainWindow = new BlockbusterInterface; 

  /* set up network communications */ 
  gSidecarServer = new SidecarServer;     

  /* Grab any options that apply to the whole program */
  ParseOptions(argc, args, &opt);
  printargs("After ParseOptions", args, argc); 

  /* initialize the slave portion if we are a slave */
  if (opt.slaveMode != 0) {
    if (!SlaveInitialize(&opt)) { // sets options->displayName
      printf("Initialization failed. Slave will now exit.\n"); 
      exit(1); 
    }
  } else {
    gMainWindow->show(); 
  }

  /* initialize the UI (GTK usually) */
  if (argc && opt.userInterface->HandleOptions != NULL) {
    /* put the DISPLAY into the environment for GTK */
    if (opt.displayName != "") {
      char buf[2048]; 
      sprintf(buf, QString("DISPLAY=%1").arg(opt.displayName).toStdString().c_str()); 
      putenv(buf);
    }
    opt.userInterface->HandleOptions(argc, args);
    printargs("After UI", args, argc); 
  }
  renderer = opt.userInterface->supportedRenderers[opt.rendererIndex]->renderer;
 
  if (argc && renderer->HandleOptions != NULL) {
    renderer->HandleOptions(argc, args);
    printargs("After renderer", args, argc); 
  }

  INFO("Using %s renderer", opt.userInterface->supportedRenderers[opt.rendererIndex]->renderer->name);
  INFO("Using %s interface", opt.userInterface->name);

  // set up a connection to sidecar if that's what launched us
  if (opt.sidecarHostPort != "") {
    gSidecarServer->connectToSidecar(opt.sidecarHostPort); 
  }
  /* Remaining args are movie filenames, load them (skip args[0]=progname) */
  
  /* initialize the smlibrary with the number of threads */
  smBase::init(opt.readerThreads); 

  /* count the remaining args and treat them as files */ 
  printargs("Before framelist", args, argc); 
  int count = 0; 
  while (count < argc && args[count]) count++;  
  if (count-1) {
    allFrames = new FrameList(count-1, &args[1]);
  }
  if (opt.sidecarHostPort != "" && allFrames == NULL) {
    ERROR("%s is not a valid movie file - nothing to display", args[1]);
    exit(1);
  } 

  /* If we still don't have any frames to display, and we need some
   * (i.e. we're not a slave), give the user interface one last
   * chance to supply us with some frames.
   */
  if (!opt.slaveMode && !allFrames && opt.userInterface->ChooseFile) {
    QStringList fileList; 
    /* Try to get a filename from the user interface */
    char *filename = opt.userInterface->ChooseFile(&opt);
    if (filename == NULL) {
      WARNING("No frames found - nothing to display");
      exit(MOVIE_OK);
    }
    fileList.append(filename);
    free(filename);

    allFrames = new FrameList; 
    allFrames->LoadFrames(fileList);
  }

  /* At this point, we should have a full list of frames.  If we don't,
   * we obviously cannot play anything.
   */
  if (!opt.slaveMode && allFrames == NULL) {
    ERROR("No frames found - nothing to display");
    exit(1);
  }

  /* Here we have a master frame list.  We can do all the nifty
   * things we want to now.  Start by creating a window and an
   * XImage, matching as closely as we can the height, width, and
   * depth discovered in our list of frames.
   */
  if (opt.slaveMode) {
    /* The slave doesn't need frames - it will get the list from the master */
    if (allFrames) {
      DEBUGMSG("Deleting frame list..."); 
      allFrames->DeleteFrames(); 
      delete allFrames;
      DEBUGMSG("Frame List deleted"); 
    } else {
      DEBUGMSG("No frames to delete"); 
    }
    SlaveLoop(&opt);
    INFO("Done with slave loop.\n"); 
  }
  else {
    /* The display loop will destroy the frames for us (it has to, because it
     * must be able to change the frames too.)
     */
    DisplayLoop(allFrames, &opt);
  }

  /* If we read settings, write them back out.  Only one of the
   * two files we read should contain the changed settings creaed
   * inside the program.  If the local file exists, it takes all
   * the changed settings; otherwise, the home file gets them.
   */
  {
    int settingsSources = SETTINGS_FROM_PROGRAM | SETTINGS_FROM_FILE;

    if (localSettingsFileExists) {
      WriteSettingsToFile(opt.settings, localSettingsFilename,
                                                  settingsSources);
      settingsSources &= ~SETTINGS_FROM_PROGRAM;
    }

    /* We'll always attempt to write to the RC file in the
     * home directory.  If the file doesn't exist, and no
     * settings have to be rewritten, the file won't
     * be created; but if anything is missing, we'll
     * store it here.
     */
    WriteSettingsToFile(opt.settings, homeSettingsFilename,
                                                settingsSources);
  }
  DestroySettings(opt.settings);
  opt.settings = NULL;
  INFO("Blockbuster is finished.\n"); 
  /* All done.  Go home. */
  return 0;
}
