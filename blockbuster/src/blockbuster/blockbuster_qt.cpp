/*
  #
  # $RCSfile: blockbuster_qt.cpp,v $
  # $Name:  $
  # 
  # ASCI Visualization Project 
  #
  # Lawrence Livermore National Laboratory
  # Information Management and Graphics Group
  # P.O. Box 808, Mail Stop L-561
  # Livermore, CA 94551-0808
  #
  # For information about this project see:
  # 	http://www.llnl.gov/sccd/lc/img/ 
  #
  # 	or contact: asciviz@llnl.gov
  #
  # For copyright and disclaimer information see:
  #	$(ASCIVIS_ROOT)/copyright_notice_1.txt
  #
  # $Id: blockbuster_qt.cpp,v 1.14 2009/05/05 21:57:28 wealthychef Exp $
  #
  # Abstract: 
  #  Contains the object definitions related to the blockbuster Qt GUI
  #
  #   Author: Rich Cook
  #
  # This work performed under the auspices of the U.S. Department of Energy by Lawrence Livermore National Laboratory under Contract DE-AC52-07NA27344.
  # This document was prepared as an account of work sponsored by an agency of the United States government. Neither the United States government nor Lawrence Livermore National Security, LLC, nor any of their employees makes any warranty, expressed or implied, or assumes any legal liability or responsibility for the accuracy, completeness, or usefulness of any information, apparatus, product, or process disclosed, or represents that its use would not infringe privately owned rights. Reference herein to any specific commercial product, process, or service by trade name, trademark, manufacturer, or otherwise does not necessarily constitute or imply its endorsement, recommendation, or favoring by the United States government or Lawrence Livermore National Security, LLC. The views and opinions of authors expressed herein do not necessarily state or reflect those of the United States government or Lawrence Livermore National Security, LLC, and shall not be used for advertising or product endorsement purposes.
  #
*/
#include "qmetatype.h"
#include <QFileDialog>
#include "QMessageBox"
#include "pathutil.h"
#include "blockbuster_qt.h"
#include "movie.h"
#include "errmsg.h"
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
// nothing here -- everything in header
BlockbusterInterface::BlockbusterInterface(QWidget *parent):
  QWidget(parent),  mFrameValidator(1,1, parent), 
  mFrameNumber(-1), mFrameRate(-1), mZoom(-1) {
  setupUi(this); 
  quitButton->setText("Quit"); 
  
  mInfoWindow = new BlockbusterInfoWindow(NULL); 
  //  horizontalLayout_5->addWidget(mInfoWindow); 

  // the hide button is peculiar to only blockbuster, not sidecar
  QPushButton *hideButton = new QPushButton("Hide", this); 
  horizontalLayout_5->addWidget(hideButton); 
  connect(hideButton, SIGNAL(clicked()), this, SLOT(hideButtonClicked())); 

  setFocusPolicy(Qt::StrongFocus); 
  frameField->setValidator(&mFrameValidator); 
  frameSlider->setTracking(false); 
  //startButton->setText("); 
  //(QIcon(":/images/start.png")); 
  backStepButton->setIcon(QIcon(":/images/backStep.png")); 
  //centerButton->setIcon(QIcon(":/images/center.png")); 
  endButton->setIcon(QIcon(":/images/end.png")); 
  //fitButton->setIcon(QIcon(":/images/fit.png")); 
  //fullSizeButton->setIcon(QIcon(":/images/fullSize.png")); 
  //openButton->setIcon(QIcon(":/images/open.png")); 
  playButton->setIcon(QIcon(":/images/play.png")); 
  //quitButton->setIcon(QIcon(":/images/quit.png")); 
  reverseButton->setIcon(QIcon(":/images/reverse.png")); 
  startButton->setIcon(QIcon(":/images/start.png")); 
  stepButton->setIcon(QIcon(":/images/step.png")); 
  stopButton->setIcon(QIcon(":/images/stop.png")); 
  
  int iconSize = 50; 
  backStepButton->setIconSize(QSize(iconSize, iconSize)); 
  endButton->setIconSize(QSize(iconSize, iconSize)); 
  openButton->setIconSize(QSize(iconSize, iconSize)); 
  playButton->setIconSize(QSize(iconSize, iconSize)); 
  quitButton->setIconSize(QSize(iconSize, iconSize)); 
  reverseButton->setIconSize(QSize(iconSize, iconSize)); 
  startButton->setIconSize(QSize(iconSize, iconSize)); 
  stepButton->setIconSize(QSize(iconSize, iconSize)); 
  stopButton->setIconSize(QSize(iconSize, iconSize)); 
  
  
  backStepButton->resize(QSize(iconSize, iconSize)); 
  endButton->resize(QSize(iconSize, iconSize)); 
  openButton->resize(QSize(iconSize, iconSize)); 
  playButton->resize(QSize(iconSize, iconSize)); 
  quitButton->resize(QSize(iconSize, iconSize)); 
  reverseButton->resize(QSize(iconSize, iconSize)); 
  startButton->resize(QSize(iconSize, iconSize)); 
  stepButton->resize(QSize(iconSize, iconSize)); 
  stopButton->resize(QSize(iconSize, iconSize)); 
  return;  
}

