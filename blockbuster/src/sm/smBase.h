/*
** $RCSfile: smBase.h,v $
** $Name:  $
**
** ASCI Visualization Project 
**
** Lawrence Livermore National Laboratory
** Information Management and Graphics Group
** P.O. Box 808, Mail Stop L-561
** Livermore, CA 94551-0808
**
** For information about this project see:
** 	http://www.llnl.gov/sccd/lc/img/
**
**      or contact: asciviz@llnl.gov
**
** For copyright and disclaimer information see:
**      $(ASCIVIS_ROOT)/copyright_notice_1.txt
**
** 	or man llnl_copyright
**
** $Id: smBase.h,v 1.13 2009/05/19 02:52:19 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/


#ifndef _SM_BASE_H
#define _SM_BASE_H

#include "smBaseP.h"
#include "smdfc.h"
#include <boost/atomic.hpp>
//
// smBase.h - base class for "streamed movies"
//

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> 
#include <sys/types.h>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "../RC_cpp_lib/stringutil.h"
#ifndef WIN32
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#else
#define off64_t __int64
#endif

#include <string>
#include <vector>
#include <map>
using std::map;
#include <deque>
//#define DISABLE_PTHREADS 1
#ifndef DISABLE_PTHREADS
#include <pthread.h>
#else
#error PTHREADS DISABLED
#define pthread_cond_t char
#define pthread_mutex_t char
#endif 

#ifndef off64_t
#define off64_t int64_t
#endif

//#define SM_VERBOSE

// On top of the movie "type".  Room for 256 "types".
// Flags/masks are old-school -- use SM 3.0 tags moving forward for new tags.  
#define SM_FLAGS_MASK	0xffffff00
#define SM_TYPE_MASK	0x000000ff
#define SM_FLAGS_STEREO	0x00000100

#define SM_FLAGS_FPS_MASK	0xffc00000
#define SM_FLAGS_FPS_SHIFT	22
#define SM_FLAGS_FPS_BASE	20.0  /* hack! */

#define SM_MAGIC_VERSION1	0x0f1e2d3c
#define SM_MAGIC_VERSION2	0x0f1e2d3d
#define SM_MAGIC_VERSION3	0x0f1e2d3e 

/*!
  ===============================================
  debug print statements, using smdbprintf, which 
  output the file, line, time of execution for each statement. 
  Incredibly useful
*/ 

#define SM_VERBOSE 1
//#undef  SM_VERBOSE

#ifdef SM_VERBOSE 
#include <stdarg.h>
#include "../RC_cpp_lib/timer.h"
#include "../RC_cpp_lib/stringutil.h"
extern void sm_setVerbose(int level);
  

struct smMsgStruct {
  smMsgStruct(int l, string fi, string func) :
    line(l), file(fi), function(func) {
    return; 
  }
  int line; 
  string file, function; 
}; 

extern double gBaseTime; /* initialized to -1 in constructor */  
inline void sm_initTimer(void) {
  gBaseTime = timer::GetExactSeconds(); 
} 
extern int smVerbose;

#define SMPREAMBLE 

// NON-FORMATTED: (for the rare case where you want to print a string without interpreting it
#define smdbprint(args...)                                  \
  sm_real_dbprint( smMsgStruct(__LINE__,__FILE__,__FUNCTION__), args)   

inline void sm_real_dbprint(const smMsgStruct msg, int level, string s) {  
  if (smVerbose < level) return; 
  if (gBaseTime == -1) sm_initTimer(); 
  cerr << " SMDEBUG [" << msg.file << ":"<< msg.function << "(), line "<< msg.line << ", time=" << doubleToString(timer::GetExactSeconds() - gBaseTime, 3) << "]: "  << s;
  return; 
}

// FORMATTED: 
#define smdbprintf(args...)                                  \
  sm_real_dbprintf( smMsgStruct(__LINE__,__FILE__,__FUNCTION__), args)   

