#include "events.h"
#include "QStringList"
#include "errmsg.h"
#include <fstream>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
using namespace std; 
//==========================================================================

//---------------------------------
uint32_t MovieEvent::MovieEventTypeToUint32(MovieEventType iType) {
  uint32_t result = 0; 
  switch(sizeof(iType)){
  case 1:
    { 
      uint8_t *ip = (uint8_t*)(&iType); 
      result = *ip; 
    }
    break;
  case 2:
    { 
      uint16_t *ip = (uint16_t*)(&iType); 
      result = *ip; 
    }
    break;
  case 4:
    {
      uint32_t *ip = (uint32_t*)(&iType); 
      result = *ip; 
    }
    break;
  default:
    result = 0; 
  }
  return result; 
}

//==========================================================================
//---------------------------------
MovieEventType MovieEvent::Uint32ToMovieEventType (uint32_t iInt) {
  MovieEventType result; 
  switch(sizeof(result)){
  case 1:
    { 
      uint8_t *ip = (uint8_t*)(&result); 
      *ip = iInt; 
    }
    break;
  case 2:
    { 
      uint16_t *ip = (uint16_t*)(&result); 
      *ip = iInt; 
    }
    break;
  case 4:
    {
      uint32_t *ip = (uint32_t*)(&result); 
      *ip = iInt; 
    }
    break;
  default:
    result = MOVIE_NONE;
  }
  return result; 
}
  
//==========================================================================
// CREATE THE FOLLOWING TWO FUNCTIONS USING MACROS:
//string MovieEvent::MovieEventTypeToString(MovieEventType  iType) {
//==========================================================================
//MovieEventType MovieEvent::StringToMovieEventType(string  iType) {
  
#define STRING2EVENT(event,argname) if (argname == #event) return event
#define RETURNSTRING(s) return #s
#define EVENT2STRING(event,argname) if (argname == event) return #event
#define RETURNEVENT(e) return e

#define CREATEFUNC(rname,fname,pname,mac,retmac)    \
  rname MovieEvent::fname(pname p) {                \
    mac(MOVIE_NONE,p);                              \
    mac(MOVIE_EXPOSE,p);                            \
    mac(MOVIE_RESIZE ,p);                           \
    mac(MOVIE_FULLSCREEN,p);                        \
    mac(MOVIE_IMAGE_MOVE,p );                       \
    mac(MOVIE_MOVE,p);                              \
    mac(MOVIE_MOVE_RESIZE,p);                       \
    mac(MOVIE_CENTER,p);                            \
    mac(MOVIE_TOGGLE_CURSOR,p);                     \
    mac(MOVIE_NOSCREENSAVER,p);                     \
    mac(MOVIE_SET_STEREO,p);                        \
    mac(MOVIE_DISABLE_DIALOGS,p);                   \
    mac(MOVIE_ZOOM_IN,p);                           \
    mac(MOVIE_ZOOM_OUT,p);                          \
    mac(MOVIE_ZOOM_FIT,p);                          \
    mac(MOVIE_ZOOM_ONE,p);                          \
    mac(MOVIE_ZOOM_SET,p);                          \
    mac(MOVIE_ZOOM_UP,p);                           \
    mac(MOVIE_ZOOM_DOWN,p);                         \
    mac(MOVIE_INCREASE_LOD,p);                      \
    mac(MOVIE_DECREASE_LOD,p);                      \
    mac(MOVIE_SET_LOD,p);                           \
    mac(MOVIE_OPEN_FILE,p);                         \
    mac(MOVIE_OPEN_FILE_NOCHANGE,p);                \
    mac(MOVIE_PLAY_FORWARD,p);                      \
    mac(MOVIE_PLAY_BACKWARD,p);                     \
    mac(MOVIE_SET_LOOP,p);                          \
    mac(MOVIE_SET_PINGPONG,p);                      \
    mac(MOVIE_CONTINUE,p);                          \
    mac(MOVIE_PAUSE,p);                             \
    mac(MOVIE_STOP,p);                              \
    mac(MOVIE_STEP_FORWARD,p);                      \
    mac(MOVIE_STEP_BACKWARD,p);                     \
    mac(MOVIE_SKIP_FORWARD,p);                      \
    mac(MOVIE_SKIP_BACKWARD,p);                     \
    mac(MOVIE_SECTION_FORWARD,p);                   \
    mac(MOVIE_SECTION_BACKWARD,p);                  \
    mac(MOVIE_SLOWER,p);                            \
    mac(MOVIE_INCREASE_RATE,p);                     \
    mac(MOVIE_DECREASE_RATE,p);                     \
    mac(MOVIE_SET_RATE,p);                          \
    mac(MOVIE_START_END_FRAMES,p);                  \
    mac(MOVIE_GOTO_START,p);                        \
    mac(MOVIE_GOTO_END,p);                          \
    mac(MOVIE_GOTO_FRAME,p);                        \
    mac(MOVIE_SAVE_FRAME,p);                        \
    mac(MOVIE_MOUSE_PRESS_1,p);                     \
    mac(MOVIE_MOUSE_PRESS_2,p);                     \
    mac(MOVIE_MOUSE_PRESS_3,p);                     \
    mac(MOVIE_MOUSE_RELEASE_1,p);                   \
    mac(MOVIE_MOUSE_RELEASE_2,p);                   \
    mac(MOVIE_MOUSE_RELEASE_3,p);                   \
    mac(MOVIE_MOUSE_MOVE,p);                        \
    mac(MOVIE_SHOW_INTERFACE,p);                    \
    mac(MOVIE_HIDE_INTERFACE,p);                    \
    mac(MOVIE_TOGGLE_INTERFACE,p);                  \
    mac(MOVIE_QUIT,p);                              \
    mac(MOVIE_STOP_ERROR,p);                        \
    mac(MOVIE_SIDECAR_BACKCHANNEL,p);               \
    mac(MOVIE_SIDECAR_MESSAGE,p);                   \
    mac(MOVIE_SIDECAR_STATUS,p);                    \
    mac(MOVIE_SNAPSHOT,p);                          \
    mac(MOVIE_SNAPSHOT_STARTFRAME,p);               \
    mac(MOVIE_SNAPSHOT_ENDFRAME,p);                 \
    mac(MOVIE_SNAPSHOT_ALT_ENDFRAME,p);             \
    mac(MOVIE_PWD,p);                               \
    mac(MOVIE_CUE_BEGIN,p);                         \
    mac(MOVIE_CUE_MOVIE_NAME,p);                    \
    mac(MOVIE_CUE_PLAY_ON_LOAD,p);                  \
    mac(MOVIE_CUE_PLAY_BACKWARD,p);                 \
    mac(MOVIE_CUE_END,p);                           \
    mac(MOVIE_CUE_COMPLETE,p);                      \
    mac(MOVIE_SLAVE_SWAP_COMPLETE,p);               \
    mac(MOVIE_SLAVE_EXIT,p);                        \
    mac(MOVIE_SLAVE_COMMAND_FAILED,p);              \
    mac(MOVIE_SLAVE_ERROR,p);                       \
    mac(MOVIE_SAVE_IMAGE,p);                        \
    mac(MOVIE_MESSAGE,p);                           \
    mac(MOVIE_DISABLE_CURRENT_FRAME,p);             \
    retmac(MOVIE_NONE);                             \
  }

