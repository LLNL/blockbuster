#ifndef BBTIFF_H
#define BBTIFF_H
#include "frames.h"
#include "boost/shared_ptr.hpp"
FrameListPtr tiffGetFrameList(const char *filename);

typedef boost::shared_ptr<struct TiffFrameInfo> TiffFrameInfoPtr; 

#define TIFF_INVALID 0
#define TIFF_RGBA 1
#define TIFF_24BIT 2

// ==================================================================
struct TiffFrameInfo: public FrameInfo {
  // ----------------------------------------------
  TiffFrameInfo(string fname); 

  // ----------------------------------------------
  virtual ~TiffFrameInfo() {
    return;
  }

  // ----------------------------------------------
  TiffFrameInfo(const TiffFrameInfo &other) :
    FrameInfo(other), mBitsPerSample(other.mBitsPerSample), 
    mSamplesPerPixel(other.mSamplesPerPixel), mPhotometric(other.mPhotometric),
    mPlanarConfiguration(other.mPlanarConfiguration), 
    mMinSample(other.mMinSample), mMaxSample(other.mMaxSample),
    mScanlineBuffer(other.mScanlineBuffer), mTiffType(other.mTiffType) {
    return; 
  }
  
  // ----------------------------------------------
  virtual TiffFrameInfo* Clone()  const{
    return new TiffFrameInfo(*this); 
  }
  
  // ----------------------------------------------
  int RGBALoadImage(ImagePtr image,  
                    ImageFormat *requiredImageFormat, 
                    const Rectangle *,
                    int levelOfDetail);

  // ----------------------------------------------
  int Color24LoadImage(ImagePtr image,  
                       ImageFormat *requiredImageFormat, 
                       const Rectangle *,
                       int levelOfDetail);

  // ----------------------------------------------
  virtual int LoadImage(ImagePtr image,  
                       ImageFormat *fmt, 
                        const Rectangle *region, 
                        int lod) {
    if (mTiffType == TIFF_24BIT) {
      return Color24LoadImage(image, fmt, region, lod); 
    } else {
      return RGBALoadImage(image, fmt, region, lod); 
    }
    return 0; 
  }


  /* 
   * Values used with TIFFGetField have to be of the specific
   * correct types, as the function is a varargs function
   * and expects these types.
   */
  uint16_t  mBitsPerSample, mSamplesPerPixel, 
    mPhotometric, mPlanarConfiguration;
  double mMinSample, mMaxSample;
  vector<unsigned char> mScanlineBuffer; /* if scanline conversion is needed */
  int mTiffType; 
  
}; 

#endif
