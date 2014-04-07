#include "events.h"
#include "QStringList"
#include "errmsg.h"
#include <fstream>
#include <boost/format.hpp>
#include <boost/scoped_array.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
using namespace std; 

//==========================================================================
// This will parse either an Event file or a script.  
vector<MovieEvent> MovieEvent::ParseScript(string filename) {
  vector<MovieEvent> script; 
  if (filename[0] != '/') {
    int psize = pathconf(".", _PC_PATH_MAX); 
    boost::scoped_array<char> path(new char[psize]);   
    char *cwd = getcwd(path.get(), psize-1);
    if (!cwd) {
      dbprintf(0, "Cannot get current working directory!\n"); 
      return script; 
    }
    filename = string(cwd) + "/" + filename; 
  }
  ifstream sfile(filename.c_str()); 
  if (!sfile.is_open()) {    
    INFO((string("ERROR: MovieEvent::ParseScript cannot open: ")+filename).c_str()); 
    return script; 
  }
  string line; 
  MovieEvent event; 
  while (!sfile.eof()) {
    std::getline(sfile, line); 
    if (! event.ParseScriptLine(line)) {
      return script; 
    }
    if (event.mEventType != "MOVIE_NONE") {
      script.push_back(event); 
    }
  }
      
  return script; 
}

//==========================================================================
// Scripts are simple:  
// EVENT CODE ( :  OPTIONAL VALUE )
// or for event files, the output of string(event)
bool MovieEvent::ParseScriptLine(string line) {
  bool dummy = 0; 
  init("MOVIE_NONE"); 
  try {
    (*this) << line; 
    return true; 
  } 
  catch (...) {
    dummy = 1; 
  }
    
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
  //MovieEventType eventType = StringToMovieEventType(tokens[0]); 
  if (tokens[0] == "MOVIE_STOP_ERROR") {   
    return false; 
  }
  mEventType = tokens[0]; 
  if (mEventType == "MOVIE_OPEN_FILE" || // or any other string-based event
      mEventType == "MOVIE_SAVE_FRAME") {  // string to name the file
    if (tokens.size() < 2) {
      dbprintf(0, "ERROR: MovieEvent::ParseScriptLine: bad line: %s", line.c_str()); 
      return false; 
    }
    mString = tokens[1]; 
    return true; 
  }
  // Handle all integer events.  
  if (mEventType == "MOVIE_GOTO_FRAME" ||  
      mEventType == "MOVIE_ZOOM_TO_FIT" ||  
      mEventType == "MOVIE_FULLSCREEN" ||  
      mEventType == "MOVIE_SIZE_TO_MOVIE" ||  
      mEventType == "MOVIE_SET_LOD" ||   
      mEventType == "MOVIE_RESIZE" || 
      mEventType == "MOVIE_MOVE" || 
      mEventType == "MOVIE_MOVE_RESIZE" || 
      mEventType == "MOVIE_IMAGE_MOVE" || 
      mEventType == "MOVIE_MOUSE_PRESS_1" || 
      mEventType == "MOVIE_MOUSE_PRESS_2" || 
      mEventType == "MOVIE_MOUSE_PRESS_3" || 
      mEventType == "MOVIE_MOUSE_RELEASE_1" || 
      mEventType == "MOVIE_MOUSE_RELEASE_2" || 
      mEventType == "MOVIE_MOUSE_RELEASE_3" || 
      mEventType == "MOVIE_MOUSE_MOVE") {
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
  }
  if (mEventType == "MOVIE_ZOOM_SET" ||   // or any other float-based event
      mEventType == "MOVIE_SET_RATE") {
    mRate = boost::lexical_cast<float> (tokens[1]);
  }
  return true;  
}


//==========================================================================
MovieEvent::operator string(void) const {
  string s = str(boost::format("mEventType=%1% mID=%2% mNumber=%3% mRate=%4% mWidth=%5% mHeight=%6% mX=%7% mY=%8% mString=<<<< %9% >>>>")
                 % (mEventType)
                 % (mID)
                 % (mNumber)
                 % (mRate)
                 % (mWidth)
                 % (mHeight)
                 % (mX)
                 % (mY)
                 % (mString));
  return s; 
}

//==========================================================================
MovieEvent &MovieEvent::operator <<(string line) {
  return *this << QString(line.c_str()); 
}

//==========================================================================
MovieEvent &MovieEvent::operator <<(QString line) {
  init("MOVIE_NONE"); 
  if (line.endsWith("\n")) line.chop(1);

  QStringList tokens = line.trimmed().split(QRegExp("\\s+"), QString::SkipEmptyParts);                       
  for (QStringList::iterator pos = tokens.begin(); pos != tokens.end(); pos++){
    QStringList items = pos->split("="); 
    if (items.size() != 2) { 
      throw QString("FromQString(): Event \"%1\" has malformed key \"%2\"").arg(line).arg(*pos);
    }
    QString key = items[0], value=items[1]; 
    if (key == "mEventType") {
      mEventType = value.toStdString();
    }
    else if (key == "mID") {
      mID = value.toInt();  
    }
    else if (key == "mNumber") {
      mNumber = value.toInt();
    }
    else if (key == "mRate") {
      mRate = value.toFloat();
    }
    else if (key == "mWidth") {
      mWidth = value.toInt();
    }
    else if (key == "mHeight") {
      mHeight = value.toInt();
    }
    else if (key == "mX") {
      mX = value.toInt();
    }
    else if (key == "mY") {
      mY = value.toInt(); 
    }
    else if (key == "mString") {
      ++pos; 
      bool stop = (*pos == ">>>>"); 
      while (!stop) {
        if (pos != tokens.end() && *pos != ">>>>") {        
          mString += pos->toStdString(); 
        }
        ++pos; 
        if (pos == tokens.end() || *pos == ">>>>") {
          stop = true; 
        } 
        else {
          mString += " "; 
        }
      }
      if (pos == tokens.end()) {
        mEventType = "MOVIE_NONE"; 
        throw QString("FromQString(): Event \"%1\" has malformed string").arg(line);        
      }
    }
  }
  return *this; 
}