inline void sm_real_dbprintf(const smMsgStruct msg, int level, const char *fmt, ...) {  
  if (smVerbose < level) return; 
  if (gBaseTime == -1) sm_initTimer(); 
  cerr << " SMDEBUG [" << msg.file << ":"<< msg.function << "(), line "<< msg.line << ", time=" << doubleToString(timer::GetExactSeconds() - gBaseTime, 3) << "]: " ;
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr,fmt,ap);
  va_end(ap);
  //cerr << endl; 
  return; 
}

#else
inline void sm_real_dbprintf(int , const char * ...) {  
  return; 
}
#define smdbprintf if(0) sm_real_dbprintf
#endif



//===============================================
/* struct SM_MetaData */

//-----------------------------------------------------------------
// DO NOT CHANGE THESE VALUES!  IT WILL DESTROY DATA IF YOU DO
#define METADATA_MAGIC         0x0088BBeecc113399
//---------------------------
#define METADATA_TYPE_UNKNOWN  0x0
#define METADATA_TYPE_ERROR    0x1231111111111123
#define METADATA_TYPE_ASCII    0xA5C11A5C11A5C11A
#define METADATA_TYPE_DATE     0xDD00772255DD1177
#define METADATA_TYPE_DOUBLE   0xF10A7F10A7F10A7F
#define METADATA_TYPE_INT64    0x4244224442244442
//---------------------------
#define APPLY_ALL_TAG "Apply To All Movies [y/n]?"
#define USE_TEMPLATE_TAG "Use As Template [y/n]?"
//----------------------------------------------------------------

typedef map<string,struct SM_MetaData> TagMap; 

extern string payloadTypeToString(uint64_t pt);

/* SM Meta data contents on disk.  The intention is this will be read backward.  In this way, you seek to the end of the file, read the last 8 bytes to confirm a tag, then read preceding 1016 bytes to see what you have, then read the payload.  No table is needed.  
         (1) n bytes: <data payload: see below>
         (2) 8 bytes: uint64: payload length
         (3) 1014 bytes: <tag name: null terminated ASCII sequence> 
         (4) 8 bytes: uint64 : a binary signature constant 0x0088BBeecc113399

         supported payloads: 
         METADATA_TYPE_ASCII:  8 bytes: magic number 0xA5C11A5C11A5C11A
                               8 bytes: uint64_t, length of 0-terminated string
                               n bytes: 0-terminated ASCII string,   
         METADATA_TYPE_DATE:   8 bytes: magic number 0xDD00772255DD1177
                               8 bytes: uint64_t, length of 0-terminated string
                               n bytes: 0-terminated DATE formatted string,
         METADATA_TYPE_DOUBLE: 8 bytes: magic number 0xF10A7F10A7F10A7F
                               8 bytes: double (FP64)
         METADATA_TYPE_INT64:  8 bytes: magic number 0x4244224442244442
                               8 bytes: long long (int64_t)

*/ 
struct SM_MetaData {
  string mTag;
  uint64_t mType; 
  string mAscii;
  double mDouble; //assuming 8 bytes here; checked in Read and Write functions for safety
  int64_t mInt64; 
  
  static void Init(void);

  SM_MetaData():mType(METADATA_TYPE_UNKNOWN) {
    Init(); 
  }
  SM_MetaData(string tag): mTag(tag) {
    Init(); 
  }
  SM_MetaData(string tag, string s): mTag(tag), mType(METADATA_TYPE_ASCII), mAscii(s) {    
    Init(); 
  }
  SM_MetaData(string tag, int64_t i):mTag(tag), mType(METADATA_TYPE_INT64), mInt64(i) {
    Init(); 
  }
  SM_MetaData(string tag, double d): mTag(tag), mType(METADATA_TYPE_DOUBLE), mDouble(d) {   
    Init(); 
  }

  SM_MetaData(string tag, string mdtype, string s) {   
    Init(); 
    Set(tag,mdtype,s); 
    return;     
  }
    
  bool GetValue(string &outvalue) {
    if (mType == METADATA_TYPE_ASCII || mType == METADATA_TYPE_DATE) {
      outvalue = mAscii; 
      return true; 
    }
    smdbprintf(1, "SM_MetaData::GetValue: Error: tag %s is not ASCII or DATE.\n", mTag.c_str()); 
    return false; 
  }
  bool GetValue(double &outvalue) {
    if (mType == METADATA_TYPE_DOUBLE) {
      outvalue = mDouble; 
      return true; 
    }
    smdbprintf(1, "SM_MetaData::GetValue: Error: tag %s is not DOUBLE.\n", mTag.c_str()); 
    return false; 
  }

