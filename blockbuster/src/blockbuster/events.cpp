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
MovieEventType MovieEvent::StringToMovieEventType(string  iType) {
  if (iType == "MOVIE_NONE") return MOVIE_NONE; 
  if (iType == "MOVIE_EXPOSE") return MOVIE_EXPOSE; 
  if (iType == "MOVIE_RESIZE") return MOVIE_RESIZE; 
  if (iType == "MOVIE_FULLSCREEN") return MOVIE_FULLSCREEN; 
  if (iType == "MOVIE_IMAGE_MOVE") return MOVIE_IMAGE_MOVE; 
  if (iType == "MOVIE_MOVE") return MOVIE_MOVE; 
  if (iType == "MOVIE_MOVE_RESIZE") return MOVIE_MOVE_RESIZE; 
  if (iType == "MOVIE_CENTER") return MOVIE_CENTER; 
  if (iType == "MOVIE_TOGGLE_CURSOR") return MOVIE_TOGGLE_CURSOR; 
  if (iType == "MOVIE_ZOOM_IN") return MOVIE_ZOOM_IN; 
  if (iType == "MOVIE_ZOOM_OUT") return MOVIE_ZOOM_OUT; 
  if (iType == "MOVIE_ZOOM_FIT") return MOVIE_ZOOM_FIT; 
  if (iType == "MOVIE_ZOOM_ONE") return MOVIE_ZOOM_ONE; 
  if (iType == "MOVIE_ZOOM_SET") return MOVIE_ZOOM_SET; 
  if (iType == "MOVIE_ZOOM_UP") return MOVIE_ZOOM_UP; 
  if (iType == "MOVIE_ZOOM_DOWN") return MOVIE_ZOOM_DOWN; 
  if (iType == "MOVIE_INCREASE_LOD") return MOVIE_INCREASE_LOD; 
  if (iType == "MOVIE_DECREASE_LOD") return MOVIE_DECREASE_LOD; 
  if (iType == "MOVIE_SET_LOD") return MOVIE_SET_LOD; 
  if (iType == "MOVIE_OPEN_FILE") return MOVIE_OPEN_FILE; 
  if (iType == "MOVIE_OPEN_FILE_NOCHANGE") return MOVIE_OPEN_FILE_NOCHANGE; 
  if (iType == "MOVIE_PLAY_FORWARD") return MOVIE_PLAY_FORWARD; 
  if (iType == "MOVIE_PLAY_BACKWARD") return MOVIE_PLAY_BACKWARD; 
  if (iType == "MOVIE_SET_LOOP") return MOVIE_SET_LOOP; 
  if (iType == "MOVIE_SET_PINGPONG") return MOVIE_SET_PINGPONG; 
  if (iType == "MOVIE_CONTINUE") return MOVIE_CONTINUE; 
  if (iType == "MOVIE_PAUSE") return MOVIE_PAUSE; 
  if (iType == "MOVIE_STOP") return MOVIE_STOP; 
  if (iType == "MOVIE_STEP_FORWARD") return MOVIE_STEP_FORWARD; 
  if (iType == "MOVIE_STEP_BACKWARD") return MOVIE_STEP_BACKWARD; 
  if (iType == "MOVIE_SKIP_FORWARD") return MOVIE_SKIP_FORWARD; 
  if (iType == "MOVIE_SKIP_BACKWARD") return MOVIE_SKIP_BACKWARD; 
  if (iType == "MOVIE_SECTION_FORWARD") return MOVIE_SECTION_FORWARD; 
  if (iType == "MOVIE_SECTION_BACKWARD") return MOVIE_SECTION_BACKWARD; 
  if (iType == "MOVIE_SLOWER") return MOVIE_SLOWER; 
  if (iType == "MOVIE_INCREASE_RATE") return MOVIE_INCREASE_RATE; 
  if (iType == "MOVIE_DECREASE_RATE") return MOVIE_DECREASE_RATE; 
  if (iType == "MOVIE_SET_RATE") return MOVIE_SET_RATE; 
  if (iType == "MOVIE_START_END_FRAMES") return MOVIE_START_END_FRAMES; 
  if (iType == "MOVIE_GOTO_START") return MOVIE_GOTO_START; 
  if (iType == "MOVIE_GOTO_END") return MOVIE_GOTO_END; 
  if (iType == "MOVIE_GOTO_FRAME") return MOVIE_GOTO_FRAME; 
  if (iType == "MOVIE_SAVE_FRAME") return MOVIE_SAVE_FRAME; 
  if (iType == "MOVIE_MOUSE_PRESS_1") return MOVIE_MOUSE_PRESS_1; 
  if (iType == "MOVIE_MOUSE_PRESS_2") return MOVIE_MOUSE_PRESS_2; 
  if (iType == "MOVIE_MOUSE_PRESS_3") return MOVIE_MOUSE_PRESS_3; 
  if (iType == "MOVIE_MOUSE_RELEASE_1") return MOVIE_MOUSE_RELEASE_1; 
  if (iType == "MOVIE_MOUSE_RELEASE_2") return MOVIE_MOUSE_RELEASE_2; 
  if (iType == "MOVIE_MOUSE_RELEASE_3") return MOVIE_MOUSE_RELEASE_3; 
  if (iType == "MOVIE_MOUSE_MOVE") return MOVIE_MOUSE_MOVE; 
  if (iType == "MOVIE_SHOW_INTERFACE") return MOVIE_SHOW_INTERFACE; 
  if (iType == "MOVIE_HIDE_INTERFACE") return MOVIE_HIDE_INTERFACE; 
  if (iType == "MOVIE_TOGGLE_INTERFACE") return MOVIE_TOGGLE_INTERFACE; 
  if (iType == "MOVIE_QUIT") return MOVIE_QUIT; 
  if (iType == "MOVIE_STOP_ERROR") return MOVIE_STOP_ERROR; 
  if (iType == "MOVIE_SIDECAR_BACKCHANNEL") return MOVIE_SIDECAR_BACKCHANNEL; 
  if (iType == "MOVIE_SIDECAR_MESSAGE") return MOVIE_SIDECAR_MESSAGE; 
  if (iType == "MOVIE_SIDECAR_STATUS") return MOVIE_SIDECAR_STATUS; 
  if (iType == "MOVIE_SNAPSHOT") return MOVIE_SNAPSHOT; 
  if (iType == "MOVIE_SNAPSHOT_STARTFRAME") return MOVIE_SNAPSHOT_STARTFRAME; 
  if (iType == "MOVIE_SNAPSHOT_ENDFRAME") return MOVIE_SNAPSHOT_ENDFRAME; 
  if (iType == "MOVIE_SNAPSHOT_ALT_ENDFRAME") return MOVIE_SNAPSHOT_ALT_ENDFRAME; 
  if (iType == "MOVIE_PWD") return MOVIE_PWD; 
  if (iType == "MOVIE_CUE_BEGIN") return MOVIE_CUE_BEGIN; 
  if (iType == "MOVIE_CUE_MOVIE_NAME") return MOVIE_CUE_MOVIE_NAME; 
  if (iType == "MOVIE_CUE_PLAY_ON_LOAD") return MOVIE_CUE_PLAY_ON_LOAD; 
  if (iType == "MOVIE_CUE_PLAY_BACKWARD") return MOVIE_CUE_PLAY_BACKWARD; 
  if (iType == "MOVIE_CUE_END") return MOVIE_CUE_END; 
  if (iType == "MOVIE_CUE_COMPLETE") return MOVIE_CUE_COMPLETE; 
  if (iType == "MOVIE_SLAVE_SWAP_COMPLETE") return MOVIE_SLAVE_SWAP_COMPLETE; 
  if (iType == "MOVIE_SLAVE_EXIT") return MOVIE_SLAVE_EXIT; 
  if (iType == "MOVIE_SLAVE_COMMAND_FAILED") return MOVIE_SLAVE_COMMAND_FAILED; 
  if (iType == "MOVIE_SLAVE_ERROR") return MOVIE_SLAVE_ERROR; 
  if (iType == "MOVIE_SAVE_IMAGE") return MOVIE_SAVE_IMAGE; 
  if (iType == "MOVIE_MESSAGE") return MOVIE_MESSAGE; 
  if (iType == "MOVIE_DISABLE_CURRENT_FRAME") return MOVIE_DISABLE_CURRENT_FRAME; 
  
  return MOVIE_NONE; 
}
  
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
    script.push_back(event); 
  }
      
  return script; 
}