CREATEFUNC(string, MovieEventTypeToString, MovieEventType, EVENT2STRING, RETURNSTRING);

CREATEFUNC(MovieEventType, StringToMovieEventType, string, STRING2EVENT,RETURNEVENT);


//==========================================================================
  vector<MovieEvent> MovieEvent::ParseScript(string filename) {
  vector<MovieEvent> script; 
  ifstream sfile(filename.c_str()); 
  if (!sfile.is_open()) {
    INFO(0, (string("ERROR: MovieEvent::ParseScript cannot open: ")+filename).c_str()); 
    return script; 
  }
  string line; 
  MovieEvent event; 
  while (!sfile.eof()) {
    std::getline(sfile, line); 
    if (! event.ParseScriptLine(line)) {
      return script; 
    }
    if (event.mEventType != MOVIE_NONE) {
      script.push_back(event); 
    }
  }
      
  return script; 
}

//==========================================================================
// Scripts are simple:  
// EVENT CODE ( :  OPTIONAL VALUE )
bool MovieEvent::ParseScriptLine(string line) {

  init(MOVIE_NONE); 

  vector<string>tokens; 
  boost::split(tokens, line, boost::is_any_of(":"), boost::token_compress_on);

  for (vector<string>::iterator pos = tokens.begin(); pos != tokens.end(); pos++) {
    if (pos->size() != 0 && (*pos)[0] == '#') {
      tokens.erase(pos, tokens.end()); 
      break; 
    }
    boost::trim(*pos); 
  }
  if (!tokens.size()) {
    // comment or empty line
    return true; 
  }
  MovieEventType eventType = StringToMovieEventType(tokens[0]); 
  mEventType = eventType; 

  switch (eventType) {
  case MOVIE_STOP_ERROR:    // StringToMovieEventType failed
    return false; 
  case MOVIE_OPEN_FILE: // or any other string-based event
  case MOVIE_SAVE_FRAME:  // string to name the file
    if (tokens.size() < 2) {
      dbprintf(0, "ERROR: MovieEvent::ParseScriptLine: bad line: %s", line.c_str()); 
      return false; 
    }
    mString = tokens[1]; 
    mEventType = eventType; 
    return true; 

    // Handle all integer events.  
  case MOVIE_GOTO_FRAME: 
  case MOVIE_SET_LOD:  
  case MOVIE_RESIZE:
  case MOVIE_MOVE:
  case MOVIE_MOVE_RESIZE:
  case MOVIE_IMAGE_MOVE:
  case MOVIE_MOUSE_PRESS_1:
  case MOVIE_MOUSE_PRESS_2:
  case MOVIE_MOUSE_PRESS_3:
  case MOVIE_MOUSE_RELEASE_1:
  case MOVIE_MOUSE_RELEASE_2:
  case MOVIE_MOUSE_RELEASE_3:
  case MOVIE_MOUSE_MOVE:
    if (tokens.size() < 2) {
      dbprintf(0, "ERROR: MovieEvent::ParseScriptLine: argument(s) missing in line: %s", line.c_str()); 
      return false; 
    }
    if (tokens.size() == 2) {
      mNumber = boost::lexical_cast<int32_t> (tokens[1]); 
    }
    if (tokens.size() > 2) {
      mX = boost::lexical_cast<int32_t> (tokens[1]); // reuse first argument.
      mY = boost::lexical_cast<int32_t> (tokens[2]); 
    }
    if (tokens.size() > 4) {
      mWidth = boost::lexical_cast<int32_t> (tokens[3]); 
      mHeight = boost::lexical_cast<int32_t> (tokens[4]); 
    } 
    return true; 

  case MOVIE_ZOOM_SET:  // or any other float-based event
  case MOVIE_SET_RATE:
    mRate = boost::lexical_cast<float> (tokens[1]);
    return true; 
  default:
    return true;     
  }
  // not possible to get here: 
  return false;  
}