  bool GetValue(int64_t &outvalue) {
    if (mType == METADATA_TYPE_INT64) {
      outvalue = mInt64; 
      return true; 
    }
    smdbprintf(1, "SM_MetaData::GetValue: Error: tag %s is not INT64.\n", mTag.c_str()); 
    return false; 
  }
        
  
  bool Set(string tag, string mdtype, string s) {  
    smdbprintf(5, "Set(%s, %s, %s)\n", tag.c_str(), mdtype.c_str(), s.c_str());
    mTag = tag; 
    try {
      if (mdtype == "INT64") {
        boost::trim(s);
        SetValue(boost::lexical_cast<int64_t>(s)); 
      } else if  (mdtype == "DOUBLE") {
        boost::trim(s);
        SetValue(boost::lexical_cast<double>(s)); 
      } else if (mdtype == "ASCII") {
        mType = METADATA_TYPE_ASCII; 
        SetValue(s); 
      } else if (mdtype == "DATE") {
        return SetDate(s); 
      } else {
        mType = METADATA_TYPE_UNKNOWN; 
        return false; 
      }
    } catch (...) {
        mType = METADATA_TYPE_UNKNOWN; 
        mAscii = "BAD_VALUE"; 
        return false; 
    }
     
    return true;     
  }

  bool Set(string tag, uint64_t mdtype, string s) {   
    mTag = tag; 
    mType = mdtype;
    return Set(mTag, TypeAsString(), s); 
  }

  bool SetDate(string s) {
    bool retval = true; 
    if (s != "") {
      s = GetStandardTimeStringFromString(s);
    }
    if (s == INVALID_TIME_STRING) {
      mType = METADATA_TYPE_UNKNOWN;
      retval = false; 
    } else {
      mType = METADATA_TYPE_DATE;
    }
    mAscii = s; 
    return retval; 
  } 
   
  void SetValue(string s) {
    if (mType == METADATA_TYPE_DATE) {
      SetDate(s); 
      return; 
    }
    mType = METADATA_TYPE_ASCII;
    mAscii = s; 
    return; 
  } 
   
  void SetValue(double d) {
    mType = METADATA_TYPE_DOUBLE;
    mDouble = d; 
    return;
  } 
   
  void SetValue(int64_t i) {
    mType = METADATA_TYPE_INT64;
    mInt64 = i; 
  } 
  

  static void WriteJsonError(ostream *s, string &filename); 
  /*!
    Accepts  "tag:value[:type]" where type defaults to ASCII. 
    Automatically recognizes canonical tags and enforces proper type. 
  */ 
  void SetFromDelimitedString(string s);

  static void SetDelimiter(string s) {
    mDelimiter = s; 
  }
        
  bool operator == (const SM_MetaData&other) const {
    return (other.mTag == mTag); 
  }
  string toString(void)  const ; 

  string toShortString(string label="", int typeLength = 0, int tagLength = 0) const {
    string formatString; 
    if (label != "") {
      formatString = str(boost::format("%1$12s")%label);
    }
    if (!typeLength) typeLength = TypeAsString().size() + 2; 
    if (!tagLength) tagLength = mTag.size() + 4; 
    formatString += str(boost::format("(%%2$%1%s) %%1$-%2%s = ") % typeLength % tagLength );
    string quote; 
    if (mType == METADATA_TYPE_ASCII || mType == METADATA_TYPE_DATE) {
      quote = "\""; 
    }
    return str(boost::format(formatString + "%4%%3%%4%") % mTag % TypeAsString() % ValueAsString() % quote);
  }
  string TypeAsString(void) const ;
  string ValueAsString(void) const ; 
  bool Write(int filedescr) const ; 
  off64_t Read(int filedescr); // read backward from current point in file, leave file ready for another read
  

