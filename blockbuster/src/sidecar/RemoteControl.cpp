#include "RemoteControl.h"
#include "errmsg.h"

#define BLOCK_APPLY(widget, cmd)                 \
  widget->blockSignals(true);  \
  widget->cmd; \
  widget->blockSignals(false)

#define LOCK_BLOCK_APPLY(widget, value, cmd)           \
  if (widget->Unlock(value)) {                        \
  widget->blockSignals(true);  \
  widget->cmd; \
  widget->blockSignals(false);                   \
  }


void RemoteControl::updateFromSnapshot(MovieSnapshot &snapshot){
  mStartFrame = snapshot.mStartFrame; 
  mEndFrame = snapshot.mEndFrame; 

  BLOCK_APPLY(mInfoWindow->windowSizeLabel, setText( QString("%1, %2").arg(snapshot.mScreenWidth).arg(snapshot.mScreenHeight)));  
  BLOCK_APPLY(mInfoWindow->windowXYLabel, setText(QString("%1, %2").arg(snapshot.mScreenXpos).arg(snapshot.mScreenYpos))); 

  BLOCK_APPLY(mInfoWindow->imageSizeLabel, setText(QString("%1, %2").arg((int32_t)snapshot.mImageWidth).arg((int32_t)snapshot.mImageHeight))); 
  BLOCK_APPLY(mInfoWindow->displaySizeLabel, setText(QString("%1, %2").arg((int32_t)(snapshot.mZoom*snapshot.mImageHeight)).arg((int32_t)(snapshot.mZoom*snapshot.mImageWidth)))); 
  BLOCK_APPLY(mInfoWindow->imageXYLabel, setText(QString("%1, %2").arg(snapshot.mImageXpos).arg(snapshot.mImageYpos))); 

  BLOCK_APPLY(stereoCheckBox, setChecked(snapshot.mStereo)); 
  BLOCK_APPLY(fpsLabel, setText(QString("%1").arg(snapshot.mFrameRate))); 
  
  BLOCK_APPLY(fullScreenCheckBox, setChecked(snapshot.mFullScreen)); 
  BLOCK_APPLY(zoomToFitCheckBox, setChecked(snapshot.mZoomToFit)); 
  LOCK_BLOCK_APPLY(zoomSpinBox, snapshot.mZoom,  setValue(snapshot.mZoom)); 
  LOCK_BLOCK_APPLY(lodSpinBox, snapshot.mLOD, setValue(snapshot.mLOD)); 

  BLOCK_APPLY(frameSlider, setRange(1, snapshot.mNumFrames));
  BLOCK_APPLY(frameSlider, setValue( snapshot.mFrameNumber+1)); 
  BLOCK_APPLY(frameSlider, setPageStep((snapshot.mStartFrame + 1 - snapshot.mEndFrame)/10)); 

  QString frametext = QString("%1").arg(snapshot.mFrameNumber+1); 
  LOCK_BLOCK_APPLY(frameField, frametext, setText(frametext)); 

  LOCK_BLOCK_APPLY(fpsSpinBox, snapshot.mTargetFPS, setValue( snapshot.mTargetFPS)); 

  BLOCK_APPLY(repeatCheckBox, setChecked(snapshot.mRepeat != 0)); 
  BLOCK_APPLY(foreverCheckBox, setEnabled(snapshot.mRepeat != 0)); 
  BLOCK_APPLY(foreverCheckBox, setChecked(snapshot.mRepeat == -1)); 
  BLOCK_APPLY(noScreensaverCheckBox, setChecked(snapshot.mNoScreensaver)); 
  BLOCK_APPLY(pingpongCheckBox, setChecked(snapshot.mPingPong)); 
  dbprintf(5, QString("Updated control window from %1\n").arg(string(snapshot).c_str())); 
  return; 
}
