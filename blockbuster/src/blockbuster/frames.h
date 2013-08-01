#ifndef BLOCKBUSTER_IMAGES_H
#define BLOCKBUSTER_IMAGES_H

#define MAX_IMAGE_LEVELS 10
#include "common.h"
#include <vector>
#include "boost/shared_ptr.hpp"
#include <QStringList>
using namespace std; 
struct Canvas; 

typedef boost::shared_ptr<struct FrameInfo> FrameInfoPtr; 
typedef boost::shared_ptr<struct TextureObject> TextureObjectPtr;

//============================================================
struct ImageFormat{
  ImageFormat(): bytesPerPixel(0), scanlineByteMultiple(0), 
                 byteOrder(0), rowOrder(0), 
                 redShift(0), greenShift(0), blueShift(0), 
                 redMask(0), greenMask(0), blueMask(0) {}
  ImageFormat(int bpp, int sbm, int bo, int ro, 
              int rs, int gs, int bs, long rm, long gm, long bm):
    bytesPerPixel(bpp), scanlineByteMultiple(sbm), 
    byteOrder(bo), rowOrder(ro), 
    redShift(rs), greenShift(gs), blueShift(bs),
    redMask(rm), greenMask(gm), blueMask(bm) {}
  int bytesPerPixel;
  int scanlineByteMultiple;
  /* byteOrder is either LSB_FIRST or MSB_FIRST.
   * If the image format is a 2-byte format, this
   * indicates the order in which the two bytes
   * are placed into image memory.
   * If the image format is a 3-byte or greater
   * format, MSB_FIRST indicates RGB data, and
   * LSB_FIRST indicates BGR data.
   */
  int byteOrder;
  /* rowOrder is either TOP_TO_BOTTOM or BOTTOM_TO_TOP, based
   * on whether the first row of pixels returned corresponds
   * to the top of the image or to the bottom of the image.
   * X11 renderers prefer TOP_TO_BOTTOM order, for example,
   * while OpenGL renderers prefer BOTTOM_TO_TOP.
   */
  int rowOrder;
  
  /* If bytesPerPixel is 3 or 4, the following are ignored.
   * If bytesPerPixel is 2, these values show how to pack
   * the byte values into a pixel value, and how to then
   * pack the pixel into memory.  The calculation goes:
   * 
   *   pixel = ((red << redShift) & redMask) |
   *           ((green << greenShift) & greenMask) |
   *           ((blue << blueShift) & blueMask);
   *
   * Note that for typical 16-bit formats, blueShift is negative
   * (and should be implemented as a right shift); but in general,
   * any of the shifts could be negative.
   *
   * The pixel, once calculated, is stuffed into memory
   * based on byteOrder.  If the byteOrder is LSB_FIRST,
   * the low-order byte is placed into lower memory.  If
   * it is MSB_FIRST, the high-order byte is placed into
   * lower memory.
   */
  int redShift, greenShift, blueShift;
  unsigned long redMask, greenMask, blueMask;
} ;


// ============================================================
struct Image {
  Image(): width(0), height(0), levelOfDetail(0), 
           frameNumber(0), mTypeName("UNKNOWN"), imageData(NULL) {
    return; 
  }
   
  virtual ~Image() {
    return; 
  }
  
  // this will be the Image API
  virtual int LoadImage(FrameInfoPtr /*frameInfo*/,
                ImageFormat */*requiredImageFormat*/, 
                const Rectangle */*region*/,
                int /*levelOfDetail */ ) {
    return 0; 
  }
  
  bool allocate(uint32_t bytes) {
    if (bytes > imageData.size()) {
      imageData.resize(bytes); 
    }
    return (imageData.size() >= bytes); 
  }

  const void *ConstData(void) const {
    return (void*)&imageData[0]; 
  }

  void *Data(void) {
    return (void*)&imageData[0]; 
  }

  uint32_t DataSize(void) {
    return imageData.size(); 
  }

  uint32_t width, height;
  ImageFormat imageFormat;
  Rectangle loadedRegion;
  int levelOfDetail;
  uint32_t frameNumber; // if appropriate
  string mTypeName ; // e.g. "SM" or "PNG"  
  private: 
  vector<char> imageData; /* the actual image data to display */
} ;

//============================================================
/* A typedef here makes things simpler for frame list routines
 * that can return different image load functions.
 */

typedef int (*LoadImageFunc)
  ( Image *image,
    FrameInfo*frameInfo,
    ImageFormat *requiredImageFormat, 
    const Rectangle *region,
    int levelOfDetail
    );