  // ----------------------------------------------------------
  static bool mInitialized; 
  static vector<SM_MetaData> CanonicalMetaData(bool includePrompts) { 
    Init(); 
    vector<SM_MetaData> mdata = mCanonicalMetaData; 
     
    if (!includePrompts) {
      mdata.pop_back();
      mdata.pop_back();
    }
    return mdata;
  }
  /*!
    Utility function for getting username info on Unix. 
    Useful for some canonical tags.
  */ 
  static map<string,string> GetUserInfo(void) ;

  /*!
    Given a tag, return "ASCII", "DOUBLE", "INT64" "DATE", or "UNKNOWN"
  */ 
  static string GetCanonicalTagType(string tag);
  static TagMap MergeMetaData(TagMap &target, const TagMap source) {
    Init(); 
    for (TagMap::const_iterator pos = source.begin(); pos != source.end(); pos++) {
      target[pos->first] = pos->second;
    }
    return target; 
  }

  static TagMap CanonicalMetaDataAsMap(bool includePrompts);
  static bool GetMetaDataFromFile(string metadatafile, TagMap &metadatavec);
  static bool  WriteMetaDataToStream(ostream *ofile, TagMap &metadatavec);
  static string CanonicalOrderMetaDataSummary( TagMap metadatavalues, bool withnums, bool promptForReuse);
  static string MetaDataSummary(const TagMap metadatavalues, bool withnums=false);
  static TagMap GetCanonicalMetaDataValuesFromUser(TagMap &previousMap, bool usePrevious, bool promptForReuse);
  // ----------------------------------------------------------
  
private:
  static string mDelimiter; // for separating tag:value[:type] strings
  static vector<SM_MetaData> mCanonicalMetaData; 
};
  
//===============================================
struct TileInfo {  
  std::string toString(void); 
  u_int tileNum;
  u_int overlaps;  /* newly overlaping data */
  u_int cached; /* overlap data from a prior read -- cached */
  u_int blockOffsetX; /* where to start writing row data in uber block */
  u_int blockOffsetY; /* likewise for column data */
  u_int tileOffsetX; /* offset into current tile to grab data */
  u_int tileOffsetY;
  u_int tileLengthX;  /* length of mem copy */
  u_int tileLengthY;
  uint32_t fileOffset; /* offset from start of frame, including header */ 
  u_int readBufferOffset; /* where to find this tile in buffer */
  u_int compressedSize; 
  u_int skipCorruptFlag;
} ;

struct smThreadData {
  smThreadData(): fd(0),currentFrame(-1) {}
  ~smThreadData() {}
  int fd; 
  uint32_t currentFrame ; // prevent duplicate reads of same frame
  std::vector<u_char> io_buf;  // for reading chunks from files
  std::vector<u_char> tile_buf; // for reading tiles; 
  std::vector<TileInfo> tile_infos; // used for computing overlap info
};
  
/*!
  A struct to hold info for compressing a frame to avoid recomputing stuff
*/ 
struct FrameCompressionWork {
  FrameCompressionWork(int inFrame, u_char *data)  {
    mFrame = inFrame; 
    mUncompressed = data; 
  }
  ~FrameCompressionWork() {
    clear(); 
  }
  