//==========================================================================
std::string MovieEvent::Stringify(void) {
  return str(boost::format("Event: type %1%, ID %2%, mNumber = %3%, rate = %4%, (width,height) = (%5%, %6%), (x,y) = (%7%, %8%), string: \"%9%\"")
             // % (MovieEventTypeToUint32(mEventType)) % (ID)
             % (MovieEventTypeToString(mEventType)) % (mID)
             % (mNumber) % (mRate) % (mWidth) % (mHeight) % (mX) % (mY)
             % (mString)); 
}

 
//==========================================================================
string MovieEvent::ToString(void) {
  return str(boost::format("%1% %2% %3% %4% %5% %6% %7% %8% string: %9%\n")
             % (MovieEventTypeToUint32(mEventType))
             % (mID)
             % (mNumber)
             % (mRate)
             % (mWidth)
             % (mHeight)
             % (mX)
             % (mY)
             % (mString));
}

//==========================================================================
void MovieEvent::FromQString(QString line) {
  if (line.endsWith("\n")) line.chop(1);

  QStringList tokens = line.split(" ");                       
  int numtokens = tokens.size(); 
  if (numtokens < 8) {
    throw QString("Bad event in FromQString(): numtokens in \"%1\"should be at least 8 but is %2").arg(line).arg(numtokens); 
  }
  QStringList::iterator pos = tokens.begin(); 
  mEventType = Uint32ToMovieEventType(pos->toInt()); ++pos; 
  mID = pos->toInt();  ++pos; 
  mNumber = pos->toInt();  ++pos; 
  mRate = pos->toFloat();  ++pos; 
  mWidth = pos->toInt();  ++pos; 
  mHeight = pos->toInt();  ++pos; 
  mX = pos->toInt();  ++pos; 
  mY = pos->toInt();  ++pos; 
      
  if (numtokens > 9) {
    tokens = line.split("string: ");  
    if (tokens.size() < 2) {
      mEventType = MOVIE_NONE; 
      throw QString("FromQString(): Event \"%1\" is missing string").arg(line); 
    }
    mString = tokens[1].toStdString(); 
  }
  return; 
}

//==========================================================================
const QIODevice &operator >> (QIODevice &iFile, MovieEvent &event) {
  QString line = iFile.readLine(); 
  event.FromQString(line); 
  dbprintf(5, "%s:%d -- Received: \"%s\"   -------->   \"%s\"\n",__FILE__, __LINE__, line.toStdString().c_str(), event.Stringify().c_str()); 
  return iFile; 
 
}

//==========================================================================
const QIODevice &operator << (QIODevice &iFile, MovieEvent &event) {
  string estring = event.ToString(); 
  int64_t bytes = iFile.write(estring.c_str(), estring.size());
  dbprintf(5, "%s:%d -- Sent %d bytes: \"%s\" \n",__FILE__, __LINE__, bytes, event.Stringify().c_str());
  return iFile; 
}

