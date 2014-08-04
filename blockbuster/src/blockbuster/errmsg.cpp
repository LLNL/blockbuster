#include "errmsg.h"
#include "events.h"
#include "common.h"
#include <sys/time.h>
#include "SidecarServer.h"
#include "util.h"
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <fstream>
#include "QThread"
#include "QTime"
#include "QMessageBox"
#include <boost/format.hpp> 
#include "settings.h"
using namespace std; 


// ==================================================================
/* This tracefile stuff gets put here because it seems I/O related.  
   Kind of didn't know where to stick it actually. 
*/
void EnableTracing(bool enable, string filename) {
  ProgramOptions *opt = GetGlobalOptions(); 
  opt->mTraceEvents = enable; 
  opt->mTraceEventsFilename = filename.c_str(); 
  if (opt->mTraceEvents) {
	if (opt->mTraceEventsFilename == "") {	  
	  time_t now = time(0);
	  tm *ltm = localtime(&now);
	  char *home = getenv("HOME");
	  if (home) {
		opt->mTraceEventsFilename = str(boost::format("%1%/.blockbuster/events.trace")%home).c_str();
	  }
	  else {
		opt->mTraceEventsFilename = tmpnam(NULL);
	  }
	  if (FILE *file = fopen(opt->mTraceEventsFilename.c_str(), "r")) {	
		fclose(file); 
		// save any existing version of the file in a temp dir:	  
		char *user = getenv("USER");
		if (!user) user = (char*) "blockbuster-tmp"; 
		string dirname = str(boost::format("/tmp/%1%")%user); 
		if (mkdir (dirname.c_str(), 0777) == -1) {
		  if (errno != EEXIST) {
			opt->mTraceEvents = false; 
			dbprintf(0, "Warning, directory %s could not be created for tracefile.  Error is \"%s\"", dirname.c_str(), strerror(errno));
			return;
		  }
		}
		string savefilename = str(boost::format("%1%/bbtrace-%2%-%3%-%4%-%5%-%6%-%7%")%dirname%(ltm->tm_year)%(ltm->tm_mon)%(ltm->tm_mday)%(ltm->tm_hour)%(ltm->tm_min)%(ltm->tm_sec));
		
		std::ifstream  src(opt->mTraceEventsFilename.c_str());
		std::ofstream  dst(savefilename.c_str());
		
		dst << src.rdbuf();
		dst.close(); 
		src.close(); 
		src.open(savefilename.c_str()); 
		if (!src.is_open()) {
		  dbprintf(0, "Warning, old tracefile %s could not be backed up to %s.  Error is \"%s\"\n", opt->mTraceEventsFilename.c_str(),savefilename.c_str(), strerror(errno)); 
		} 
		else {
		  dbprintf(0, "Backed up old tracefile %s to %s.\n", opt->mTraceEventsFilename.c_str(),savefilename.c_str()); 
		}		
	  } 
	}
	opt->mTraceEventsFile = fopen(opt->mTraceEventsFilename.c_str(), "w"); 
	if (opt->mTraceEventsFile) {
	  dbprintf(0, "Tracing events to log file %s\n", opt->mTraceEventsFilename.c_str());
	}  
	else {
	  dbprintf(0, "Warning: could not open log file %s\n", opt->mTraceEventsFilename.c_str());   
	}
  }
  return; 
}


// ==================================================================
/*!
  For certain tight timings -- generally want this off
*/ 
int gTimerOn = 0; 

void enableTimer(bool onoff) {
  gTimerOn=onoff; 
}

/* ---------------------------------------------*/ 
/* for error reporting in dialogs, folks have been using a canvas, bleh*/ 
int maxMessageLevel = M_WARNING;

struct MessageRec {
  MessageRec():thread(NULL), threadnum(0) {
    return; 
  }
  MessageRec(QThread *t, uint32_t n):thread(t), threadnum(n) {
    return; 
  }
  MessageRec(const MessageRec &r) {
    *this = r; 
  }

  const MessageRec &operator = (const MessageRec &r) {
    file = r.file; 
    function = r.function; 
    line = r.line; 
    level = r.level; 
    thread = r.thread;
    threadnum = r.threadnum; 
    return *this; 
  }

  string file;
  string function;
  int     line;
  int     level;
  QThread *thread; 
  uint32_t threadnum; 
} ;

map<QThread *, MessageRec> gMessageRecs;

void addMessageRec(QThread *thread, uint32_t threadnum){
  MessageRec r(thread, threadnum); 
  gMessageRecs[thread] = MessageRec(thread, threadnum);
  return; 
}

void removeMessageRec(QThread *thread) {
  gMessageRecs.erase(thread); 
}

