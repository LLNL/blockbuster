#ifndef BLOCKBUSTER_COMMON_H
#define BLOCKBUSTER_COMMON_H

// THIS FILE WILL NOT COMPILE WITHOUT A C++ COMPILER

#include "QApplication"
#include "QDataStream"
#include "QTcpServer"
#include "QThread"
//#include "blockbuster_qt.h"
class BlockbusterInterface; 
#define TIMER_PRINT DEBUGMSG

// =====================================
/* Since sidecar and blockbuster share keyboard controls, document them
   in a common place:
*/
void PrintKeyboardControls(void); 

uint16_t GetCurrentThreadID(void) ; 
void RegisterThread(QThread *qthreadptr); 
void UnregisterThread(QThread *qthreadptr); 

  /* Useful macros */
#define MAX2(a,b) ((a)<(b)?(b):(a))
#define MIN2(a,b) ((a)>(b)?(b):(a))

  /* This macro rounds a given value "x" upwards to the nearest multiple of 
   * "m".  It's often used in pixmap calculations, as it seems pixmaps often
   * have scanlines rounded up to a given multiple.
   */
#define ROUND_TO_MULTIPLE(x,m) ((m)*((int)((x) + (m) - 1)/(m)))

//extern nonsense *blah; 
extern BlockbusterInterface *gMainWindow; 
extern QApplication *gCoreApp; // Defined in movie.cpp
extern QThread *gMainThread; 


/* 
   extern bool gDbprintf; 
   void enable_dbprintf(bool enable=true); 
   void real_dbprintf(const char *fmt, ...);
   void real_dbprintf(QString msg);
   
   #define dbprintf if (gDbprintf) real_dbprintf
*/ 
//=============================================================
/* this does not go in errmsg.h, as it is used by sidecar */
/*
#ifdef DEBUG
// #warning turning on dbprintf statements
#define dbprintf real_dbprintf
#else
// this is a noop for most compilers... 
#define dbprintf if (0) real_dbprintf
#endif
void set_verbose(int level); 

void real_dbprintf(int level, const char *fmt, ...);
void real_dbprintf(int level, QString msg); 
void real_dbprintf(QString msg); 
void real_dbprintf(const char *fmt, ...); 
*/
/* BLOCKBUSTER_PATH_MAX is used not only for the maximum length of a filename,
 * but for the maximum length of just about any string read from
 * a file
 */
#define BLOCKBUSTER_PATH_MAX 4096

  /* Constants for the byteOrder are chosen to match X11's definition */
#define LSB_FIRST (LSBFirst)
#define MSB_FIRST   (MSBFirst)

  /* Constants for the rowOrder are our own. */
#define TOP_TO_BOTTOM       1
#define BOTTOM_TO_TOP       2
#define ROW_ORDER_DONT_CARE 3

  /* For geometry */
#define DONT_CARE 194243 /* unlikely but positive for uint's */
#define CENTER -998

  /* A simple type (defined primarily to improve code readability) used
   * when a success/failure message is required.
   */
  typedef enum {
    MovieFailure = 0,
    MovieSuccess = 1
  } MovieStatus;

  /* Main program exit codes */
#define MOVIE_OK    0
#define MOVIE_HELP    1
#define MOVIE_BAD_FLAG    2
#define MOVIE_FATAL_ERROR 3

struct Rectangle {
  qint32 x, y;
  qint32 width, height;
} ;

int RectContainsRect(const Rectangle *r1, const Rectangle *r2);
Rectangle RectUnionRect(const Rectangle *r1, const Rectangle *r2);

int LODFromZoom(float zoom);
double GetCurrentTime(void);

#endif
