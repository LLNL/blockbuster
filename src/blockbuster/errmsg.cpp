#include "errmsg.h"
#include "events.h"
#include "common.h"
#include <sys/time.h>
#include "SidecarServer.h"
#include "canvas.h"
#include "util.h"
#include <iostream>
#include <fstream>
#include "QThread"
#include "QTime"
#include "QMessageBox"
using namespace std; 

/* ---------------------------------------------*/ 
/* for error reporting in dialogs, folks have been using a canvas, bleh*/ 
Canvas *messageCanvas = NULL; // 
int maxMessageLevel = M_WARNING;
MessageRec theMessage;

pthread_mutex_t debug_message_lock = PTHREAD_MUTEX_INITIALIZER; 
static bool gDoDialogs = true; 
static int gVerbose = 0; 

//===============================================
QString getExactSeconds(void) {
  struct timeval t; 
  gettimeofday(&t, NULL); 
  //return QTime::currentTime().toString("ssss.zzz").toStdString(); 
  return QString("%1").arg(t.tv_sec + (double)t.tv_usec/1000000.0, 0, 'f', 3).right(8); 
}

//===============================================
void set_verbose(int level) {
  maxMessageLevel = gVerbose = level; 
}

#define DBPRINTF_PREAMBLE QString("<t=%1> %2:%3, %4():  ").arg(getExactSeconds()).arg(theMessage.file).arg(theMessage.line).arg(theMessage.function).toStdString()

//===============================================
void real_dbprintf(int level, QString msg) { 
  if (gVerbose < level) return; 
  cerr << DBPRINTF_PREAMBLE << msg.toStdString() << endl;
  return ;
}

//===============================================
void real_dbprintf(int level, const char *fmt, ...) {
  if (gVerbose < level) return; 
  cerr <<  DBPRINTF_PREAMBLE; 
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr,fmt,ap);
  va_end(ap);
  return; 
}

/*!
  The slaves will call this to prevent error messages from popping up on DMX displays from the backends.
*/ 
void SuppressMessageDialogs(void) {
  gDoDialogs = false; 
}

/*!
  Attempt to display a message in a dialog, unless there is no main window, e.g. we are a slave or just using X11
*/
int DisplayDialog(const char *message) { 
    char buffer[4096]; 
    snprintf(buffer, 4096, "%s\n\n(%s:%s():%d)",
             message, theMessage.file, 
             theMessage.function, theMessage.line);
    switch (theMessage.level) {
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
  {"quiet", "Emit no messages (verbosity 0)", MINMESSAGE - 1},
  {"syserr", "Only emit system-level error messages (verbosity 1)", M_SYSERROR},
  {"error", "Emit system and program error messages (verbosity 2)", M_ERROR},
  {"warning", "Emit all errors and program warnings (verbosity 3)", M_WARNING},
  {"info", "Emit errors, warnings, and progress information (verbosity 4)", M_INFO},
  {"debug", "Emit debugging information (verbosity 5)", M_DEBUG},
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
  
void Message(QString msg) {
  Message(msg.toStdString().c_str()); 
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
  
  if (theMessage.level <= maxMessageLevel) {

    /* Collect the arguments into a single string */
    va_start(args, format);
    (void) vsnprintf(buffer, BLOCKBUSTER_PATH_MAX, format, args);
    va_end(args);

	if (gDoDialogs && DisplayDialog(buffer)) return; 

    /* If we get to here, either we don't have a messaging canvas,
     * or it failed to report the message.  We'll handle it on
     * the console instead.
     */
    QString errmsg("BLOCKBUSTER ("); 
    errmsg += localHostname.c_str(); 
    errmsg += "): ";

    switch(theMessage.level) {
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
        .arg(theMessage.file).arg( theMessage.function)
        .arg(QString::number(theMessage.line))
        //.arg((long)QThread::currentThread())
        .arg(GetCurrentThreadID())
        .arg(timestring);       
    }
    cerr << errmsg.toStdString() << endl;     
    errmsg.replace("\n", "  --- [ newline ] --- "); 
    //gSidecarServer->SendEvent(MovieEvent(MOVIE_SIDECAR_MESSAGE, errmsg)); 
    
    if (theMessage.level == M_SYSERROR) {
      perror("blockbuster");
    }
  }
}
#ifdef __cplusplus 
}
#endif
