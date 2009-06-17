#ifndef ERRMSG_H
#define ERRMSG_H
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __cplusplus
#include <QString>
struct MessageLevel *FindMessageLevel(QString name);
struct MessageLevel *FindMessageLevel(int verbosity); // maps verbosity from 0 to 5 into "messageLevel" from SYSERROR to DEBUG 
#endif
extern int maxMessageLevel; 
extern struct Canvas *messageCanvas;

struct MessageLevel {
  char *name;
  char *description;
  int messageLevel;
}; 
extern pthread_mutex_t debug_message_lock; 
#ifdef DEBUG
// #warning turning on dbprintf statements
#define dbprintf theMessage.file=__FILE__,theMessage.function=__FUNCTION__,theMessage.line=__LINE__,real_dbprintf
#else
// this is a noop for most compilers... 
#define dbprintf if (0) real_dbprintf
#endif
void set_verbose(int level); 

//void real_dbprintf(const char *fmt, ...); 
#ifdef __cplusplus
void real_dbprintf(int level, const char *fmt, ...);
void real_dbprintf(int level, QString msg); 
//void real_dbprintf(QString msg); 
#endif
// dbprintf stuff is in common.h, as it is used in sidecar
/*!
  The slaves will call this to prevent error messages from popping up on DMX displays from the backends.
*/ 
extern void SuppressMessageDialogs(void);

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

struct MessageRec {
  const char *file;
  const char *function;
  int     line;
  int     level;
} ;

extern struct MessageRec theMessage;

#ifdef linux
#define MESSAGEBASE \
theMessage.file=__FILE__,theMessage.function=__FUNCTION__,theMessage.line=__LINE__,Message
#else
#define MESSAGEBASE \
   theMessage.file=__FILE__,theMessage.function="(unknown)",theMessage.line=__LINE__,Message
#endif
#define SYSERROR \
   theMessage.level=M_SYSERROR,MESSAGEBASE
#define ERROR \
   theMessage.level=M_ERROR,MESSAGEBASE
#define WARNING \
   theMessage.level=M_WARNING,MESSAGEBASE
#define INFO \
   theMessage.level=M_INFO,MESSAGEBASE
  /* #ifdef DEBUG */
#define DEBUGMSG \
    theMessage.level=M_DEBUG,MESSAGEBASE           \

#ifdef __cplusplus 

void Message(QString msg); 

extern "C" {
#endif
   void Message(const char *format, ...);
#ifdef __cplusplus 
}
#endif
   void NullMessage(const char *format, ...);


#endif