void setMessageRec(const char* file, const char* func, uint32_t line, uint32_t level) {
  gMessageRecs[QThread::currentThread()].file = file; 
  gMessageRecs[QThread::currentThread()].function = func; 
  gMessageRecs[QThread::currentThread()].line = line; 
  gMessageRecs[QThread::currentThread()].level = level; 
  return; 
}

pthread_mutex_t debug_message_lock = PTHREAD_MUTEX_INITIALIZER; 
static bool gDoDialogs = true; 
static int gVerbose = 0; 

static QString gLogFileName; 
static ofstream gLogFile; 

//===============================================
QString getExactSeconds(void) {
  struct timeval t; 
  gettimeofday(&t, NULL); 
  //return QTime::currentTime().toString("ssss.zzz").toStdString(); 
  return QString("%1").arg(t.tv_sec + (double)t.tv_usec/1000000.0, 0, 'f', 3).right(8); 
}

//===============================================
void enableLogging(bool enable, QString logfile) {
  if (enable) {
    if (logfile != "") {
      gLogFileName = logfile; 
    } else {
      char *home = getenv("HOME");
      if (home) {
        gLogFileName = QString("%1/.blockbuster/blockbuster-log.txt").arg(home);
      } else {
        gLogFileName = "blockbuster-log.txt";
      }
    }
    gLogFile.open(gLogFileName.toStdString().c_str(), std::ofstream::out ); 
    if (gLogFile) {
      cerr << "Blockbuster log is at " << gLogFileName.toStdString() << endl; 
    } else {
      cerr << "Could not open blockbuster log " << gLogFileName.toStdString() << endl; 
    }
  } else {
    gLogFile.close(); 
    gLogFileName = ""; 
  }
  return; 
}

//===============================================
void set_verbose(int level) {
  maxMessageLevel = gVerbose = level; 
}
int get_verbose(void) { 
  return maxMessageLevel; 
}

#define DBPRINTF_PREAMBLE \
  str(boost::format("<t=%1%> %2%:%3%, %4%():  ") %                      \
      (getExactSeconds().toStdString())%                                \
      (gMessageRecs[QThread::currentThread()].file)%                    \
      string(gMessageRecs[QThread::currentThread()].function)%          \
      (gMessageRecs[QThread::currentThread()].line))

//===============================================
void real_dbprintf(int level, QString msg) {
  real_dbprintf(level, msg.toStdString()); 
}

//===============================================
void real_dbprintf(int level, string msg) { 
  if (gVerbose < level) return; 
  std::string preamble = DBPRINTF_PREAMBLE; 
  cerr << preamble << msg << endl;
  if (gLogFile.is_open()) {
    gLogFile << preamble << msg << endl;
  }
  return ;
}
#define MAX_MSG_LENGTH (100*1024)

//===============================================
void real_dbprintf(int level, const char *fmt, ...) {
  static char msgbuf[MAX_MSG_LENGTH+1]; 
  if (gVerbose < level) return; 
  std::string preamble = DBPRINTF_PREAMBLE; 
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(msgbuf, MAX_MSG_LENGTH, fmt, ap);
  va_end(ap);
  cerr <<  preamble << msgbuf; 
  if (gLogFile.is_open()) {
    gLogFile << preamble << msgbuf; 
  }
  return; 
}

/*!
  The slaves will call this to prevent error messages from popping up on DMX displays from the backends.
*/ 
void SuppressMessageDialogs(bool yn) {
  gDoDialogs = !yn; 
}

/*!
  Attempt to display a message in a dialog, unless there is no main window, e.g. we are a slave or just using X11
*/
int DisplayDialog(const char *message) { 
  if (!gDoDialogs || !InMainThread() ) return 0; 
    char buffer[4096]; 
    
    snprintf(buffer, 4096, "%s\n\n(%s:%s():%d)",
             message, 
             gMessageRecs[QThread::currentThread()].file.c_str(), 
             gMessageRecs[QThread::currentThread()].function.c_str(), 
             gMessageRecs[QThread::currentThread()].line);
    switch (gMessageRecs[QThread::currentThread()].level) {
    case M_SYSERROR:
      QMessageBox::critical(NULL, "Blockbuster Error",
                            QString(buffer));
      break; 
    case M_ERROR:
      QMessageBox::warning(NULL, "Blockbuster Error",
                           QString(buffer));
      break; 
    case M_WARNING:
      QMessageBox::information(NULL, "Blockbuster Note",
                               QString(buffer));
      break; 
    case M_INFO:
    case M_DEBUG:
    default:
      return 0; 
    }
    return 1; 

}
/*********************************************************************
 * Message reporting mechanism.  Other modules use this to report
 * errors and warnings.
 */

