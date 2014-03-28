#ifndef BLOCKBUSTER_COMMON_H
#define BLOCKBUSTER_COMMON_H

// THIS FILE WILL NOT COMPILE WITHOUT A C++ COMPILER

#include "QApplication"
#include "QDataStream"
#include "QTcpServer"
#include "QThread"
#include <unistd.h>
#include <stdint.h>
#include "boost/shared_ptr.hpp"
#include "boost/format.hpp"

//#include "blockbuster_qt.h"
class BlockbusterInterface; 

// =====================================
/* Since sidecar and blockbuster share keyboard controls, document them
   in a common place:
*/
void PrintKeyboardControls(void); 

bool InMainThread(void);
uint16_t GetCurrentThreadID(void) ; 
int RegisterThread(QThread *qthreadptr, int id, bool isMain = false); 
void UnregisterThread(QThread *qthreadptr); 

  /* Useful macros */
#define MAX2(a,b) ((a)<(b)?(b):(a))
#define MIN2(a,b) ((a)>(b)?(b):(a))

  /* This macro rounds a given value "x" upwards to the nearest multiple of 
   * "m".  It's often used in pixmap calculations, as it seems pixmaps often
   * have scanlines rounded up to a given multiple.
   */
#define ROUND_UP_TO_MULTIPLE(x,m) (m*((int)((x) + (m) - 1)/m))

//extern nonsense *blah; 
extern BlockbusterInterface *gMainWindow; 
extern QApplication *gCoreApp; // Defined in movie.cpp
extern QThread *gMainThread; 



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
#define CENTER -99998

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


typedef boost::shared_ptr<struct Rectangle> RectanglePtr; 

struct Rectangle {
  Rectangle():x(0),y(0),width(0), height(0){}
  Rectangle(qint32 ix, qint32 iy, qint32 w, qint32 h):
    x(ix),y(iy),width(w),height(h) {}
  operator QString() const {
    return QString(std::string(*this).c_str()); 
  }
  operator std::string() const {
    return str(boost::format("Rectangle {%1%, %2%, %3%, %4%}") % (x) % (y) % (width) % (height)); 
  } 
  qint32 x, y;
  qint32 width, height;
} ;

//! RectContainsRect: Return 1 if r1 contains r2, else return 0.
int RectContainsRect(const Rectangle * r1, const Rectangle * r2);
Rectangle RectUnionRect(const Rectangle * r1, const Rectangle * r2);

double GetCurrentTime(void);

#endif