//==========================================================================
// inefficient, but it was easy to type :-)
bool MovieSnapshot::operator == (const MovieSnapshot &other) const{
  return (this->toString() == other.toString()); 
}

//==========================================================================
QString MovieSnapshot::humanReadableString(void) const {
  QString retval = QString("Snapshot:  mSnapshotType=%1 mFilename=%2 mFrameRate=%3 mTargetFPS=%4 mZoom=%5 mLOD=%6 mPlayStep=%7 mStartFrame=%8 mEndFrame=%9 mNumFrames=%10 mFrameNumber=%11 mLoop=%12 mPingPong=%13 mFullScreen=%14 mZoomOne=%15 mNoScreensaver=%16 mScreenHeight=%17 mScreenWidth=%18 mScreenXpos=%19 mScreenYpos=%20 mImageHeight=%21 mImageWidth=%22 mImageXpos=%23 mImageYpos=%24")
    .arg(mSnapshotType)
    .arg(mFilename)
    .arg(mFrameRate)
    .arg(mTargetFPS)
    .arg(mZoom)
    .arg(mLOD)
    .arg(mPlayStep)
    .arg(mStartFrame)
    .arg(mEndFrame)
    .arg(mNumFrames)
    .arg(mFrameNumber)
    .arg(mLoop)
    .arg(mPingPong)
    .arg(mFullScreen)
    .arg(mZoomOne)
    .arg(mNoScreensaver)
    .arg(mScreenHeight)
    .arg(mScreenWidth)
    .arg(mScreenXpos)
    .arg(mScreenYpos)
    .arg(mImageHeight)
    .arg(mImageWidth)
    .arg(mImageXpos)
    .arg(mImageYpos);

  //dbprintf("Snapshot::toString(): \"%s\"\n", (const char*)retval.toAscii());
  return retval; 
  
}

//==========================================================================
QString MovieSnapshot::toString(void) const {
  QString retval = QString("%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14 %15 %16 %17 %18 %19 %20 %21 %22 %23 %24")
    .arg(mSnapshotType)
    .arg(mFilename)
    .arg(mFrameRate)
    .arg(mTargetFPS)
    .arg(mZoom)
    .arg(mLOD)
    .arg(mPlayStep)
    .arg(mStartFrame)
    .arg(mEndFrame)
    .arg(mNumFrames)
    .arg(mFrameNumber)
    .arg(mLoop)
    .arg(mPingPong)
    .arg(mFullScreen)
    .arg(mZoomOne)
    .arg(mNoScreensaver)
    .arg(mScreenHeight)
    .arg(mScreenWidth)
    .arg(mScreenXpos)
    .arg(mScreenYpos)
    .arg(mImageHeight)
    .arg(mImageWidth)
    .arg(mImageXpos)
    .arg(mImageYpos);

  //dbprintf("Snapshot::toString(): \"%s\"\n", (const char*)retval.toAscii());
  return retval; 
  
}
//==========================================================================
void MovieSnapshot::fromString(QString iString) {
  QStringList parts(iString.split(" "));  
  try {
    int partnum = 0;    
    mSnapshotType     = parts[partnum++].toInt();
    mFilename         = parts[partnum++];
    mFrameRate        = parts[partnum++].toFloat();
    mTargetFPS        = parts[partnum++].toFloat();
    mZoom             = parts[partnum++].toFloat();
    mLOD              = parts[partnum++].toInt();
    mPlayStep         = parts[partnum++].toInt();
    mStartFrame       = parts[partnum++].toInt();
    mEndFrame         = parts[partnum++].toInt();
    mNumFrames        = parts[partnum++].toInt();
    mFrameNumber      = parts[partnum++].toInt();
    mLoop             = parts[partnum++].toInt(); 
    mPingPong         = parts[partnum++].toInt(); 
    mFullScreen       = parts[partnum++].toInt();
    mZoomOne          = parts[partnum++].toInt();
    mNoScreensaver    = parts[partnum++].toInt();
    mScreenHeight     = parts[partnum++].toInt();
    mScreenWidth      = parts[partnum++].toInt();
    mScreenXpos       = parts[partnum++].toInt();
    mScreenYpos       = parts[partnum++].toInt();
    mImageHeight      = parts[partnum++].toInt();
    mImageWidth       = parts[partnum++].toInt();
    mImageXpos        = parts[partnum++].toInt();
    mImageYpos        = parts[partnum++].toInt();
  }
  catch (...) {
    cerr << QString("Error: cannot create snapshot from string: \"%1\"").arg(iString).toStdString() << endl;
  }
  return; 
}