//============================================================
/* Information about one frame of the movie. */ 
struct FrameInfo {
  FrameInfo(): width(0), height(0), depth(0), maxLOD(0), 
               mFrameNumberInFile(0) /*,  enable(0) */{
    return; 
  }
  
  
  // NEW VERSION 
  FrameInfo(int w, int h, int d, int maxlod, string fname, uint32_t frame):
    width(w), height(h), depth(d), maxLOD(maxlod), filename(fname), 
    mFrameNumberInFile(frame)/*, enable(en)*/ {
    return; 
  }
  
  FrameInfo(int w, int h, int d, int lod, string fname, 
            int /*en*/,  LoadImageFunc lif):
    width(w), height(h), depth(d), maxLOD(lod), filename(fname), 
    mFrameNumberInFile(0)/*, enable(en)*/,  LoadImageFunPtr(lif) {
    return; 
  }
  
  virtual ~FrameInfo() {
    return; 
  }

  QString toString(void) {
    return QString("{ FrameInfo: frameNumber = %1 in file %2}").arg(mFrameNumberInFile).arg(filename.c_str()); 
  }

  virtual int LoadImage(ImageFormat */*requiredImageFormat*/, 
                        const Rectangle */*region*/,
                        int /*levelOfDetail*/) {
    cerr << "This needs to be made virtual" << endl; 
    return 0;
  }

  
  Image *LoadAndConvertImage(unsigned int frameNumber,
                             ImageFormat *canvasFormat, 
                             const Rectangle *region, int levelOfDetail);
  
  /* Basic statistics */
  int width, height, depth;
  
  int maxLOD; /* 0 if LOD not supported */
  
  /* Associated file */
  string filename;
  
  /* If there is more than one frame in a single file, the format
   * driver can use this integer to distinguish them.
   */
  int mFrameNumberInFile;

  /* Pointer to frame data that's specific to the canvas */
  TextureObjectPtr mTextureObject;
  
  /* A flag that we can use to disable frames that have errors */
  // int enable;
  
  Image *mImage; 

  /* This routine is called to pull the frame contents out
   * of a file.  The file format module can pull out only the
   * desired subimage, or it might pull out the whole thing;
   * if it can switch to the desired format, it should, but
   * it can return any format it wishes (or can).
   * 
   * The returned image must be released with delete
   *
  */
  /* int LoadImage(Image *image,
		struct FrameInfoPtr frameInfo,
		struct Canvas *canvas,
		const Rectangle *region,
		int levelOfDetail
		);
  */
  LoadImageFunc LoadImageFunPtr;
  
} ;


//============================================================

  /* A FrameList contains an array of frame pointers and associated metadata.  
   * This structure basically represents a movie.
   */
struct FrameList {
  // ----------------------------------------------------
  FrameList() { 
    init(); 
  }

  // ----------------------------------------------------
  FrameList(FrameInfoPtr fi) {
    init(); 
    append(fi); 
    return; 
  }

  // ----------------------------------------------------
  FrameList(int numfiles, char **files) {
    init(); 
    char **fp = files; 
    QStringList filenames; 
    while (numfiles--) {
      filenames.append(*fp); 
      ++fp; 
    }
    LoadFrames(filenames); 
    return; 
  }

   // ----------------------------------------------------
 void init(void) {
    stereo = false; 
    targetFPS = 30.0;
  }

  // ----------------------------------------------------
  void DeleteFrames(void) {
    frames.clear(); 
  }

  // ----------------------------------------------------
  uint32_t numStereoFrames(void) const {
    if (stereo) return frames.size()/2; 
    return frames.size(); 
  }

  // ----------------------------------------------------
  uint32_t numActualFrames(void) const { return frames.size(); }

  // ----------------------------------------------------
  FrameInfoPtr getFrame(uint32_t num) const {
     if (num < frames.size()){
      return frames[num];
    }
    return FrameInfoPtr(); 
  }
  // ----------------------------------------------------
  void GetInfo(int &maxWidth, int &maxHeight, int &maxDepth,
		    int &maxLOD, float &fps);

  // ----------------------------------------------------
  void append(FrameList *other) {
    for  (uint32_t i=0; i < other->frames.size(); i++) {
      frames.push_back(other->frames[i]); 
    }  
    return; 
  }

  // ----------------------------------------------------
  void append(FrameInfoPtr frame) {
    frames.push_back(frame); 
    return; 
  }
  
  // ----------------------------------------------------
  bool LoadFrames(QStringList &files);

  // ----------------------------------------------------
  bool stereo;
  float targetFPS;      /* desired/target frames/second playback rate */
  QString formatName, formatDescription; 
  private:
  vector<FrameInfoPtr> frames; 
} ;



#endif
