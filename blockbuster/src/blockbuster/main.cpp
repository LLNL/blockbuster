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

#include "common.h"
#include "blockbuster_qt.h"
#include "events.h"

#ifdef USE_MPI
#include "mpi.h"
#endif

#include <signal.h>
#include "QMessageBox"
#include <QFileDialog>
#include "errmsg.h"
#include <version.h>
#include "settings.h"
#include <X11/Xlib.h>
#include <iostream> 
#include "movie.h"
#include <sys/stat.h>
#include "frames.h"
#include "slave.h"
#include "util.h"
#include "SidecarServer.h"
#include "smFrame.h"
#include "Prefs.h"
#include <boost/iostreams/device/file_descriptor.hpp>
// global for Qt use: 
QApplication *gCoreApp=NULL; 
BlockbusterInterface *gMainWindow = NULL; 

ProgramOptions gProgramOptions;

// =====================================================================
/* This utility is used to get the number of processors
 * available on a system.  If the number of processors is
 * greater than 1, threads will be enabled by default.
 * If the number cannot be determined with any validity,
 * 0 will be returned.  Only works on Linux. 
 */
static int GetNumProcessors(void)
{
  // works on Linux and OS X: 
  return  sysconf( _SC_NPROCESSORS_ONLN );

}


// =====================================================================
void version(void) {
  printf( "Blockbuster version "BLOCKBUSTER_VERSION"  (c) Tungsten Graphics, Inc. with modifications by IMG group at Lawrence Livermore National Laboratory\n\n");
  return; 
}