  void clear(void) {
    int  pos = mCompressed.size(); 
    smdbprintf(5, "deleting %d compressed image buffers for frame %d\n", 
               pos, mFrame);
    
    while (pos--) {
      if (mCompressed[pos]) {
        smdbprintf(5, "deleting compressed image %d buffer %p for frame %d\n", 
                   pos, mCompressed[pos], mFrame);
        delete mCompressed[pos]; 
        mCompressed[pos]= NULL;
      }
    }
    smdbprintf(5, "deleting uncompressed image buffer %p for frame %d\n", 
               mUncompressed, mFrame);
    if (mUncompressed) {
      delete mUncompressed; 
      mUncompressed = NULL; 
    }
    mCompressed.clear(); 
    mCompTileSizes.clear(); 
    mCompFrameSizes.clear(); 
  }
  string toString(void ) {
    string mystring = string("<< FrameCompressionWork: mFrame = ") + intToString(mFrame); 
    int numsizes=mCompFrameSizes.size(), sizenum = 0; 
    if (numsizes) {
      mystring += ", mCompFrameSizes = ("; 
      while (sizenum < numsizes-1) {
        mystring += intToString(mCompFrameSizes[sizenum]) + ","; 
        ++ sizenum;
      }
      mystring += intToString(mCompFrameSizes[sizenum]) + ")"; 
    } else {
      mystring += ", no compressed frames found";
    }
    if (mCompTileSizes.size()) {
      mystring += ", tiles per resolution: "; 
      uint32_t resnum = 0; 
      while (resnum < mCompTileSizes.size()) {
        int numtiles = mCompTileSizes[resnum].size();
        uint32_t totalTilesize = 0; 
        mystring += "<<resolution " + intToString(resnum) + ", " + intToString(numtiles) + "tiles"; 
        if (numtiles) { 
          mystring += ", tilesizes: ("; 
          int tilenum = 0; 
          while (tilenum < numtiles-1) {
            mystring += intToString(mCompTileSizes[resnum][tilenum]) + ",";
            totalTilesize += mCompTileSizes[resnum][tilenum];
           ++tilenum; 
          }
          totalTilesize += mCompTileSizes[resnum][tilenum];
          mystring += intToString(mCompTileSizes[resnum][tilenum]) + "), total =  " + intToString(totalTilesize) + ">>";
          if (resnum < mCompTileSizes.size()-1) 
            mystring += ", ";
        }      
        ++ resnum;          
      }
    } else {
      mystring += ", no tiles detected"; 
    }
    return mystring;
  }      
  int mFrame;
  vector< vector<int> > mCompTileSizes; //sizes of the compressed tiles, for each resolution
  vector<int> mCompFrameSizes; // total size of compressed frame at each resolution
  u_char *mUncompressed;
  vector<u_char *>mCompressed; // Buffers of compressed frames, one for each resolution level
}; 

/*!
  Helps organize read buffering
*/ 
struct InputBuffer {
  int mLOD; 
  int32_t mFirstFrame, mNumFrames, mBytesPerFrame; 
  vector<int64_t> mRawOffsets; 
  boost::shared_ptr<char> mRawBytes; 
}; 

/*!
  Helps organize buffering for I/O. 
*/ 
struct OutputBuffer {
  OutputBuffer(uint32_t buffSize): mRequiredWriteBufferSize(0),
                  mFirstFrameNum(0), mNumFrames(0) {
    mFrameBuffer.resize(buffSize); 
    return; 
  }
  bool full(void) {
    return (mNumFrames >= mFrameBuffer.size()); 
  } 

  void resize(uint32_t frames) {
    smdbprintf(3, "OutputBuffer::resize() Resizing frame buffer to %ld frames\n", frames); 
    mFrameBuffer.resize(frames); 
  }

  bool empty(void) { 
    return (mNumFrames == 0); 
  }
  
  bool addFrame(FrameCompressionWork* frame){
    if (frame->mFrame < mFirstFrameNum) {
      smdbprintf(0, "Bad thing: addFrame called on frame %d, which is before first allowable frame number %d\n", frame->mFrame, mFirstFrameNum);
      abort(); 
    }
      
    uint32_t slotnum = frame->mFrame - mFirstFrameNum;
    if (full() || slotnum >= mFrameBuffer.size()) {
      smdbprintf(2, "Warning: Attempted to place frame %d into slot %d failed!  (full == %d)\n", frame->mFrame, slotnum, int(full()));
      return false;
    }
    if (mFrameBuffer[slotnum]) {
      smdbprintf(0, "Bad thing:  placing frame %d in an occupied slot!\n", frame->mFrame);
      abort();
    }
    mRequiredWriteBufferSize += frame->mCompFrameSizes[0] + frame->mCompTileSizes[0].size()*sizeof(uint32_t); 
    mFrameBuffer[slotnum] = frame; 
    if (!frame) {
        smdbprintf(0, "NULL frame %d being added in addFrame()\n", frame->mFrame);
        abort();
    }
    smdbprintf(5, "Frame %d added in slot %d\n", frame->mFrame, slotnum);
    mNumFrames.fetch_add(1, boost::memory_order_acq_rel);
    return true; 
  }
  
  
  vector<FrameCompressionWork*> mFrameBuffer; // stored as work quanta until actually rewritten; then marshalled into mWriteBuffer for efficient I/O 
  boost::atomic<int64_t> mRequiredWriteBufferSize; // computed as frames are buffered
  int32_t mFirstFrameNum;  
  boost::atomic<uint32_t> mNumFrames;  // number of entries in mDataBuffer -- can't use size() because elements are placed out of order, and resize() is called only once.
}; 