struct MessageLevel messageLevels[] = {
  {(char*)"quiet", (char*)"Emit no messages (verbosity 0)", MINMESSAGE - 1},
  {(char*)"syserr", (char*)"Only emit system-level error messages (verbosity 1)", M_SYSERROR},
  {(char*)"error", (char*)"Emit system and program error messages (verbosity 2)", M_ERROR},
  {(char*)"warning", (char*)"Emit all errors and program warnings (verbosity 3)", M_WARNING},
  {(char*)"info", (char*)"Emit errors, warnings, and progress information (verbosity 4)", M_INFO},
  {(char*)"debug", (char*)"Emit debugging information (verbosity 5)", M_DEBUG},
  {NULL, NULL, 0},
};
 
/* map verbosity from 0 to 5 onto other system for my sanity */
struct MessageLevel *FindMessageLevel(int verbosity) {
  switch (verbosity) {
  case 0: return FindMessageLevel("quiet"); break; 
  case 1: return FindMessageLevel("system"); break; 
  case 2: return FindMessageLevel("error"); break; 
  case 3: return FindMessageLevel("warning"); break; 
  case 4: return FindMessageLevel("info"); break; 
  case 5: return FindMessageLevel("debug"); break; 
  default: return FindMessageLevel("quiet"); break; 
  }
  return NULL; // cannot get here
}

/* the heart of the mechanism -- I hate it, but here it is -- fix this later */
struct MessageLevel *FindMessageLevel(QString name)
{
  int i = 0;
  while (messageLevels[i].name != NULL) {
    if (name == messageLevels[i].name) {
      return &messageLevels[i];
    }
    i++;
  }
    
  return NULL;
}


void NullMessage(const char *format,...)
{
  format="unused"; 
  return;
}
  
void Message (std::string msg) {
  Message(msg.c_str()); 
}

void Message(QString msg) {
  Message(msg.toStdString()); 
}

/* some pure C code calls this, so have to make it accessible */
#ifdef __cplusplus 
extern "C" {
#endif
void Message(const char *format,...)

{
  static string localHostname = GetLocalHostname(); 
  //static string dbfilename=string("/g/g0/rcook/bb_debug-") + localHostname + ".debug"; 
  //static ofstream *dbfile = new ofstream(dbfilename.c_str()); 
  va_list args;
  char buffer[BLOCKBUSTER_PATH_MAX];

  if (gMessageRecs[QThread::currentThread()].level <= maxMessageLevel) {

    /* Collect the arguments into a single string */
    va_start(args, format);
    (void) vsnprintf(buffer, BLOCKBUSTER_PATH_MAX, format, args);
    va_end(args);


    /* Always output to console, then perhaps to dialog
     */
    QString errmsg("BLOCKBUSTER ("); 
    errmsg += localHostname.c_str(); 
    errmsg += "): ";

    switch(gMessageRecs[QThread::currentThread()].level) {
    case M_SYSERROR:  errmsg += "SYSERROR: "; break;
    case M_ERROR: errmsg += "ERROR: "; break;
    case M_WARNING: errmsg += "WARNING: "; break;
    case M_INFO: errmsg += "INFO: "; break;
    case M_DEBUG: errmsg += "DEBUG: "; break;
    }
    errmsg += buffer; 
    if (maxMessageLevel >= M_DEBUG) {
#if 1
      struct timeval t; 
      gettimeofday(&t, NULL); 
        // TIME EVERYTHING! 
      QString timestring = QString("%1").arg(t.tv_sec + (double)t.tv_usec/1000000.0, 0, 'f', 3).right(8); 
#else
      QString timestring = "(not computed)"; 
#endif 

      errmsg += QString(" [%1:%2() line %3, thread %4, time=%5]")
        .arg(gMessageRecs[QThread::currentThread()].file.c_str())
        .arg( gMessageRecs[QThread::currentThread()].function.c_str())
        .arg(QString::number(gMessageRecs[QThread::currentThread()].line))
        //.arg((long)QThread::currentThread())
        .arg(GetCurrentThreadID())
        .arg(timestring);       
    }
    cerr << errmsg.toStdString() << endl ;
    cerr.flush(); 
    errmsg.replace("\n", "  --- [ newline ] --- "); 
    //gSidecarServer->SendEvent(MovieEvent("MOVIE_SIDECAR_MESSAGE", errmsg)); 
    
    if (gMessageRecs[QThread::currentThread()].level == M_SYSERROR) {
      perror("blockbuster");
    }
    // this only does something if gDoDialogs is true and we're in the main thread. 
	DisplayDialog(buffer);
    
    
  }
}
#ifdef __cplusplus 
}
#endif