//==========================================================================
const QIODevice &operator >> (QIODevice &iFile, MovieEvent &event) {
  QString line = iFile.readLine(); 
  event << line; 
  dbprintf(5, "%s:%d -- Received: \"%s\"   -------->   \"%s\"\n",__FILE__, __LINE__, line.toStdString().c_str(), string(event).c_str()); 
  return iFile; 
 
}

//==========================================================================
const QIODevice &operator << (QIODevice &iFile, MovieEvent &event) {
  string estring = string(event) + "\n"; 
  int64_t bytes = iFile.write(estring.c_str(), estring.size());
  dbprintf(5, "%s:%d -- Sent %d bytes: \"%s\" \n",__FILE__, __LINE__, bytes, string(event).c_str());
  return iFile; 
}

//==========================================================================
// inefficient, but it was easy to type :-)
bool MovieSnapshot::operator == (const MovieSnapshot &other) const{
  return (string(*this) == string(other) ); 
}


//==========================================================================
MovieSnapshot::operator string() const {
  string retval = str(boost::format("mSnapshotType=%s mFilename=%s mFrameRate=%0.6f mTargetFPS=%0.6f mZoom=%0.6f mLOD=%d mStereo=%d mPlayStep=%d mStartFrame=%d mEndFrame=%d mNumFrames=%d mFrameNumber=%d mRepeat=%d mPingPong=%d mFullScreen=%d mSizeToMovie=%d mZoomToFit=%d mNoScreensaver=%d mScreenHeight=%d mScreenWidth=%d mScreenXpos=%d mScreenYpos=%d mImageHeight=%d mImageWidth=%d mImageXpos=%d mImageYpos=%d")
                      %(mSnapshotType)
                      %(mFilename)
                      %(mFrameRate)
                      %(mTargetFPS)
                      %(mZoom)
                      %(mLOD)
                      %(mStereo)
                      %(mPlayStep)
                      %(mStartFrame)
                      %(mEndFrame)
                      %(mNumFrames)
                      %(mFrameNumber)
                      %(mRepeat)
                      %(mPingPong)
                      %(mFullScreen)
                      %(mSizeToMovie)
                      %(mZoomToFit)
                      %(mNoScreensaver)
                      %(mScreenHeight)
                      %(mScreenWidth)
                      %(mScreenXpos)
                      %(mScreenYpos)
                      %(mImageHeight)
                      %(mImageWidth)
                      %(mImageXpos)
                      %(mImageYpos));
  
  return retval; 
  
}
//==========================================================================
MovieSnapshot &MovieSnapshot::operator <<(string s) {
  
  QString theString = s.c_str(); 
  QStringList parts(theString.trimmed().split(QRegExp("\\s+"), QString::SkipEmptyParts));  
  try {
    for (QStringList::iterator pos = parts.begin(); pos != parts.end(); pos++) {
      QStringList items = pos->split("="); 
      if (items.size() != 2) { 
        throw QString("operator <<(): MovieSnapshot \"%1\" has malformed key=value pair \"%2\"").arg(theString).arg(*pos);
      }
      QString key = items[0], value=items[1]; 
      if (key == "mSnapshotType") mSnapshotType = value.toStdString();
      else if (key == "mFilename") mFilename = value.toStdString();
      else if (key == "mFrameRate") mFrameRate = value.toFloat();
      else if (key == "mTargetFPS") mTargetFPS = value.toFloat();
      else if (key == "mZoom") mZoom = value.toFloat();
      else if (key == "mLOD") mLOD = value.toInt();
      else if (key == "mStereo") mStereo = value.toInt();
      else if (key == "mPlayStep") mPlayStep = value.toInt();
      else if (key == "mStartFrame") mStartFrame = value.toInt();
      else if (key == "mEndFrame") mEndFrame = value.toInt();
      else if (key == "mNumFrames") mNumFrames = value.toInt();
      else if (key == "mFrameNumber") mFrameNumber = value.toInt();
      else if (key == "mRepeat") mRepeat = value.toInt(); 
      else if (key == "mPingPong") mPingPong = value.toInt(); 
      else if (key == "mFullScreen") mFullScreen = value.toInt();
      else if (key == "mSizeToMovie") mSizeToMovie = value.toInt();
      else if (key == "mZoomToFit") mZoomToFit = value.toInt();
      else if (key == "mNoScreensaver") mNoScreensaver = value.toInt();
      else if (key == "mScreenHeight") mScreenHeight = value.toInt();
      else if (key == "mScreenWidth") mScreenWidth = value.toInt();
      else if (key == "mScreenXpos") mScreenXpos = value.toInt();
      else if (key == "mScreenYpos") mScreenYpos = value.toInt();
      else if (key == "mImageHeight") mImageHeight = value.toInt();
      else if (key == "mImageWidth") mImageWidth = value.toInt();
      else if (key == "mImageXpos") mImageXpos = value.toInt();
      else if (key == "mImageYpos") mImageYpos = value.toInt();
      else {
        throw QString("operator <<(): MovieSnapshot \"%1\" has unknown key \"%2\"").arg(theString).arg(key);
      }
        
    }
  }
  catch (...) {
    cerr << str(boost::format("Error: cannot create snapshot from string: \"%1%\"")%s) << endl;
  }
  return *this; 
}