//=============================================================
bool BlockbusterInterface::GetEvent (MovieEvent &event) {
  //gCoreApp->processEvents(); 
  if (mEventQueue.size()) {
    event = mEventQueue.front(); 
    mEventQueue.pop_front(); 
    return true; 
  }
  return false; 
}

//=============================================================
void BlockbusterInterface::setFrameNumber(int frameNumber) {
  //DEBUGMSG("BlockbusterInterface::setFrameNumber", frameNumber); 
  mFrameNumber = frameNumber; 
  frameSlider->blockSignals(true); 
  frameSlider->setValue(frameNumber); 
  frameSlider->blockSignals(false); 
  frameField->blockSignals(true); 
  frameField->setText(QString("%1").arg(frameNumber)); 
  frameField->blockSignals(false); 
  //cerr << "setFrameNumber not implemented" << endl; 
  return; 
}

//=============================================================
void BlockbusterInterface::setFrameRange(int32_t bottom, int32_t top) {
  mStartFrame = bottom; 
  mEndFrame = top; 
  frameSlider->blockSignals(true); 
  mFrameValidator.setRange(bottom, top); 
  frameSlider->setRange(bottom, top); 
  frameSlider->blockSignals(false); 
  return; 
} 
 
//=============================================================
void BlockbusterInterface::setTitle(QString title) {
  setWindowTitle(title); 
  return; 
}

//=============================================================
void BlockbusterInterface::setLOD(int lod){
  mLOD = lod; 
  lodSpinBox->blockSignals(true); 
  lodSpinBox->setValue(lod); 
  lodSpinBox->blockSignals(false); 
  return; 
}

//=============================================================
void BlockbusterInterface::setLODRange(int min, int max){
  lodSpinBox->blockSignals(true); 
  lodSpinBox->setRange(min,max); 
  mLOD = min; 
  lodSpinBox->setValue(min); 
  lodSpinBox->blockSignals(false); 
  return; 
}

//=============================================================
void BlockbusterInterface::setFrameRate(double rate) {
  mFrameRate = rate; 
  fpsSpinBox->blockSignals(true); 
  fpsSpinBox->setValue(rate); 
  fpsSpinBox->blockSignals(false); 
  return; 
}

//=============================================================
void BlockbusterInterface::setFrameRateRange(double min, double max) {
  mFrameRate = max; 
  fpsSpinBox->blockSignals(true); 
  fpsSpinBox->setRange(min,max); 
  fpsSpinBox->setValue(max);
  fpsSpinBox->blockSignals(false); 
  return; 
}

//=============================================================
void BlockbusterInterface::setStereo(bool stereo){
  stereoCheckBox->blockSignals(true); 
  stereoCheckBox->setChecked(stereo); 
  stereoCheckBox->blockSignals(false); 
  return; 
}

//=============================================================
void BlockbusterInterface::setZoom(double zoom){
  mZoom = zoom; 
  zoomSpinBox->blockSignals(true); 
  zoomSpinBox->setValue(zoom);   
  zoomSpinBox->blockSignals(false); 
  return; 
}

//=============================================================
void BlockbusterInterface::setZoomToFit(bool ztf){
  mZoomToFit = ztf; 
  zoomToFitCheckBox->blockSignals(true); 
  zoomToFitCheckBox->setChecked(ztf);   
  zoomToFitCheckBox->blockSignals(false); 
  return; 
}

//=============================================================
void BlockbusterInterface::setRepeatBehavior (int behavior){
  // avoid spurious signals: 
  mRepeat = behavior; 
  foreverCheckBox->setChecked (behavior == REPEAT_FOREVER);   
  repeatCheckBox->setChecked(behavior != 0); 
  return; 
}

