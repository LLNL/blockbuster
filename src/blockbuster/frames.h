#ifndef BLOCKBUSTER_IMAGES_H
#define BLOCKBUSTER_IMAGES_H

#define MAX_IMAGE_LEVELS 10
#include "common.h"
#include <vector>
#include <QStringList>

using namespace std; 
struct Canvas; 


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

struct Image {
  Image():width(0), height(0),  levelOfDetail(0), 
          frameNumber(0), imageDataBytes(0), imageData(NULL), 
          mManageData(false){}
  Image(uint32_t w, uint32_t h, 
        ImageFormat &form, Rectangle &region, int lod, uint32_t frame, 
        unsigned int idb, void *data, bool manageData=false) :
    width(w), height(h), imageFormat(form), loadedRegion(region), 
    levelOfDetail(lod), 
    frameNumber(frame), imageDataBytes(idb), imageData(data), 
    mManageData(manageData) {}

  ~Image() {
    if (mManageData && imageData) free(imageData); 
    return; 
  }
  
    
  uint32_t width, height;
  ImageFormat imageFormat;
  Rectangle loadedRegion;
  int levelOfDetail;
  uint32_t frameNumber; // if appropriate
  unsigned int imageDataBytes;
  void *imageData;      /* the actual image data */
  bool mManageData; // if true, then imageData will be free()'d during destructor
  
} ;

//============================================================
/* A typedef here makes things simpler for frame list routines
 * that can return different image load functions.
 */

typedef int (*LoadImageFunc)
  ( Image *image,
    struct FrameInfo *frameInfo,
    ImageFormat *requiredImageFormat, 
    const Rectangle *region,
    int levelOfDetail
    );

typedef  void (*DestroyFrameFunc)(struct FrameInfo *frameInfo);


//============================================================
struct FrameInfo {
  FrameInfo(): width(0), height(0), depth(0), maxLOD(0), filename(NULL), 
               frameNumber(0), privateData(NULL), canvasPrivate(NULL), 
               enable(0), LoadImage(NULL), DestroyFrameInfo(NULL) {}
  FrameInfo(int w, int h, int d, int lod, char *fname, int fnum, 
            void *priv, void *cp, int en, 
            LoadImageFunc lif, DestroyFrameFunc dff):
    width(w), height(h), depth(d), maxLOD(lod), filename(fname), 
    frameNumber(fnum), privateData(priv), canvasPrivate(cp), enable(en), 
    LoadImage(lif), DestroyFrameInfo(dff) {}
            
  ~FrameInfo() {}

  QString toString(void) {
    return QString("{ FrameInfo: frameNumber = %1 }").arg(frameNumber); 
  }
  /* Basic statistics */
  int width, height, depth;
  
  int maxLOD; /* 0 if LOD not supported */
  
  /* Associated file */
  char *filename;
  
  /* If there is more than one frame in a single file, the format
   * driver can use this integer to distinguish them.
   */
  int frameNumber;
  
  /* If the image loader has anything else it wants to store for the
   * frame (file offsets, image information, etc.) it goes here.
   * This is very poorly named -- it's not private at all in the classic
   * sense of the word.  In fact, all of this "object oriented" code is 
   * completely bogus.  
   */
  void *privateData;
  
  /* Pointer to frame data that's specific to the canvas */
  void *canvasPrivate;
  
  /* A flag that we can use to disable frames that have errors */
  int enable;
  
  /* This routine is called to pull the frame contents out
   * of a file.  The file format module can pull out only the
   * desired subimage, or it might pull out the whole thing;
   * if it can switch to the desired format, it should, but
   * it can return any format it wishes (or can).
   * 
   * The returned image can be released with free()
   *
  */
  /* int LoadImage(Image *image,
		struct FrameInfo *frameInfo,
		struct Canvas *canvas,
		const Rectangle *region,
		int levelOfDetail
		);
  */
  LoadImageFunc LoadImage;
  
  void (*DestroyFrameInfo)(struct FrameInfo *frameInfo);
} ;


//============================================================

  /* A FrameList is an array of frames and associated metadata.  
   * The list itself is always released
   * with free(), although each of the frames on the inside must first be
   * released with the appropriate (*DestroyFrameInfo)() call.
   * This structure basically represents a movie.
   */
struct FrameList {
  FrameList() { init(); }
  FrameList(FrameInfo* fi) {
    init(); 
    append(fi); 
    return; 
  }

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

  void init(void) {
    stereo = false; 
    targetFPS = 30.0;
  }

  void DeleteFrames(void); 

  uint32_t numStereoFrames(void) const {
    if (stereo) return frames.size()/2; 
    return frames.size(); 
  }

  uint32_t numActualFrames(void) const { return frames.size(); }

  FrameInfo * getFrame(uint32_t num) const {
    if (num < frames.size()){
      return frames[num];
    }
    return NULL; 
  }
  void GetInfo(int &maxWidth, int &maxHeight, int &maxDepth,
		    int &maxLOD, float &fps);

  void append(FrameList *other) ; 

  void append(FrameInfo *frame) {
    frames.push_back(frame); 
    return; 
  }
  
  bool LoadFrames(QStringList &files);

  bool stereo;
  float targetFPS;      /* desired/target frames/second playback rate */
  QString formatName, formatDescription; 
  private:
  vector<FrameInfo *> frames; 
} ;


/* A format is a collection of routines that know how to read 
 * image files of a given format.
 */
/* struct FileFormat {
   char *name;
   char *description;
*/
  /* This returns a list of frames available in the file, or NULL if the file
   * can't be handled by this format driver, or if there are no frames available
   * in the file.  The list of frames is terminated by a NULL, and should be
   * released with free() when the application is done with it.
   *
   * Note that all other function pointers are associated with the frame, not
   * with the format; this lets the format choose optimized functions (rather
   * than generic functions) if it determines that such are appropriate, on
   * a per-frame basis.
   */
  /* FrameList *(*GetFrameList)(const char *filename);
     
  } ;
 
  extern FileFormat *fileFormats[];
  extern FileFormat pngFormat, pnmFormat, tiffFormat, smFormat, sgiRgbFormat;
  */ 

//FrameList *LoadFrameList(int fileCount, const char **files, void *settings);
void DefaultDestroyFrameInfo(FrameInfo *frameInfo);

#endif