// =====================================================================
void usage(void) {
  version(); 
  printf("Usage: blockbuster [options] [-- -<renderer-specific options>] [file]\n");
  printf("Play a movie from a file or series of files\n");
  printf("'file' is an optional file name, or a prefix of a set of file names\n");
  printf("\nOptions:\n");
  printf("   Note: substrings are also matched, so -h is the same as -help, but beware of non-unique prefixes!\n");
  printf("--cachedebug: turn on verbose cache debugging messages (also enabled if verbosity is > 5)\n"); 
  printf("--cachesize/-c <num> specifies cache limit in frames (you can leave this unset unless you are running out of memory)\n");
  printf("--DecorationsDisable/-D (or --no-decorations): do not use window manager decorations such as titlebar etc.\n");
  printf("--display/-d <display> specifies X display\n");
  printf("--dmxstereo: short for -r dmx --stereo.\n");
  printf("--font <fontname> specifies X font\n");
  printf("--Framerate/-F <rate> sets the initial frame rate target for the movie [30.0 or movie-specified]\n");
  printf("--fpsSampleFrequency <rate> sets the number of times per second that FPS is calculated [2.0]\n");
  printf("--fullscreen/-f: turns off window decorations and goes to full-screen display\n"); 
  printf("--geom(etry)/-g <geometrystring> specifies X window geometry\n");
  printf("--help/-h displays this help message\n");
  printf("--keyhelp:  display list of keyboard controls\n");
  printf("--LOD/-L num: specifies a starting level of detail for the given movie\n");  
  printf("--log/-l filename: Log all messages to the given file for debugging.\n");
  printf("--loops <n> See -repeat below.\n");
  printf("--messageLevel sets the message level, in order of chattiness:  quiet, syserr, error, warning, info, debug\n"); 
  printf("--no-autores:  normally, you want blockbuster to decrease resolution when the zoom increases.  This flag suppresses this, for testing.\n");
  printf("--no-controls/--withoutControls/-w turns off the control window (if defined for the user interface)\n");
  printf("--no-screensaver: use fake mouse clicks to defeat screensaver\n");
  printf("--no-small-windows: disallow resizing of window less than movie size.  Workaround for window manager bug on powerwall.n"); 
  printf("--no-stereo-switch: suppresses automatic stereo or mono mode switches based on movie being stereo or mono\n");
  printf("--play/-p automatically starts the movie after loading\n");
  printf("--playexit framenum: play one time until frame given, then exit.  Useful for testing.  If framenum == -1, play all the way to end.\n");
  printf("--preload <num> specifies how many frames to preload\n");
  printf("--renderer/-r <name> specifies the method used to render images\n");
  printf("\tgl: Render using OpenGL glDrawPixels to an X11 window\n");
  printf("\t    (supported in 'gtk', 'x11' user interfaces)\n");
  printf("\tgltexture: Render using OpenGL texture mapping to an X11 window\n");
  printf("\t    (supported in 'gtk', 'x11' user interfaces)\n");
  printf("\tx11: Render using X11 rendering routines to an X11 window\n");
  printf("\t    (supported in 'gtk', 'x11' user interfaces)\n");
  printf("\tgl_stereo: Render using OpenGL glDrawPixels to an X11 window in stereo\n");
  printf("\t    (supported in 'gtk', 'x11' user interfaces)\n");
  printf("\tdmx: Render on back-end using DMX: Use -R to specify  backend renderer\n");
  printf("\t    (supported in 'gtk', 'x11' user interfaces)\n");
  
  printf("--repeat <n> specifies how many times to repeat (number or 'forever').  So a value of 1 means the movie will play twice and then stop.\n");
  printf("--replayEvents filename: replay all events from the given event log.\n"); 
  printf("--stereo: short for -r gl_stereo, unless -dmx is given, in which case it is short for -R gl_stereo.\n");
  printf("--script filename: run the given blockbuster script\n"); 
  printf("--threads/-t <num> specifies how many threads to use for reading from disk.\n");
  printf("--timer: enable timer (makes things very verbose, you probably do not want this)\n"); 
  printf("--no-trace: Disables logging of events to a \"trace file\" which can be replayed later using --script.  See also --trace-file.\n"); 
  printf("--trace-file: Secify the trace file to write to, which can be replayed later using --script.  See also --no-trace.\n"); 
  printf("--verbose/-v num: sets verbosity, with 0=quiet, 1=system, 2=error, 3=warning, 4=info, 5=debug.  Same behavior as -messageLevel but with numbers, basically.\n");
  printf("--version/-V prints the current blockbuster version\n");
  printf("--zoom <zoom> sets the initial zoom ['auto' tracks window size,\n");
  printf("                          0 is 'use default'] ['auto']\n");
  
  printf("\nSLAVE OPTIONS\n"); 
  printf("--backend-renderer <renderer> specifies the renderer for the backend slaves to use (e.g., gl)\n");
  printf("--backend-path <renderer> specifies the path to the backend slaves (defaults to \"blockbuster\"\n");
  printf("--mpi:  Instead of connecting to each backend node and running blockbuster, connect to one backend node and launch $BLOCKBUSTER_MPI_SCRIPT on that host with args given in the string $BLOCKBUSTER_MPI_SCRIPT_ARGS\n");  
  printf("--mpiscript script argstring:  Same as -mpi, but instead of running $BLOCKBUSTER_MPI_SCRIPT, run \"script\" with args given in \"argstring\"\n"); 
  printf("--ports <\"ports\"> specifies, in a quoted space-delimited list as a single argument, the port that each slave will use to communicate back to the master\n");
  /* DO NOT EXPOSE THIS TO USER: 
  printf("--slave: Run blockbuster as a backend slave\n");
  */ 
  printf("The following file formats are supported:\n");
  printf("    PNG: Single-frame image in a PNG file\n");
  printf("    PNM: Single-frame image in a PNM (PBM/PGM/PPM) file\n");
  printf("    TIFF: Single-frame image in a TIFF file\n");
  printf("    SM: Multiple frames in an SM (Streaming Movie) file\n");
  printf("    SGI RGB: Single-frame image in an SGI RGB file\n");

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


// =====================================================================
// OK for string and Qstring arg types
template <class T> 
bool  CHECK_STRING_ARG(const char *flag, int &argc, char *argv[], T &t)	{

  const char *found = strstr(flag, argv[1]); 
  if (found != flag) return false;

  ConsumeArg(argc, argv, 1); 
  checkarg(argc, flag); // exits on error-- bad but no time to fix
  DEBUGMSG("Setting string arg to %s\n", argv[1]); 

  t = argv[1];
  ConsumeArg(argc, argv, 1); 

  return true;
}


// =====================================================================
bool  CHECK_ATOF_ARG(const char *flag, int &argc, char *argv[], float &flt)	{
  if (strstr(flag, argv[1]) == flag) {
    DEBUGMSG("Setting float arg\n"); 
    ConsumeArg(argc, argv, 1); 
	checkarg(argc, flag);
	flt = atof(argv[1]);
    ConsumeArg(argc, argv, 1); 
	return true; 
  }
  return false; 
}

  
// =====================================================================
bool  CHECK_ATOI_ARG(const char *flag, int &argc, char *argv[], int &intgr)	{
  if (strstr(flag, argv[1]) == flag) {
    DEBUGMSG("Setting Int arg\n"); 
    ConsumeArg(argc, argv, 1); 
	checkarg(argc, flag);
	intgr = atoi(argv[1]);
    ConsumeArg(argc, argv, 1); 
	return true; 
  }
  return false; 
}

// =====================================================================
bool  SET_BOOL_ARG(const char *flag, int &argc, char *argv[], int &barg, int setval, bool consume=true)	{
  if (strstr(flag, argv[1]) == flag) {
    DEBUGMSG("Setting bool arg\n"); 
	barg = setval;
    if (consume) {
      ConsumeArg(argc, argv, 1); 
    }
	return true;
  }
  return false; 
}
  
// =====================================================================
static char ** DuplicateArgs(int argc, char *argv[]) {
  char **argv_new = (char **)malloc(sizeof(char *) * (argc+1)); 
  int argnum = 0; 
  while (argnum < argc) {
    argv_new[argnum] = strdup(argv[argnum]); 
    argnum++; 
  }
  argv_new[argnum] = NULL; 
  return argv_new;
}

// =====================================================================
/*!
  Look for environment variables.  
  This is a start at a systematic approach to this.  
  Not all environment variables are set here.  Far from it. 
*/ 
static void ParseEnvironmentVars(void) {
  ProgramOptions *opt = GetGlobalOptions(); 
  char *noscreensavers = getenv("BLOCKBUSTER_NOSCREENSAVER"); 
  if (noscreensavers) {
    string noscreensaverstr(noscreensavers); 
    opt->noscreensaver = (noscreensaverstr != "0" && noscreensaverstr != "false");    
  }
  return; 
}

// =====================================================================
/* 
 * Parse argv[] options and set flags in <opt>.
 */
static void ParseOptions(ProgramOptions *opt, int &argc, char *argv[])
{
  int numProcessors;
  /* defaults */
    string value = GetSetting(opt->settings, "splashScreen");
  if (atoi(value.c_str()) == 0) {
    opt->splashScreen = 0;
  }
  
  /* These depend on the number of processors available;
   * there's not a lot of sense in setting up threads
   * if they'll steal time from the main thread.
   */
  //opt->preloadFrames = 100; // ignored now. 

  /* Figure out which user interface and renderer we're
   * using right now (the defaults), so we can give
   * appropriate help messages.
   */

  int dummy = 0; 
  string dummyString; 
  while (argc > 1) {
    DEBUGMSG("Checking arg %s\n", argv[1]); 
	QString zoomString; 
	QString geometryString; 
	if (SET_BOOL_ARG("--cachedebug", argc, argv, opt->mCacheDebug, 1)) {
      continue; 
    }
    else if (CHECK_STRING_ARG("--bgcolor", argc, argv, dummyString)) {
      stringstream ss(dummyString);       
      try { 
        if (dummyString[0] == '#') { 
          if (dummyString.size() != 7) {
             throw  "Bad value"; 
          }            
           
          //handle hex code 
          int num; 
          char pound; 
          if (!(ss >> pound >> hex >> num)) {
            cerr <<  str(boost::format("Bad hex value found in color string: \"%s\"\n")%dummyString); 
            throw  "Bad value"; 
          }
          opt->mBackgroundColor[0] = (num/0x10000)/255.0; 
          opt->mBackgroundColor[1] = (num/0x100)/255.0; 
          opt->mBackgroundColor[2] = (num%0x100)/255.0; 
        }
        else {
          // Either rgb as 3 floats, or 3 255-based integers
          string num; 
          for (int i = 0; i<3; i++) {
            opt->mBackgroundColor[i] = 0; 
            if (!(ss >> opt->mBackgroundColor[i])) {
              throw  "Bad value"; 
            }
            
            if (opt->mBackgroundColor[i] > 1) {
              opt->mBackgroundColor[i] /= 255.0; 
            }
            if (opt->mBackgroundColor[i] < 0 || opt->mBackgroundColor[i] > 1.0) {
              cerr << "Value " << i << " of \"" << dummyString << "\" is out of range" << endl; 
              throw  "Bad value"; 
            }
          }
        }
        string junk; 
        if (ss>>junk) {
          cerr <<  str(boost::format("Extra junk \"%s\" found at end of color string \"%s\"\n")%junk%dummyString); 
          throw  "Bad value"; 
        }                    
      } catch (...) {
        cerr << "Bad argument for bgcolor: " << dummyString << endl; 
        exit(1); 
      }
    } 
	else if (CHECK_ATOI_ARG("--cachesize", argc, argv,  opt->mMaxCachedImages) ||
        CHECK_ATOI_ARG("-c", argc, argv,  opt->mMaxCachedImages) ) {
      continue;
    }
	else if (SET_BOOL_ARG("--no-decorations", argc, argv, opt->decorations, 0) ||
             SET_BOOL_ARG("-D",argc,argv, opt->decorations,0) ||
             SET_BOOL_ARG("--DecorationsDisable",argc,argv, opt->decorations,0)) {
      continue; 
    }
	else if (CHECK_STRING_ARG("--display", argc, argv, opt->displayName) || 
             CHECK_STRING_ARG("-d", argc, argv, opt->displayName) ) {
      continue;
    }
	else if (SET_BOOL_ARG("--dmxstereo", argc, argv, opt->mDMXStereo, 1)) {
      opt->rendererName = "dmx"; 
      continue;
    }
	else if (CHECK_STRING_ARG("--font", argc, argv, opt->fontName))  {
      continue;
    }
    else if (CHECK_ATOF_ARG("--Framerate", argc, argv,  opt->frameRate) || 
             CHECK_ATOF_ARG("-F", argc, argv,  opt->frameRate))   {
      continue; 
    }
    else if (CHECK_ATOF_ARG("--fpsSampleFrequency", argc, argv,  opt->fpsSampleFrequency))  {
      continue; 
    }
    else if (SET_BOOL_ARG("--fullscreen", argc, argv, opt->fullScreen, 1) ||
             SET_BOOL_ARG("-f", argc, argv, opt->fullScreen, 1)) {
      opt->zoomToFit=1;
      opt->decorations = 0; 
	  continue;
	} 
    else if (CHECK_STRING_ARG("--geometry", argc, argv, geometryString) ||
             CHECK_STRING_ARG("--geom", argc, argv, geometryString) ||
             CHECK_STRING_ARG("-g", argc, argv, geometryString)) {
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
	else if (SET_BOOL_ARG("--help", argc, argv, dummy, 1) ||
             SET_BOOL_ARG("-h", argc, argv, dummy, 1)) {
	  usage(); 
	  exit(MOVIE_HELP);	   	
	}
	else if (SET_BOOL_ARG("--keyhelp", argc, argv, dummy, 1)) {
      PrintKeyboardControls(); 
	  exit(MOVIE_HELP);	   	
	}
	else if (CHECK_ATOI_ARG("--LOD", argc, argv, opt->LOD) ||
             CHECK_ATOI_ARG("-L", argc, argv, opt->LOD)){
      if (opt->LOD > 0) {
        // user thinks LOD 1 is lowest level... 
        opt->LOD--; 
      }
      continue;
    }
	else if (CHECK_STRING_ARG("--log", argc, argv, opt->logFile) ||
             CHECK_STRING_ARG("-l", argc, argv, opt->logFile))  {
      enableLogging(true, opt->logFile); 
      continue;
    }
    else if (CHECK_STRING_ARG("--loops", argc, argv,  opt->repeatCountName)) {
      cerr << "--loops is no longer a valid option.  Please use --repeat." << endl; 
      exit(1); 
    }
	else if (CHECK_STRING_ARG("--messageLevel", argc, argv, opt->messageLevelName))  {
      if ((opt->messageLevel = FindMessageLevel(opt->messageLevelName)) == NULL) {
        QString errmsg("no such message level '%1'.  Use -h for help."); 
        ERROR(errmsg.arg( opt->messageLevelName));
        exit(MOVIE_BAD_FLAG);
      }
      dbprintf(0, QString("Setting messageLevel to \"%1\"\n").arg(opt->messageLevelName)); 
      
      //maxMessageLevel = opt->messageLevell->messageLevel;
      set_verbose(opt->messageLevel->messageLevel); 
      sm_setVerbose(opt->messageLevel->messageLevel); 
      //if (maxMessageLevel == 4) enable_dbprintf(); 
      continue;
    }
	else if (SET_BOOL_ARG("--no-autores", argc, argv, opt->noAutoRes, 1)) {
      continue; 
    }
	else if (SET_BOOL_ARG("--withoutControls", argc, argv, opt->drawInterface, 0)|| 
             SET_BOOL_ARG("--no-controls", argc, argv, opt->drawInterface, 0)  || 
             SET_BOOL_ARG("-w", argc, argv, opt->drawInterface, 0)) {
      continue;
    }
	else if (SET_BOOL_ARG("--no-screensaver", argc, argv, opt->noscreensaver, 1)) {
      continue;
	}
	else if (SET_BOOL_ARG("--no-small-windows", argc, argv,  opt->noSmallWindows, 1)) {
      continue; 
    }
	else if (SET_BOOL_ARG("--no-stereo-switch", argc, argv,  opt->stereoSwitchDisable, 1)) 
      continue; 
	else if (SET_BOOL_ARG("--play", argc, argv, opt->play, 1)||
             SET_BOOL_ARG("-p", argc, argv, opt->play, 1)){
      continue; 
    }
	else if (CHECK_ATOI_ARG("--playexit", argc, argv,  opt->playExit)) {
      opt->play = 1; 
      continue;
    }
    else if (CHECK_ATOI_ARG("--preload", argc, argv,  opt->preloadFrames)) {
      continue;
    }
     
	else if (CHECK_STRING_ARG("--renderer", argc, argv, opt->rendererName)) continue;
	else if (CHECK_STRING_ARG("--repeat", argc, argv,  opt->repeatCountName)) {
      continue ;   
    }
	else if (CHECK_STRING_ARG("--replayEvents", argc, argv, opt->mReplayEventsFilename))  {
      continue;
    }
	else if (CHECK_STRING_ARG("--sidecar", argc, argv, opt->sidecarHostPort)) {
      gSidecarServer->PromptForConnections(false); 
      continue;
    }
	else if (CHECK_STRING_ARG("--script", argc, argv, opt->mScript)) 
      continue;
	else if (SET_BOOL_ARG("--speedTest", argc, argv,  opt->speedTest, 1)) {
      cerr << "speedTest detected, speedTest is " << opt->speedTest << endl; 
      continue;
    }
    else if (CHECK_ATOI_ARG("--threads", argc, argv, opt->readerThreads) ||
             CHECK_ATOI_ARG("-t", argc, argv, opt->readerThreads)) 
      continue; 
	else if (SET_BOOL_ARG("--timer", argc, argv, gTimerOn, 1)) continue;
    else if (SET_BOOL_ARG("--no-trace", argc, argv, opt->mTraceEvents, 0)) {
	  continue; 
    }
	else if (CHECK_STRING_ARG("--trace-file", argc, argv, opt->mTraceEventsFilename))  {
      opt->mTraceEvents = true; 
      continue;
    }
	else if (CHECK_ATOI_ARG("--verbose", argc, argv,maxMessageLevel) ||
             CHECK_ATOI_ARG("-v", argc, argv,maxMessageLevel))  {
      opt->messageLevel = FindMessageLevel(maxMessageLevel);
      set_verbose(maxMessageLevel); 
      sm_setVerbose(maxMessageLevel); 
      continue;
    }
	else if (SET_BOOL_ARG("--version", argc, argv, dummy, 1)||
             SET_BOOL_ARG("-V", argc, argv, dummy, 1)) {
	  version(); 
	  exit (0); 
	}

	else if (CHECK_STRING_ARG("--zoom", argc, argv, zoomString)) {
	  if (zoomString == "auto" || zoomString == "0" || zoomString == "fit" || zoomString == "fill") {
		opt->zoomToFit = 1;
	  }
      else {		
		opt->zoom = zoomString.toFloat();
		opt->zoomToFit = 0;
	  }	
    }
    //=====================================================

	else if (CHECK_STRING_ARG("--slave", argc, argv, opt->masterHost)) {
      continue;
    }
    // SLAVE OPTIONS
	else if (CHECK_STRING_ARG("--backend-renderer", argc, argv,  opt->backendRendererName)) {
      continue;
    }
    else if (CHECK_STRING_ARG("--backend-path", argc, argv, opt->executable)) {
      continue;
    }
	else if (SET_BOOL_ARG("--mpi", argc, argv, opt->useMPI, 1)){
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
	else if (CHECK_STRING_ARG("--mpiscript", argc, argv,  opt->mpiScript)) {
      opt->mpiScriptArgs = argv[1];      
      ConsumeArg(argc, argv, 1); 
      opt->slaveLaunchMethod = "mpi"; 
#ifndef USE_MPI
      WARNING("You are running the slaves in MPI mode, but the master is not compiled with MPI.  If the slaves are not compiled with MPI (using USE_MPI=1 when running make), then they will not work."); 
#endif
      continue;
    }
    else if (CHECK_STRING_ARG("--ports", argc, argv, opt->slavePorts)) {
      cerr << "-ports is obsolete" << endl;
      // note MPI is now detected farther down -- search for "hostTokens[2]"  
      continue;
    }
	// an unparsed arg means just return the number of args parsed...
	else break; 
  }
  
  // ==================== END ARGUMENT PARSING ======================

  if (opt->mDMXStereo) {
    opt->backendRendererName = "gl_stereo";
  } 
  
  if (get_verbose() > 5) {
    opt->mCacheDebug = true; 
  }
  enableCacheDebug(opt->mCacheDebug);
    
  numProcessors = GetNumProcessors();
  if (opt->readerThreads == -1) {
    if (numProcessors > 1) {
      opt->readerThreads = max(numProcessors-2,1);
    } 
    dbprintf(1, "User did not specify thread count; using %d\n", opt->readerThreads); 
  }
  if (opt->readerThreads < 0) {
    dbprintf(0, "Error: number of threads cannot be %d\n", opt->readerThreads);
    exit(1); 
  }
  if (opt->readerThreads == 0) {
    dbprintf(0, "Warning: threads disabled.\n", opt->readerThreads);
  }
  DEBUGMSG("Using %d threads", opt->readerThreads); 

  if (opt->masterHost != "") {
	opt->slaveMode = 1;
    QStringList hostTokens=opt->masterHost.split(":"); 
    opt->masterHost = hostTokens[0]; 
 
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
      ERROR("The default master port is no longer used for security reasons"); 
      exit(2);
    }
    DEBUGMSG(QString("SlaveHost: %1 Port: %2\n")
             .arg(opt->masterHost).arg(opt->masterPort));  
  }
  

  if (opt->mMaxCachedImages == 0) {
    dbprintf(0, "Warning:  image cache size set to 0 by user.  No preloading or caching will be done and no threading will be done.\n"); 
    opt->preloadFrames = 0;
    opt->readerThreads = 0; 
  }
  else if (opt->mMaxCachedImages < 0){
    dbprintf(0, "Error: image cache size cannot be %d\n", opt->mMaxCachedImages);
    exit(1); 
  }  
  
    if (opt->preloadFrames == 0) {
      dbprintf(0, "Warning: preloadFrames set to 0 by user.  No preloading will be done.\n"); 
    }
    else if (opt->preloadFrames < 0){
      dbprintf(0, "Error: preloadFrames cannot be %d\n", opt->preloadFrames);
      exit(1); 
    }
  
  if ( opt->preloadFrames && opt->preloadFrames >= opt->mMaxCachedImages) {
    dbprintf(0, "Error: preloadFrames %d must be less than cache size %d.\n", 
             opt->preloadFrames, opt->mMaxCachedImages);
    exit (1); 
  }
   
  DEBUGMSG("Preload is %d and cache size is %d\n", opt->preloadFrames, opt->mMaxCachedImages); 

  /* We've read all the command line options, so everything is set.
   * Pull out some of the information we'll need later.
   */
  if (opt->repeatCountName == "forever") {
    opt->repeatCount = REPEAT_FOREVER;
  }
  else {
    opt->repeatCount = opt->repeatCountName.toInt();
  }
    
  EnableTracing(opt->mTraceEvents, opt->mTraceEventsFilename); 
  
  return  ; 
}

void printargs(string description, char *args[], int argc) {
  QString arglist = QString("PRINTARGS: %1: ").arg(description.c_str()); 
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
  DEBUGMSG("WARNING:  ExitHandler called, but dmx_AtExitCleanup no longer being called.\n");
  //dmx_AtExitCleanup();
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


// =====================================================================
/*!  
  MAIN()  This is slightly important.
*/
int main(int argc, char *argv[])
{
  ProgramOptions *opt = GetGlobalOptions(); 
  Preferences Prefs; 
  
  Prefs.AddArg(ArgType("help", "bool").SetFlags())
    .AddArg(ArgType("cachesize").SetValue(100).SetLongFlag())
    .AddArg(ArgType("cachedebug", "bool").SetLongFlag())
    .AddArg(ArgType("no-decorations", "bool").SetFlags("--no-decorations", "-D"))
    .AddArg(ArgType("fullscreen", "bool").SetFlags())
    .AddArg(ArgType("display").SetValue( ":0").SetFlags()); 
  
  Slave *theSlave; 
  char localSettingsFilename[BLOCKBUSTER_PATH_MAX];
  char homeSettingsFilename[BLOCKBUSTER_PATH_MAX];
  int homeSettingsFileExists = 0, localSettingsFileExists = 0;
  char *envHOME;
  struct stat statBuf;
  int rv, retval = 0;
  
 
  version(); // announce our self
  /*! 
	If we are running dmx, then kill the slaves before exiting the program
  */ 
  atexit(ExitHandler); 

  /* Grab the INT signal so we can clean up */
  signal(SIGINT, InterruptHandler);

  /* This hack should prevent Qt 4.2.3 from segfaulting: */
  char noglib[] = "QT_NO_GLIB=1";
  putenv(noglib); 

  /* We might be saving settings files.  Look for them
   * in the user's home directory, and in the current
   * directory.
   */
  opt->settings = (Settings*)CreateBlankSettings();
  envHOME = getenv("HOME");
  if (envHOME != NULL) {
    sprintf(homeSettingsFilename, "%s/.blockbusterrc", envHOME);
    rv = stat(homeSettingsFilename, &statBuf);
    if (rv == 0 && S_ISREG(statBuf.st_mode)) {
      homeSettingsFileExists = 1;
      ReadSettingsFromFile(opt->settings, homeSettingsFilename);
    }
  }
  sprintf(localSettingsFilename, "./.blockbusterrc");
  rv = stat(localSettingsFilename, &statBuf);
  if (rv == 0 && S_ISREG(statBuf.st_mode)) {
    localSettingsFileExists = 1;
    ReadSettingsFromFile(opt->settings, localSettingsFilename);
  }


  /* Grab any options that apply to the whole program */
  char **newargs = DuplicateArgs(argc, argv); 
  int newargc = argc;
  int dummy =0; 
  // Get Qt rockin'.  This creates the basic Qt object.  

  ParseEnvironmentVars(); 
  ParseOptions(opt, newargc, newargs);
  printargs("After ParseOptions", newargs, newargc); 

  /* put the DISPLAY into the environment for Qt */
  if (opt->displayName != "") {
    char buf[2048]; 
    sprintf(buf, QString("DISPLAY=%1").arg(opt->displayName).toStdString().c_str()); 
    putenv(buf);
  }
  
  
  gCoreApp = new QApplication(dummy, newargs); 
  if (opt->executable == "") {
    opt->executable = gCoreApp->applicationFilePath(); 
  }

  // register the main thread first
  RegisterThread(QThread::currentThread(), opt->readerThreads, true); 

  /* initialize the slave portion if we are a slave */
  if (opt->slaveMode != 0) {
    theSlave = new Slave(opt); 
    if (!theSlave || !theSlave->InitNetwork() || !theSlave->GetDisplayName()) {
      printf("Initialization failed. Slave will now exit.\n"); 
      exit(1); 
    }
    SuppressMessageDialogs(true); 
  } else {
    gMainWindow = new BlockbusterInterface; 
    
    /* set up network communications */ 
    gSidecarServer = new SidecarServer;     
    if (opt->drawInterface){
      gMainWindow->show(); 
    }
  }

  INFO(QString("Using %1 renderer").arg(opt->rendererName));
  
  // set up a connection to sidecar if that's what launched us
  if (opt->sidecarHostPort != "") {
    INFO(QString("Connecting to sidecar on %1\n").arg(opt->sidecarHostPort)); 
    gSidecarServer->connectToSidecar(opt->sidecarHostPort); 
    SuppressMessageDialogs(true); 
    
  }
  
  vector<MovieEvent> script; 
  if (opt->mScript != "") {
    script = MovieEvent::ParseScript(opt->mScript.toStdString());
    if (!script.size()) {
      dbprintf(0, "Script %s was invalid\n", opt->mScript.toStdString().c_str()); 
      return 1;
    }
  }
  if (opt->mReplayEventsFilename != "") {
    if (script.size()) {
      dbprintf(0, "Warning:  overriding previously parsed script file because event file is given.\n"); 
    }
    script = MovieEvent::ParseScript(opt->mReplayEventsFilename.toStdString());
    if (!script.size()) {
      dbprintf(0, "Event file %s was invalid\n", opt->mScript.toStdString().c_str()); 
    }
  }
  // Movie arguments are now placed at the head of any scripting commands. 
  for (int count = 1; count < newargc && newargs[count];  count++) {
    script.insert(script.begin(), MovieEvent("MOVIE_OPEN_FILE", newargs[count]));
  }
    
  if (opt->slaveMode) {
    retval = theSlave->Loop();
    INFO("Done with slave loop.\n");
  }
  
  retval = DisplayLoop(opt, script);

  /* If we read settings, write them back out.  Only one of the
   * two files we read should contain the changed settings creaed
   * inside the program.  If the local file exists, it takes all
   * the changed settings; otherwise, the home file gets them.
   */
  {
    int settingsSources = SETTINGS_FROM_PROGRAM | SETTINGS_FROM_FILE;

    if (localSettingsFileExists) {
      WriteSettingsToFile(opt->settings, localSettingsFilename,
                          settingsSources);
      settingsSources &= ~SETTINGS_FROM_PROGRAM;
    }

    /* We'll always attempt to write to the RC file in the
     * home directory.  If the file doesn't exist, and no
     * settings have to be rewritten, the file won't
     * be created; but if anything is missing, we'll
     * store it here.
     */
    WriteSettingsToFile(opt->settings, homeSettingsFilename,
                        settingsSources);
  }
  INFO("Blockbuster is finished.\n"); 
  /* All done.  Go home. */
  return retval;
}