//=============================================================
void BlockbusterInterface::setPingPongBehavior (int behavior){
  mPingPong = behavior; 
  pingpongCheckBox->setChecked(behavior);   
  return; 
}

//=============================================================
void BlockbusterInterface::reportWindowMoved(int xpos, int ypos){
  mInfoWindow->windowXYLabel->setText(QString("%1, %2").arg(xpos).arg(ypos)); 
  return; 
}

//=============================================================
void BlockbusterInterface::reportWindowResize(int x, int y){
  mInfoWindow->windowSizeLabel->setText(QString("%1, %2").arg(x).arg(y));   
  return; 
}

//=============================================================
void BlockbusterInterface::reportMovieMoved(int xpos, int ypos){
  //cerr << "reportMovieMoved " << xpos << ", " << ypos << endl; 
  mInfoWindow->imageXYLabel->setText(QString("%1, %2").arg(xpos).arg(ypos)); 
  return; 
} 

//=============================================================
void BlockbusterInterface::reportMovieFrameSize(int x, int y){
  //cerr << "movie frame size " << x <<", " << y<<endl; 
  mInfoWindow->imageSizeLabel->setText(QString("%1, %2").arg(x).arg(y)); 
  return; 
} 

//=============================================================
void BlockbusterInterface::reportMovieDisplayedSize(int x, int y){
  mInfoWindow->displaySizeLabel->setText(QString("%1, %2").arg(x).arg(y)); 
  return; 
} 

//=============================================================
void BlockbusterInterface::reportActualFPS(double rate){
  fpsLabel->setText(QString("%1").arg(rate)); 
  return; 
}


//=============================================================
void BlockbusterInterface::reportMovieCueStart(void){
  statusLabel->setText("CUE RUNNING"); 
  return; 
}

//=============================================================
void BlockbusterInterface::reportMovieCueComplete(void){
  statusLabel->setText(""); 
  return; 
}


//=============================================================
void BlockbusterInterface::on_quitButton_clicked() {
  mEventQueue.push_back(MovieEvent("MOVIE_QUIT")); 
  return; 
}

//=============================================================
void BlockbusterInterface::on_openButton_clicked() {
  QString filename = 
    QFileDialog::  getOpenFileName(this, "Choose a movie file",
                                   mPreviousDir,
                                   "Readable Files (*.sm *png *tiff *pnm *raw *sgi);;Any (*)");
  if (filename != "") {
    mPreviousDir = Dirname(filename.toStdString()).c_str(); 
    mEventQueue.push_back(MovieEvent("MOVIE_OPEN_FILE", filename)); 
  }
}

//======================================================   
void BlockbusterInterface::on_stereoCheckBox_stateChanged(int state) {
  mEventQueue.push_back(MovieEvent("MOVIE_SET_STEREO", state)); 
  return; 
}

void BlockbusterInterface::on_centerPushButton_clicked() {
  mEventQueue.push_back(MovieEvent("MOVIE_CENTER", centerPushButton->isChecked())); 
}
void BlockbusterInterface::on_fullSizeButton_clicked() {
  mEventQueue.push_back(MovieEvent("MOVIE_ZOOM_ONE")); 
}
void BlockbusterInterface::on_zoomToFitCheckBox_clicked() {
  mEventQueue.push_back(MovieEvent("MOVIE_ZOOM_TO_FIT", zoomToFitCheckBox->isChecked())); 
}
void BlockbusterInterface::on_fullScreenCheckBox_clicked() {
  mEventQueue.push_back(MovieEvent("MOVIE_FULLSCREEN", fullScreenCheckBox->isChecked()));  
  if (fullScreenCheckBox->isChecked()) {
    sizeToMovieCheckBox->setChecked(false); 
  }
  return; 
}
void BlockbusterInterface::on_sizeToMovieCheckBox_clicked() {
  mEventQueue.push_back(MovieEvent("MOVIE_SIZE_TO_MOVIE", sizeToMovieCheckBox->isChecked()));     
  if (sizeToMovieCheckBox->isChecked()) {
    fullScreenCheckBox->setChecked(false); 
  }
  return; 
}
void BlockbusterInterface::on_infoButton_clicked() {
  mInfoWindow->show(); 
}
void BlockbusterInterface::on_startButton_clicked() {
  mEventQueue.push_back(MovieEvent("MOVIE_GOTO_START")); 
}
void BlockbusterInterface::on_backStepButton_clicked() {
  mEventQueue.push_back(MovieEvent("MOVIE_STEP_BACKWARD")); 
}
void BlockbusterInterface::on_reverseButton_clicked() {
  mEventQueue.push_back(MovieEvent("MOVIE_PLAY_BACKWARD")); 
}
void BlockbusterInterface::on_stopButton_clicked() {
  mEventQueue.push_back(MovieEvent("MOVIE_STOP")); 
}
void BlockbusterInterface::on_playButton_clicked() {
  mEventQueue.push_back(MovieEvent("MOVIE_PLAY_FORWARD")); 
}
void BlockbusterInterface::on_stepButton_clicked() {
  mEventQueue.push_back(MovieEvent("MOVIE_STEP_FORWARD")); 
}
void BlockbusterInterface::on_endButton_clicked() {
  mEventQueue.push_back(MovieEvent("MOVIE_GOTO_END")); 
}