// =========================================================================
/*!
  SMbASE base class for streaming movies
*/ 
// =========================================================================
class smBase {
 public:
  smBase(int mode, const char *fname, int numthreads=1, uint32_t bufferSize=50) {
    init(mode, fname, numthreads, bufferSize); 
  }

  smBase(const char *fname, u_int w, u_int h,
         u_int nframes, u_int *tilesizes = NULL, u_int nres=1, 
         int numthreads=1); 

  void init(int mode, const char *fname, int numthreads=1, uint32_t bufferSize=50);
    // create a new movie for writing
  /* int newFile(const char *fname, u_int w, u_int h,
     u_int nframes, u_int *tilesizes = NULL, u_int nres=1, int numthreads=1);*/

  virtual ~smBase();
  
  /// set size of output buffers. 
  void setBufferSize(uint32_t frames);

  bool fillBuffers(void); 
  void readThread(void); 
  void startReadThread(void); 
  void stopReadThread(void); 

  // get the decompressed image 
  uint32_t getFrame (int frame, void *out, int threadnum, int res=0); // for threads
  
  // get part of an image and decompress it appropriately
  uint32_t getFrameBlock(int frame, void *out, int threadnum, 
                     int destRowStride=0,
                     int *dim = NULL, 
                     int *pos = NULL, int *step = NULL,int res = 0);
  
  void getCompFrame(int frame, int threadnum, void *out, int &outsize,int res = 0);
  int getCompFrameSize(int frame,int res = 0);
  
  // set the frame image, either uncompressed or compressed data
  void compressAndWriteFrame(int frame, u_char *data);
  //void setCompFrame(int frame, void *data, int size,int res = 0);
  void writeCompFrame(int frame, void *data, int *sizes,int res = 0);

  // for multithreaded case, this allows buffering into a queue.  This will hang if a frame is ever skipped!  
  // void bufferFrame(int frame,unsigned char *data, bool oktowrite);
  void writeThread(void); 
  void startWriteThread(void); 
  void stopWriteThread(void); 

  void combineResolutionFiles(void);
  bool flushFrames(bool force); 
  void compressAndBufferFrame(int frame,u_char *data);
  //void flushFrames(void); 

  // Tile based version follows
  int computeTileSizes(FrameCompressionWork *wrk, int resolution);
  void compressFrame(FrameCompressionWork *wrk);

  // convert an image into its compressed form, tiled 
  int compFrame(void *in, void *out, int *outsizes,int res = 0);
  
#ifdef WIN32
  static void __cdecl init(void);
#else
  static void init(void);
#endif
  
  string getName(void) { return mMovieName; }
  u_int getWidth(int res=0)  { return(framesizes[res][0]); }
  u_int getHeight(int res=0) { return(framesizes[res][1]); }
  u_int getNumFrames() { return(mNumFrames); }
  u_int getNumResolutions() { return(mNumResolutions); }
  u_int getTileWidth(int res=0) { return(tilesizes[res][0]); }
  u_int getTileHeight(int res=0) { return(tilesizes[res][1]); }
  u_int getTileNx(int res=0) { return(tileNxNy[res][0]); }
  u_int getTileNy(int res=0) { return(tileNxNy[res][1]); }
  uint32_t getNumTiles(int res) { return getTileNx(res)*getTileNx(res); }
  u_int getMaxNumTiles() { return(maxNumTiles);}
  
  int Min(int a,int b) { return((a > b) ? b : a); }
  
  // used in img2sm and smquery to print frame rate and other movie details
  string InfoString(bool verbose) ; 
  void printFrameDetails(FILE *fp, int f, int res);
  
