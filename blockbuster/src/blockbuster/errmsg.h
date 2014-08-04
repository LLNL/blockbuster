#ifndef ERRMSG_H
#define ERRMSG_H
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define ECHO_FUNCTION(level) dbprintf(level, "BEGIN\n"); 
#ifdef __cplusplus
#include <QString>
#include "QThread"
#include <pthread.h>

// ----------------------------------
// For tracing movie events:
void EnableTracing(bool enable, std::string filename);
// ----------------------------------


extern int gTimerOn; 
#define TIMER_PRINT(...) if (gTimerOn) DEBUGMSG(__VA_ARGS__)

struct MessageLevel *FindMessageLevel(QString name);
struct MessageLevel *FindMessageLevel(int verbosity); // maps verbosity from 0 to 5 into "messageLevel" from SYSERROR to DEBUG 

// prints output to file at maximum debug level.  Warning:  big files will result!  
void enableLogging(bool enable, QString logfile);
#endif
extern int maxMessageLevel; 

struct MessageLevel {
  char *name;
  char *description;
  int messageLevel;
}; 
extern pthread_mutex_t debug_message_lock; 
// #ifdef DEBUG
// #warning turning on dbprintf statements
#define dbprintf   setMessageRec(__FILE__,__FUNCTION__,__LINE__, 0), real_dbprintf
/*
  Not necessary to suppress for normal builds -- very nominal performance hit.
   #else
   // this is a noop for most compilers... 
   #define dbprintf if (0) real_dbprintf
   #endif
*/
void set_verbose(int level); 
int get_verbose(void);

//void real_dbprintf(const char *fmt, ...); 
#ifdef __cplusplus
void real_dbprintf(int level, const char *fmt, ...);
void real_dbprintf(int level, QString msg); 
void real_dbprintf(int level, std::string msg); 
//void real_dbprintf(QString msg); 
// dbprintf stuff is in common.h, as it is used in sidecar
/*!
  The slaves will call this to prevent error messages from popping up on DMX displays from the backends.
*/ 
void SuppressMessageDialogs(bool);
#endif

// assertions are kind of sucky.  I prefer an actual error.  This looks uninformative, but ERROR at least tells you a line number.  Later I could add more informative info as well.   
#define bb_assert(condition)                      \
  if (!(condition)) {\
    ERROR("Failed assertion");             \
    exit(1) ; \
  }

  /* This routine is used for reporting messages.  Each message level
   * has its own macro so that they can be easily taken out of 
   * a compilation (by redefining that macro).
   */
#define MINMESSAGE  0
#define M_SYSERROR  0
#define M_ERROR   1
#define M_WARNING 2
#define M_INFO    4
#define M_DEBUG   5

  /* To address portability issues with varargs macros, we use
   * this approach.  Basically, one fills out this global struct
   * and calls the Message() function.  This allows the macro to
   * be written so as to fill out the struct and then remapping
   * the function call.  The varargs bit stays in C. Note: there
   * is a race condition here obviously...
   */

#ifdef __cplusplus 
void addMessageRec(QThread *thread, uint32_t threadnum);
void removeMessageRec(QThread *thread);
#endif
#define DOMESSAGE(level)                                        \
  setMessageRec(__FILE__,__FUNCTION__,__LINE__, level), Message
#define SYSERROR   DOMESSAGE(M_SYSERROR)
#define ERROR      DOMESSAGE(M_ERROR)
#define WARNING    DOMESSAGE(M_WARNING)
#define INFO       DOMESSAGE(M_INFO)
#define DEBUGMSG   DOMESSAGE(M_DEBUG)
#ifdef __cplusplus 

void Message(std::string msg); 
void Message(QString msg); 

extern "C" {
#endif
 void setMessageRec(const char* file, const char* func, uint32_t line, uint32_t level); 
  void Message(const char *format, ...);
#ifdef __cplusplus 
}
#endif
   void NullMessage(const char *format, ...);


#endif