void BlockbusterInterface::on_saveImageButton_clicked() {
  mEventQueue.push_back(MovieEvent("MOVIE_SAVE_FRAME"));
  
  return; 
}

//======================================================   
void BlockbusterInterface::hideButtonClicked() {
  this->hide();
}


//======================================================   
void BlockbusterInterface::on_frameSlider_valueChanged(int value) {
  if (value == mFrameNumber) return; 
  frameSlider->blockSignals(true); 
  if (value <mStartFrame) {
    frameSlider->setValue(mStartFrame);
    value = mStartFrame; 
    return;
  }
  if (value > mEndFrame) {
    frameSlider->setValue(mEndFrame); 
    value = mEndFrame; 
    return;
  }
  frameSlider->blockSignals(false); 
  setFrameNumber(value); 
  mEventQueue.push_back(MovieEvent ("MOVIE_GOTO_FRAME", value-1)); 
}

//======================================================   
void BlockbusterInterface::on_frameField_returnPressed() {
  mEventQueue.push_back
    (MovieEvent("MOVIE_GOTO_FRAME",frameField->text().toInt()));
  return; 
}

//======================================================   
void BlockbusterInterface::on_zoomSpinBox_valueChanged(double value) {
  if (value != mZoom) {
    mEventQueue.push_back(MovieEvent("MOVIE_ZOOM_SET", (float)value));     
    zoomToFitCheckBox->setChecked(false); 
  }
  return; 
}

//======================================================   
void BlockbusterInterface::on_lodSpinBox_valueChanged(int value) {
  if (value != mLOD) {
    mEventQueue.push_back(MovieEvent("MOVIE_SET_LOD", value)); 
  }
  return; 
}

//======================================================   
void BlockbusterInterface::on_fpsSpinBox_valueChanged(double value) {
  if (value != mFrameRate) {
    mEventQueue.push_back(MovieEvent("MOVIE_SET_RATE", (float)value)); 
  }
  return; 
}

//======================================================   
void BlockbusterInterface::on_foreverCheckBox_stateChanged(int state)  {
  if ((state != 0) == (mRepeat == -1)) return; 

  MovieEvent event("MOVIE_SET_REPEAT", -1); 
  if (!state) {
    event.mNumber = repeatCheckBox->isChecked(); 
  }
  mEventQueue.push_back(event); 
  return; 
}

//======================================================   
void BlockbusterInterface::on_pingpongCheckBox_stateChanged(int state) {
  if (state && repeatCheckBox->isChecked()) {
    repeatCheckBox->setChecked(false); 
  }
  if (state == mPingPong) return; 
  mEventQueue.push_back(MovieEvent("MOVIE_SET_PINGPONG", state)); 
  return; 
}

//======================================================   
void BlockbusterInterface::on_scrubCheckBox_stateChanged(int state) {
  frameSlider->setTracking(state); 
  return; 
}

//======================================================   
void BlockbusterInterface::on_repeatCheckBox_stateChanged(int state) {
  foreverCheckBox->setEnabled(state); 
  if (state && pingpongCheckBox->isChecked()){
    pingpongCheckBox->setChecked(false); 
  }
  if ( (state && mRepeat) || (!state && !mRepeat)) return; 
  if (state > 1) state = 1; 
  mEventQueue.push_back(MovieEvent("MOVIE_SET_REPEAT", state)); 
  return; 
}