  // open a movie
#ifdef WIN32
  static smBase * __cdecl openFile(const char *fname,  int mode, int numthreads);
#else
  static smBase *openFile(const char *fname, int mode, int numthreads);
#endif

  void DeleteMetaData(void) {
    mMetaData.clear(); 
    return; 
  }
  void DeleteMetaData(string tag) {
    mMetaData.erase(tag); 
  }

  // Add the tag/value pairs from tagvec. 
  //void AddTagValue(string tag, string mdtype, int64_t value); 

  void setPosterFrame(int64_t f){
    SetMetaData("SM__posterframe", f); 
    smdbprintf(1, str(boost::format("Setting poster frame to %1%.\n")%f).c_str()); 
  }
  int64_t getPosterFrame(void) {
    int64_t frame = -1; 
    if (mMetaData.find("SM__posterframe") != mMetaData.end()) {
      frame = mMetaData["SM__posterframe"].mInt64;
    }
    if (frame == -1 && mNumFrames > 0) {
      frame = mNumFrames/2; 
      SetMetaData("SM__posterframe", frame); 
    }
    return frame;
  }

  void ExportPosterFrame(void);

  void SetMetaDataFromDelimitedString(string s) {
    SM_MetaData md;
    md.SetFromDelimitedString(s); 
    SetMetaData(md); 
  }
  void SetMetaData(const SM_MetaData &md);
  void SetMetaData(const TagMap mdmap, string tagPrefix="");
  void SetMetaData(vector<SM_MetaData> &mdvec);
  void SetMetaData(string commandLine, string tagfile, bool canonical, string delimiter, vector<string> taglist, int poster, bool exportTagfile, bool quiet );

  template <class T> 
    void SetMetaData(const string tag, const T &value) {
    SM_MetaData md(tag,value); 
    SetMetaData(md); 
    return; 
  }

  TagMap GetMetaData(void) { return mMetaData; }
   
  // ===========================================================
  /* Do not change outvalue unless data is found.  Allows simpler code in caller */ 
  bool GetMetaData(string tag, string &outvalue) {
    if (mMetaData.find(tag) == mMetaData.end()) {    
      smdbprintf(1, "smBase::GetMetaData: Warning: tag %s not found.\n", tag.c_str()); 
      return false; 
    }     
    return mMetaData[tag].GetValue(outvalue); 
  }

  // ===========================================================
  /* Do not change outvalue unless data is found.  Allows simpler code in caller */ 
  bool GetMetaData(string tag, double &outvalue) {
    if (mMetaData.find(tag) == mMetaData.end()) {    
      smdbprintf(1, "smBase::GetMetaData: Warning: tag %s not found.\n", tag.c_str()); 
      return false; 
    }     
    return mMetaData[tag].GetValue(outvalue); 
  }

  // ===========================================================
  /* Do not change outvalue unless data is found.  Allows simpler code in caller */ 
  bool GetMetaData(string tag, int64_t &outvalue) {
    if (mMetaData.find(tag) == mMetaData.end()) {    
      smdbprintf(1, "smBase::GetMetaData: Warning: tag %s not found.\n", tag.c_str()); 
      return false; 
    }     
    return mMetaData[tag].GetValue(outvalue); 
  }

  // ===========================================================
  string MetaDataAsString(string label=""); 

  // ===========================================================
  void WriteMetaData(void);

  // ===========================================================
  bool ExportMetaData(ostream *ofile) {  
    if (mMetaData.size()) {
      return SM_MetaData::WriteMetaDataToStream(ofile, mMetaData); 
    }
    return false; 
  }

  // ===========================================================
  bool ImportMetaData(string filename) {
    if (mMetaData.size() ) {
      smdbprintf(2, "Importing metadata from file %s\n", filename.c_str()); 
      return SM_MetaData::GetMetaDataFromFile(filename, mMetaData); 
    } 
    return false; 
  }

  
  // ===========================================================
  // close a movie
  void closeFile(void);
  void deleteFile(void) { 
    cerr << "Deleting file " << mMovieName << endl; 
    unlink(mMovieName);
  }
  
