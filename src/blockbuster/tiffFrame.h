#ifndef BBTIFF_H
#define BBTIFF_H
#include "frames.h"
#include "boost/shared_ptr.hpp"
FrameList *tiffGetFrameList(const char *filename);

typedef boost::shared_ptr<struct TiffFrameInfo> TiffFrameInfoPtr; 

// ==================================================================
struct TiffFrameInfo: public FrameInfo {
  TiffFrameInfo():bitsPerSample(0), samplesPerPixel(0), 
                       photometric(false), 
                       minSample(0), maxSample(0) {
    // this exists to ensure that mPrivateData's destructor is called
    return; 
  }
  virtual ~TiffFrameInfo() {
    return;
  }

  virtual int LoadImage(ImageFormat *fmt, 
                        const Rectangle *region, 
                        int lod) {
    return 0; 
  }


  int bitsPerSample;	/* 8 or 16 */
  int samplesPerPixel; /* 1 - grayscale; 3 - color */
  int photometric; /* can invert a grayscale image */
  double minSample; /* for 16-bit samples */
  double maxSample; /* for 16-bit samples */
  vector<unsigned char> scanlineBuffer; /* if scanline conversion is needed */

}; 

#endif