//==========================================================================
// Scripts are simple:  
// EVENT CODE ( :  OPTIONAL VALUE )
bool MovieEvent::ParseScriptLine(string line) {
  vector<string>tokens; 
  boost::split(tokens, line, boost::is_any_of(":"), boost::token_compress_on);
  mEventType = MOVIE_NONE; 
  MovieEventType eventType = StringToMovieEventType(tokens[0]); 
  for (uint32_t i = 0; i< tokens.size(); i++) {
    boost::trim(tokens[i]); 
  }

  switch (eventType) {
  case MOVIE_OPEN_FILE: // or any other string-based event
    if (tokens.size() < 2) {
      dbprintf(0, "ERROR: MovieEvent::ParseScriptLine: bad line: %s", line.c_str()); 
      return false; 
    }
    mString = tokens[1]; 
    mEventType = eventType; 
    return true; 

  case MOVIE_GOTO_FRAME:  // or any other single-integer-based event
    if (tokens.size() < 2) {
      dbprintf(0, "ERROR: MovieEvent::ParseScriptLine: bad line: %s", line.c_str()); 
      return false; 
    }
    mNumber = boost::lexical_cast<int32_t> (tokens[1]); 
    mEventType = eventType; 
    return true; 

  default:
    return false; 
  }
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
  return str(boost::format("%1 %2 %3 %4 %5 %6 %7 %8 string: %9\n")
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
    throw QString("Bad event in FromQString(): numtokens in \"%1\"should be 8 but is %2").arg(line).arg(numtokens); 
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