  // various flag/type info
  int getType(void) { return mTypeID;}
  u_int getFlags(void) { return(flags); };
  void setStereo(void) { flags |= SM_FLAGS_STEREO; }
  void setFlags(u_int f) { flags = f; };
  float getFPS(void) { 
    int i = (flags & SM_FLAGS_FPS_MASK) >> SM_FLAGS_FPS_SHIFT;
    if (i == 0) return(30.0);
    return ((float)i)/SM_FLAGS_FPS_BASE;
  };
  void setFPS(float fps) {
    u_int i = (u_int)(fps * SM_FLAGS_FPS_BASE);
    i = (i << SM_FLAGS_FPS_SHIFT) & SM_FLAGS_FPS_MASK;
    flags = (flags & ~SM_FLAGS_FPS_MASK) | i;
  };
  int getVersion(void) { return(mVersion); };
  
  void computeTileOverlap(int *blockDim, int* blockPos, int res, int thread);

  // metadata
  TagMap mMetaData; 

  bool haveError(void) { return mErrorState != 0; }
  string errorMessage(void) { return mErrorMessage; }
  // Set error state to true.  msg is not necessarily echoed. 
  void SetError(string msg) {
    if (!mErrorState) {
      mErrorState = 1; 
      mErrorMessage = msg; 
      cerr << msg << endl; 
    }
    return; 
  }

 protected:
  
  // internal functions to compress or decompress a rectangle of pixels
  // most subclasses need only replace these...
  virtual void compBlock(void *in, void *out, int &outsize, int *dim) = 0;
  virtual bool decompBlock(u_char *in,u_char *out,int insize, int *dim) = 0;
  
  
  
 protected:
  int mTypeID; // for an instance.  
  int mErrorState; // 0 = none, -1 = reported, 1 = unreported
  string mErrorMessage; // for reference -- the first occurring message.  

 private:
  void readHeader(void);
  void initWin(void);
  uint32_t readData(int fd, u_char *buf, int bytes);
  uint32_t readFrame(u_int frame, int threadnum);
  uint32_t readTiledFrame(u_int frame, int*dimensions, int*position, int resolution, int threadnum);
  
  vector<u_char>mWriteBuffer; // for use in marshalling data and writing
  int mNumThreads; 
  // Flags on top of the filetype...
  u_int flags;
  
  // number of frames in the movie
  u_int mNumFrames;
  u_int mNumResolutions;
  
  // image size
  u_int framesizes[8][2]; // 8 is the limit on LOD apparently. 
  // dimensions of the tiles...
  u_int tilesizes[8][2];
  u_int tileNxNy[8][2];
  u_int maxNumTiles;
  
  // 64-bit offset of each compressed frame
  vector<off64_t>mFrameOffsets;

  // 32-bit length of each compressed frame
  vector<unsigned int>mFrameLengths;
  
  // "mod" flag
  int bModFile;
  
  // version
  int mVersion;
    
  //path to movie file
  char *mMovieName;
  
  
  vector<int> mResFDs; // for mipmap files written in "parallel" 
  vector<string> mResFileNames; //  they have to be written to, then read
  vector<uint64_t> mResFileBytes; // size of the tmp files
 /*!
    Per-thread data structures, to ensure thread safety.  Every function must now specify a thread number. 
  */ 
  std::vector<smThreadData> mThreadData; 
  // staging buffer is for worker threads to place work to
  OutputBuffer *mOutputBuffers[2]; 
  OutputBuffer *mStagingBuffer, 
  // output buffer is for I/O thread to write to disk
    *mOutputBuffer; 
  pthread_mutex_t mBufferMutex; 

  pthread_t mWriteThread; 
  bool mWriteThreadRunning, mWriteThreadStopSignal; 

  // Information about reading requests, etc.  
  int32_t mRenderFrame;  // this determines read and decompression priorities 
  pthread_t mReadThread; 
  bool mReadThreadRunning, mReadThreadStopSignal; 

  // directory of movie types
  static u_int ntypes;
  static u_int *typeID;
  static smBase *(**smcreate)(const char *, int);
  
};

#endif
