#include "events.h"
#include "QStringList"
#include "errmsg.h"

using namespace std; 
//==========================================================================


  //---------------------------------
  uint32_t MovieEventTypeToUint32(MovieEventType iType) {
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
  //---------------------------------
  MovieEventType Uint32ToMovieEventType (uint32_t iInt) {
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
const QIODevice &operator >> (QIODevice &iFile, MovieEvent &event) {
  QString line = iFile.readLine(); 
  event.FromQString(line); 
  dbprintf(5, "%s:%d -- Received: \"%s\"   -------->   \"%s\"\n",__FILE__, __LINE__, line.toStdString().c_str(), event.Stringify().c_str()); 
  return iFile; 
 
}

//==========================================================================
const QIODevice &operator << (QIODevice &iFile, MovieEvent &event) {
  QString estring = event.ToQString(); 
  int64_t bytes = iFile.write(estring.toStdString().c_str(), estring.size());
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

