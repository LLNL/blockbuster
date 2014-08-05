/* 

*/
#include "sm/smBase.h"
#include <MovieCues.h>
#include "QInputDialog"
#include "QMessageBox"
#include "QFileDialog"
#include "events.h"
#include <iostream> 
#include "errmsg.h"
using namespace std; 

//======================================================================
MovieCue::MovieCue(MovieCue *other, QListWidget *parent): QListWidgetItem(other->text(), parent) {
  setText(other->text() + "_copy"); 
  mMovieName = other->mMovieName; 
  mLoadMovie = other->mLoadMovie; 
  mPlayMovie = other->mPlayMovie; 
  mPlayBackward = other->mPlayBackward; 
  mShowControls = other->mShowControls; 
  mFullScreen = other->mFullScreen;
  mSizeToMovie = other->mSizeToMovie;
  mPingPong = other->mPingPong; 
  mCurrentFrame = other->mCurrentFrame; 
  mStartFrame = other ->mStartFrame; 
  mEndFrame = other->mEndFrame;
  mRepeatFrames = other->mRepeatFrames;
  mWindowWidth = other->mWindowWidth;
  mWindowHeight = other->mWindowHeight;
  mWindowXPos = other->mWindowXPos;
  mWindowYPos = other->mWindowYPos;
  mImageXPos = other->mImageXPos;
  mImageYPos = other->mImageYPos;
  mLOD = other->mLOD;
  mFrameRate = other->mFrameRate;
  mZoom = other->mZoom;
  mZoomOne = other->mZoomOne; 
  mZoomToFit = other->mZoomToFit; 
  isValid = other->isValid;
  mEOF = other->mEOF;
  return; 
}

//======================================================================
void MovieCue::ReadScript(const MovieScript &iScript) {
  for (std::vector<MovieEvent>::const_iterator pos = iScript.begin(); pos != iScript.end(); pos++) {
    if (pos->mEventType == "MOVIE_CUE_BEGIN") {
      setText(QString(pos->mString.c_str())); 
    }
    else if (pos->mEventType == "MOVIE_OPEN_FILE") {
      mLoadMovie = true; 
      mMovieName = pos->mString.c_str(); 
      mCurrentFrame = pos->mNumber; 
    }
    else if (pos->mEventType == "MOVIE_CUE_PLAY_ON_LOAD") { 
      mPlayMovie = pos->mHeight; 
    }
    else if (pos->mEventType == "MOVIE_SET_PINGPONG") { 
      mPingPong = pos->mHeight; 
    }
    else if (pos->mEventType == "MOVIE_SET_REPEAT") { 
      mRepeatFrames = pos->mHeight; 
    }
    else if (pos->mEventType == "MOVIE_SHOW_INTERFACE") {
	  mShowControls = true;
    }
    else if (pos->mEventType == "MOVIE_HIDE_INTERFACE") {
      mShowControls = false;
    }
    else if (pos->mEventType == "MOVIE_CUE_PLAY_BACKWARD") { 
      mPlayBackward = pos->mHeight; 
    }
    else if (pos->mEventType == "MOVIE_GOTO_FRAME") {
      mCurrentFrame = pos->mNumber; 
    }
    else if (pos->mEventType == "MOVIE_START_END_FRAMES") { 
      mStartFrame = pos->mWidth; 
      mEndFrame = pos->mHeight; 
    }
    else if (pos->mEventType == "MOVIE_SET_RATE") { 
      mFrameRate = pos->mRate; 
    }
    else if (pos->mEventType == "MOVIE_FULLSCREEN") {
      mFullScreen = pos->mNumber;
      mSizeToMovie = !pos->mNumber; 
      if (pos->mNumber) {
        mWindowXPos = 0;
        mWindowYPos = 0; 
        mWindowWidth = 0; 
        mWindowHeight = 0; 
      }
    }
    else if (pos->mEventType == "MOVIE_SIZE_TO_MOVIE") { 
      mSizeToMovie = pos->mNumber; 
      mFullScreen = !pos->mNumber; 
    }
    else if (pos->mEventType == "MOVIE_ZOOM_SET") {
      mZoom = pos->mRate;
    }
    else if (pos->mEventType == "MOVIE_ZOOM_ONE") { 
      mZoomOne = true; 
    }
    else if (pos->mEventType == "MOVIE_ZOOM_TO_FIT") { 
      mZoomToFit = pos->mNumber; 
    }
    else if (pos->mEventType == "MOVIE_MOVE_RESIZE") { 
      mWindowXPos = pos->mX; 
      mWindowYPos = pos->mY; 
      mWindowWidth = pos->mWidth; 
      mWindowHeight = pos->mHeight; 
    }  
    else if (pos->mEventType == "MOVIE_IMAGE_MOVE") { 
      mImageXPos = pos->mX; 
      mImageYPos = pos->mY; 
    }
    else if (pos->mEventType == "MOVIE_SET_LOD") {
      mLOD = pos->mX; 
    }
    else {
      cerr << "Warning:  unknown event in Cue script: " << pos->mEventType << endl; 
      
    }
    ++pos; 
  }
  return; 
}

//======================================================================
void MovieCue::GenerateScript(MovieScript &oScript) const{
  oScript.clear();   
  /* store events pertaining to the user interface */
  oScript.push_back(MovieEvent("MOVIE_CUE_BEGIN", text(), mEndFrame)); 
  if (mShowControls) {
    oScript.push_back(MovieEvent("MOVIE_SHOW_INTERFACE"));
  } else {
    oScript.push_back(MovieEvent("MOVIE_HIDE_INTERFACE"));
  } 
	
  if (mLoadMovie && mMovieName != "") {    
    oScript.push_back(MovieEvent("MOVIE_OPEN_FILE", mMovieName, mCurrentFrame));
  }  

  if (mPlayMovie) {
    oScript.push_back(MovieEvent("MOVIE_CUE_PLAY_ON_LOAD", mPlayBackward?-1:1));
  }

  oScript.push_back(MovieEvent("MOVIE_SET_REPEAT", mRepeatFrames));

  oScript.push_back(MovieEvent("MOVIE_SET_PINGPONG", mPingPong));

  oScript.push_back(MovieEvent("MOVIE_FULLSCREEN", mFullScreen)); 
    
  oScript.push_back(MovieEvent("MOVIE_SIZE_TO_MOVIE", mSizeToMovie)); 
    
  oScript.push_back(MovieEvent("MOVIE_GOTO_FRAME",mCurrentFrame));  

  oScript.push_back(MovieEvent("MOVIE_START_END_FRAMES", mStartFrame, mEndFrame, 0,0)); 

  int xPos = mWindowXPos, yPos = mWindowYPos, width = mWindowWidth, height = mWindowHeight; 
  if (!mFullScreen && !mSizeToMovie) {
    if (width != -1 || height != -1) {
      if (width == -1) width = 0; 
      if (height == -1) height = 0; 
      oScript.push_back(MovieEvent("MOVIE_RESIZE", width, height, 0, 0)); 
    }  
    if (xPos != -1 || yPos != -1) {
      if (xPos == -1) xPos = 0; 
      if (yPos == -1) yPos = 0; 
      oScript.push_back(MovieEvent("MOVIE_MOVE", 0, 0, xPos, yPos)); 
    }
  }

  if (mZoomOne) {
    oScript.push_back(MovieEvent("MOVIE_ZOOM_ONE")); 
  } 
  else if (mZoomToFit) {
    oScript.push_back(MovieEvent("MOVIE_ZOOM_TO_FIT", mZoomToFit)); 
  } 
  else {
    oScript.push_back(MovieEvent("MOVIE_ZOOM_SET", mZoom)); 
  }

  if (mNoStereo) {
    oScript.push_back(MovieEvent("MOVIE_SET_STEREO", 0)); 
  } 
    
 
  oScript.push_back(MovieEvent("MOVIE_SET_RATE", mFrameRate)); 
  oScript.push_back(MovieEvent("MOVIE_SET_LOD", mLOD)); 
  oScript.push_back(MovieEvent("MOVIE_IMAGE_MOVE", 0,0, mImageXPos, mImageYPos));
  if (mPlayMovie) {
    if (mPlayBackward){
      oScript.push_back(MovieEvent("MOVIE_PLAY_BACKWARD"));
    }
    else{
      oScript.push_back(MovieEvent("MOVIE_PLAY_FORWARD"));
    }
  }
  /* end of record marker */
  oScript.push_back(MovieEvent("MOVIE_CUE_END")); 
  return; 
}


//======================================================================
MovieCueManager::MovieCueManager(QWidget *parent ) :
  QWidget(parent), mCurrentCue(NULL), 
  mBlockExecution(false) {
  SetCueUnchanged(); 
  setupUi(this); 
  connect(movieCueList, SIGNAL(itemActivated(QListWidgetItem *)), 
          this, SLOT(userDoubleClickedItem(QListWidgetItem*))); 
  connect(currentFrameField, SIGNAL(editingFinished()), 
          this, SLOT(Validate())); 
  return; 
}



//======================================================================
/* call this when new cues are added or removed, and when content or order of cues is changed, and when cue files are saved or loaded */
void MovieCueManager::cueFileDirty(bool dirty){
  int cues = movieCueList->count(); 
  saveCuesButton->setEnabled(dirty && cues); 
  saveCuesAsButton->setEnabled(cues); 
  deleteAllCuesButton->setEnabled(cues); 
  mCueFileDirty = dirty; 
  return;  
}

//======================================================================
/*! 
  Check if there are unsaved changes.  If so, give the user the options to:
  1 -- save the cue file before quitting
  2 -- quit without saving
  3 -- do nothing and not quit
  Returns true if the user chose one one of the first two options. 
*/
bool MovieCueManager::okToQuit(void) {
  if (!okToCloseWindow()) {
    return false; 
  }
  if (!mCueFileDirty) {
    return true; 
  } else {
    QMessageBox::StandardButton reply =
      QMessageBox::critical(NULL, tr("Save Cue File"),
                            "The cue file has changed.  Save before quitting?",
                            QMessageBox::Save | 
                            QMessageBox::Discard | QMessageBox::Cancel);
    if (reply == QMessageBox::Save){
      on_saveCuesButton_clicked(); 
      return true;
    }  else if (!reply || reply == QMessageBox::Discard) {
      return true; 
    } else {
      return false; 
    }   
  }
  throw string("Logic error:  okToQuit -- reached unreachable line"); 
  return false;  // should never get here
}

//======================================================================
bool MovieCueManager::okToCloseWindow(void) {
  if (cueChanged()) {
    QMessageBox::StandardButton reply =
      QMessageBox::critical(this, tr("Save Current Cue"),
                            "The current cue has changed.  Apply changes before closing?",
                            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (reply == QMessageBox::Save){
      on_applyChangesButton_clicked(); 
      return true; 
    }  else if (reply == QMessageBox::Discard) {
      setCurrentCue(mCurrentCue); // revert
      return true; 
    } else {
      return false; 
    }
  } else {
    return true; 
  }
  throw string("Logic error:  okToCloseWindow -- reached unreachable line"); 
  return false; // should never get here 
}
//======================================================================
void MovieCueManager::closeEvent(QCloseEvent *event) {
  if (okToCloseWindow()) {
    emit closed(); 
    event->accept(); 
  } else {
    event->ignore(); 
  }
  return; 
}


//======================================================================
void MovieCueManager::setCueRunning(bool running) {
  //change interface to reflect what's happening:
  if (!running) {
    emit stopCurrentCue(); 
    movieCueList->setEnabled(true); 
    executeCueButton->setText("Execute");
    on_movieCueList_itemSelectionChanged(); 
  } else {
    movieCueList->setEnabled(false); 
    moveToBottomButton->setEnabled(false); 
    moveDownButton->setEnabled(false); 
    moveUpButton->setEnabled(false); 
    moveToTopButton->setEnabled(false); 
    duplicateCueButton->setEnabled(false);
    executeCueButton->setText("Stop Cues");
    loopCuesButton->setEnabled(false); 
    deleteCueButton->setEnabled(false); 
  }
  return; 
}

//======================================================================
void MovieCueManager::on_deleteCueButton_clicked(){
  int definitely =  QMessageBox::
    question( this,
              tr("Delete Cue"),
              tr("Are you sure you want to delete the selected Cues?  "
                 "There is no undo capability!"),
              QMessageBox::No | QMessageBox::Yes, QMessageBox::No);
  
  if (definitely == QMessageBox::No) {
    return; 
  }
  
   
  

  QList<QListWidgetItem *> theList = movieCueList->selectedItems();

  if (!theList.size()) return; 

  QList<QListWidgetItem *>::iterator pos = theList.begin(), endpos=theList.end(); 
  int row=0; 
  while (pos != endpos) {
    row = movieCueList->row(*pos);
    MovieCue *theCue = dynamic_cast<MovieCue *>(movieCueList->takeItem(row)); 
    delete theCue; 
    ++pos;
  }
  if (movieCueList->count()) {
    if (!row) row=1;
    movieCueList->setCurrentRow(row-1); 
    setCurrentCue(dynamic_cast<MovieCue *>(movieCueList->currentItem())); 
  } else {
    setCurrentCue(NULL); 
  }
  
  cueFileDirty(true);   
  return; 
}

//======================================================================
void MovieCueManager::on_deleteAllCuesButton_clicked(){
  int definitely =  QMessageBox::
    question( this,
              tr("Delete Cue"),
              tr("Are you sure you want to delete all Cues?"
                 "There is no undo capability!"),
              QMessageBox::No, QMessageBox::Yes);
  if (definitely == QMessageBox::No) {
    return; 
  }
  setCurrentCue(NULL); 
  movieCueList->clear(); 
  cueFileDirty(true); 
  return; 
}

//======================================================================
void MovieCueManager::on_moveToTopButton_clicked(){
  int row = movieCueList->currentRow();

  if (!row) {
    QMessageBox::warning(this,  "Error",
                         "Attempted to move the top item up in the list");
    return; 
  }

  MovieCue *theCue = dynamic_cast<MovieCue *>(movieCueList->takeItem(row)); 
  movieCueList->insertItem(0, theCue); 
  movieCueList->setCurrentRow(0); 
  cueFileDirty(true); 
  return; 
}

//======================================================================
void MovieCueManager::on_moveUpButton_clicked(){
  int row = movieCueList->currentRow();
  if (!row) {
    QMessageBox::warning(this,  "Error",
                         "Attempted to move the top item up in the list");
    return; 
  }

  MovieCue *theCue = dynamic_cast<MovieCue *>(movieCueList->takeItem(row));   
  movieCueList->insertItem(row-1, theCue); 
  movieCueList->setCurrentRow(row-1); 
  cueFileDirty(true); 
  
  return; 
}

//======================================================================
void MovieCueManager::on_moveDownButton_clicked(){
  int row = movieCueList->currentRow(), numrows = movieCueList->count(); ;
  if (row == numrows-1) {
    QMessageBox::warning(this,  "Error",
                         "Attempted to move the bottom item down in the list");
    return; 
  }

  MovieCue *theCue = dynamic_cast<MovieCue *>(movieCueList->takeItem(row));   
  movieCueList->insertItem(row+1, theCue); 
  movieCueList->setCurrentRow(row+1); 
  cueFileDirty(true); 
  return; 
}

//======================================================================
void MovieCueManager::on_moveToBottomButton_clicked(){
  int row = movieCueList->currentRow(), numrows = movieCueList->count(); ;
  if (row == numrows-1) {
    QMessageBox::warning(this,  "Error",
                         "Attempted to move the bottom item down in the list");
    return; 
  }

  MovieCue *theCue = dynamic_cast<MovieCue *>(movieCueList->takeItem(row));   
  movieCueList->insertItem(numrows-1, theCue); 
  movieCueList->setCurrentRow(numrows-1); 

  cueFileDirty(true); 
  return; 

}

//======================================================================
void MovieCueManager::EnableDisableFields(bool enable) {
  cueNameField->setEnabled(enable); 
  cueNameLabel->setEnabled(enable); 
  movieNameField->setEnabled(enable && loadCheckBox->isChecked()); 
  loadCheckBox->setEnabled(enable); 
  playCheckBox->setEnabled(enable); 
  repeatOnceCheckBox->setEnabled(enable); 
  repeatForeverCheckBox->setEnabled(enable); 
  pingpongCheckBox->setEnabled(enable); 
  backwardCheckBox->setEnabled(enable); 
  showControlsCheckBox->setEnabled(enable); 
  
  currentFrameLabel->setEnabled(enable); 
  currentFrameField->setEnabled(enable); 
  startFrameLabel->setEnabled(enable); 
  startFrameField->setEnabled(enable); 
  setStartCurrentButton->setEnabled(enable); 
  resetStartButton->setEnabled(enable); 
  endFrameLabel->setEnabled(enable); 
  endFrameField->setEnabled(enable); 
  setEndCurrentButton->setEnabled(enable); 
  resetEndButton->setEnabled(enable); 
  
  windowHeightLabel->setEnabled(enable); 
  windowWidthLabel->setEnabled(enable); 
  fullScreenCheckBox->setEnabled(enable);
  sizeToMovieCheckBox->setEnabled(enable);
  noStereoCheckBox->setEnabled(enable);
  zoomOneCheckBox->setEnabled(enable);
  zoomToFitCheckBox->setEnabled(enable);
  zoomLabel->setEnabled(enable); 
  zoomField->setEnabled(enable &&  !zoomToFitCheckBox->isChecked() && !zoomOneCheckBox->isChecked()); 
  
  windowXPosLabel->setEnabled(enable); 
  windowYPosLabel->setEnabled(enable); 
  
  imageXPosLabel->setEnabled(enable); 
  imageYPosLabel->setEnabled(enable); 
  imageXPosField->setEnabled(enable); 
  imageYPosField->setEnabled(enable); 

  LODLabel ->setEnabled(enable); 
  LODField ->setEnabled(enable); 
    
  frameRateField->setEnabled(enable); 
  frameRateLabel->setEnabled(enable); 

  movieCueLabel->setEnabled(enable); 

  bool enableSize = enable && !sizeToMovieCheckBox->isChecked() && !fullScreenCheckBox->isChecked();     
  
  windowWidthField->setEnabled(enableSize); 
  windowHeightField->setEnabled(enableSize); 
  
  windowXPosField->setEnabled(enableSize); 
  windowYPosField->setEnabled(enableSize); 

  return; 
}


//======================================================================
/* if iCue is NULL, disable and clear the editor */
void MovieCueManager::setupMovieCueEditor(MovieCue *iCue) {
  MovieCue *tmp = iCue; 
  if (!tmp) tmp = new MovieCue; /* if no current cue, then make a blank one */

  cueNameField->setText(tmp->text()); 
  movieNameField->setText(tmp->mMovieName); 
  QString nums; 

  currentFrameField->setText(nums.setNum(tmp->mCurrentFrame+1)); 
  startFrameField->setText(nums.setNum(tmp->mStartFrame+1)); 
  endFrameField->setText(nums.setNum(tmp->mEndFrame+1)); 
  windowWidthField->setText(nums.setNum(tmp->mWindowWidth)); 
  windowHeightField->setText(nums.setNum(tmp->mWindowHeight)); 
  windowXPosField->setText(nums.setNum(tmp->mWindowXPos)); 
  windowYPosField->setText(nums.setNum(tmp->mWindowYPos)); 
  imageXPosField->setText(nums.setNum(tmp->mImageXPos)); 
  imageYPosField->setText(nums.setNum(tmp->mImageYPos)); 
  LODField->setText(nums.setNum(tmp->mLOD)); 
  frameRateField->setText(nums.setNum(tmp->mFrameRate)); 
  zoomField->setText(nums.setNum(tmp->mZoom)); 

  loadCheckBox->setChecked(tmp->mLoadMovie); 
  browseButton->setEnabled(loadCheckBox->isChecked());
  playCheckBox->setChecked(tmp->mPlayMovie);
  pingpongCheckBox->setChecked(tmp->mPingPong);
  repeatOnceCheckBox->setChecked(tmp->mRepeatFrames == 1);
  repeatForeverCheckBox->setChecked(tmp->mRepeatFrames == -1);
  backwardCheckBox->setChecked(tmp->mPlayBackward);
  showControlsCheckBox->setChecked(tmp->mShowControls);
  applyChangesButton->setEnabled(false); 
  fullScreenCheckBox->setChecked(tmp->mFullScreen);
  sizeToMovieCheckBox->setChecked(tmp->mSizeToMovie);
  noStereoCheckBox->setChecked(tmp->mNoStereo);
  zoomOneCheckBox->setChecked(tmp->mZoomOne);
  zoomToFitCheckBox->setChecked(tmp->mZoomToFit);
 
  EnableDisableFields(iCue != NULL);  // this goes last

  cueNameField->selectAll(); 
  cueNameField->setFocus();

  if (!iCue) delete tmp; /* tmp was created if iCue was NULL */

  SetCueUnchanged();
  return; 
}

//======================================================================
void MovieCueManager::setCurrentCue(MovieCue *iCue) {
  mCurrentCue = iCue; 
  if (iCue) {
    movieCueList->setCurrentItem(mCurrentCue, QItemSelectionModel::ClearAndSelect); 
  }
  setupMovieCueEditor(mCurrentCue); 
  emit currentCueChanged(mCurrentCue); 
  return;
}

//======================================================================
void MovieCueManager::on_movieCueList_itemSelectionChanged() {
  if (cueChanged()) {
    int discard =  QMessageBox::
      question( this,
                tr("Current Cue Changed"),
                tr("You have edited the current movie cue."
                   "Do you want to continue and lose your changes?"),
                QMessageBox::No, QMessageBox::Yes);
    printf("Dialog returned %d\n", discard); 
    if (discard == QMessageBox::No) {
      movieCueList->blockSignals(true); 
      movieCueList->setCurrentItem(mCurrentCue); 
      movieCueList->blockSignals(false); 
      return; 
    }
  }
  QList<QListWidgetItem *> theItems = movieCueList->selectedItems();
  if (theItems.size() == 1) {
    setCurrentCue(dynamic_cast<MovieCue *>( theItems[0])); 
  } else {
    setCurrentCue(NULL);    
  }  
  bool enable = (mCurrentCue != NULL); 
  int row = movieCueList->currentRow(), numrows = movieCueList->count(); 
  moveToBottomButton->setEnabled(enable && (row != numrows - 1)); 
  moveDownButton->setEnabled(enable && (row != numrows - 1)); 
  moveUpButton->setEnabled(enable && row); 
  moveToTopButton->setEnabled(enable && row); 
  duplicateCueButton->setEnabled(enable);
  executeCueButton->setEnabled(theItems.size() > 0);
  loopCuesButton->setEnabled(theItems.size() > 0);
  deleteCueButton->setEnabled(theItems.size() > 0); 
  emit currentCueChanged(mCurrentCue); 
  return;
}


//======================================================================
void MovieCueManager::on_newCueButton_clicked(){
  if (cueChanged()) {
    int discard =  QMessageBox::
      question( this,
                tr("Current Cue Changed"),
                tr("You have edited the current movie cue."
                   "Do you want to continue and lose your changes?"),
                QMessageBox::No, QMessageBox::Yes);
    printf("Dialog returned %d\n", discard); 
    if (discard == QMessageBox::No) {
      return; 
    }
  }
  setCurrentCue(new MovieCue("My Movie Cue", movieCueList)); 
  cueFileDirty(true); 
  return; 
} 

//======================================================================
void MovieCueManager::on_duplicateCueButton_clicked(){
  //  MovieCue *cue = new MovieCue; 
  //*cue = *mCurrentCue; 
  setCurrentCue(new MovieCue(mCurrentCue, movieCueList)); 
  cueFileDirty(true); 
  return; 
} 


//======================================================================
/*! 
  Helper for Validate() function
*/ 
void MovieCueManager::fixOrder(QLineEdit *smaller, QString smalltext, 
                               QLineEdit *greater, QString bigtext) {
  if (smaller->text().toLong() > greater->text().toLong() ) {
    QMessageBox mbox (this); 
    mbox.setText(smalltext + " cannot be greater than " + bigtext); 
    mbox.setInformativeText("What would you like to do?");  
    QAbstractButton *fixSmaller =  
      mbox.addButton(QString("Make %1 equal %2").arg(smalltext).arg(bigtext), QMessageBox::YesRole);
    mbox.addButton(QString("Make %1 equal %2").arg(bigtext).arg(smalltext), QMessageBox::YesRole); 
    mbox.exec(); 

    if (mbox.clickedButton() == fixSmaller) {
      smaller->setText(greater->text()); 
    } else {
      greater->setText(smaller->text()); 
    }
  }
  return; 
}

//======================================================================
void MovieCueManager::Validate() {
  if (endFrameField->text().toLong() != 0) {
    fixOrder(startFrameField, "start frame", endFrameField, "end frame");
    fixOrder(currentFrameField, "current frame", endFrameField, "end frame");
  }
  fixOrder(startFrameField, "start frame", currentFrameField, "current frame");
  
  return; 
}

//======================================================================
/* store the current cue into our list in memory -- does NOT save to disk */
void MovieCueManager::on_applyChangesButton_clicked(){

  Validate(); 
  mCurrentCue->setText(cueNameField->text()); 
  mCurrentCue->mMovieName = movieNameField->text(); 

  mCurrentCue->mCurrentFrame = currentFrameField->text().toInt()-1; 
  mCurrentCue->mStartFrame = startFrameField->text().toInt()-1; 
  mCurrentCue->mEndFrame = endFrameField->text().toInt()-1; 

  mCurrentCue->mWindowWidth = windowWidthField->text().toInt(); 
  mCurrentCue->mWindowHeight = windowHeightField->text().toInt(); 
  mCurrentCue->mWindowXPos = windowXPosField->text().toInt(); 
  mCurrentCue->mWindowYPos = windowYPosField->text().toInt(); 
  mCurrentCue->mImageXPos = imageXPosField->text().toInt(); 
  mCurrentCue->mImageYPos = imageYPosField->text().toInt(); 
  mCurrentCue->mLOD = LODField->text().toInt(); 
  mCurrentCue->mFullScreen = fullScreenCheckBox->isChecked(); 
  mCurrentCue->mSizeToMovie = sizeToMovieCheckBox->isChecked(); 
  mCurrentCue->mNoStereo = noStereoCheckBox->isChecked(); 
  mCurrentCue->mFrameRate = frameRateField->text().toFloat(); 
  mCurrentCue->mZoom = zoomField->text().toFloat(); 
  mCurrentCue->mZoomOne = zoomOneCheckBox->isChecked(); 
  mCurrentCue->mZoomToFit = zoomToFitCheckBox->isChecked(); 

  mCurrentCue->mLoadMovie = (loadCheckBox->isChecked());
  mCurrentCue->mPlayMovie = (playCheckBox->isChecked());
  if  (repeatOnceCheckBox->isChecked()) {
    mCurrentCue->mRepeatFrames = 1;
  } else if  (repeatForeverCheckBox->isChecked()) {
    mCurrentCue->mRepeatFrames = -1;
  } else {
    mCurrentCue->mRepeatFrames = 0;
  }
  mCurrentCue->mPingPong = (pingpongCheckBox->isChecked());
  mCurrentCue->mPlayBackward = (backwardCheckBox->isChecked());
  mCurrentCue->mShowControls = (showControlsCheckBox->isChecked());

  applyChangesButton->setEnabled(false); 
 
  SetCueUnchanged(); 
  
  cueFileDirty(true); 
  return; 
}
//======================================================================
/* what is means to "execute" a cue depends on our context; let our parent subscribe to this signal and deal with it */
void MovieCueManager::on_executeCueButton_clicked() {
  if (mBlockExecution) return; 
  if (executeCueButton->text() != "Execute") {
    setCueRunning(false); 
    mStopLooping = true; 
    return; 
  }

  setCueRunning(true); 

  
  int i = 0, numCues = movieCueList->count(); 
  if (!numCues) {
    ERROR("Programming error:  execute button was clicked, but there are no cues..."); 
    return; 
  }
  mStopLooping = false; 
  while (!mStopLooping && i < movieCueList->count()) {    
    MovieCue *theCue = dynamic_cast<MovieCue *>(movieCueList->item(i)); 
    if (theCue->isSelected()) {
      DEBUGMSG(QString("Executing cue %1 of %2: %3").arg(i).arg(movieCueList->count()).arg(theCue->text()) ); 
      //mExecutingCue = theCue; 
      emit executeCue(theCue); // executeCue needs to be synchronous
    }
    ++i;
  } 

  DEBUGMSG("Done executing cues"); 
  mStopLooping = true; 
  setCueRunning(false); 
  //mExecutingCue = NULL; 
  return; 
}


//======================================================================
/* what is means to "execute" a cue depends on our context; let our parent subscribe to this signal and deal with it */
void MovieCueManager::on_loopCuesButton_clicked() {

  setCueRunning(true); 
  mStopLooping = false; 
  // need to set up a dialog and start looping -- right now this just 
  while (!mStopLooping) {
    int i = 0; 
    while (i < movieCueList->count()) {
      MovieCue *theCue = dynamic_cast<MovieCue *>(movieCueList->item(i)); 
      if (theCue->isSelected()) {
        //mExecutingCue = theCue; 
        emit executeCue(theCue); // executeCue needs to call ProcessEvents to maintain interactivity, of course, and to allow user to cancel
      }
      ++i;
    }
  }
  // re-enable whatever buttons are appropriate for the selection:
  setCueRunning(false); 
  mStopLooping = true; 
  //mExecutingCue = NULL; 
  return; 
}

//======================================================================
/*! 
  Similarly, pass on taking snapshots to the parent
*/ 
void MovieCueManager::on_takeSnapshotButton_clicked() {
  emit takeSnapshot(); 
  return; 
}

//======================================================================
/* When Enter is pressed, it's the same as if a cue button is double-clicked */ 
void MovieCueManager::listItemSlot(QListWidgetItem *) {
  dbprintf(5, "listItemSlot\n"); ; 
  on_executeCueButton_clicked();
  return; 
}
//======================================================================
/* When Enter is pressed, it's the same as if a cue button is double-clicked */ 
void MovieCueManager::cueActivatedSlot(const QModelIndex & ) {
  cerr << "cueActivatedSlot" << endl; 
  on_executeCueButton_clicked();
  return; 
}
//======================================================================
void MovieCueManager::on_readCueFileButton_clicked(){
  QString filename = QFileDialog::
    getOpenFileName(this, "Choose a cue file to open",
                    "",
                    "Cue Files (*.cus *.cues *.cue)");
  if (filename != "") {
    ReadCueFile(filename.toStdString()); 
  }
  return; 
}

//======================================================================
void MovieCueManager::ReadCueFile(std::string filename) {
  bool cuesDirty = false; // if changes were made while reading

  QFile theFile(filename.c_str()); 
  if (!theFile.open( QIODevice::ReadOnly)) {
    QMessageBox::warning(this,  "Error",
                         "The file could not be opened");
    theFile.close(); 
    theFile.setFileName(""); 
    return; 
  }
  
  // things look ok, save the filename for later use
  mCueFileName = filename.c_str(); 
  
  // clear the cue queue:
  setCurrentCue(NULL); 
  movieCueList->clear(); 
  
  // read the cues into our list
  //QDataStream stream(&theFile); 
  MovieCue *cue = new MovieCue; 
  do  {
    try {
      if (theFile.atEnd()) {
        //dbprintf(5, "EOF encountered\n"); 
        break; 
      }
      theFile >> *cue; 
    } catch (QString errmsg) {
      if (cue->mEOF) {
        //dbprintf(5, "EOF encountered 2\n"); 
        break; 
      } else {
        dbprintf(0, qPrintable(errmsg+"\n")); 
      }
    }
    if (cue->isValid) {
      if (cue->mFrameRate < 0.2) {
        cue->mFrameRate = 0.2; 
        QMessageBox::warning
          (NULL,  "Warning",
           QString("Cue \"%1\" has a frame rate less than 0.2.\nAdjusting to 0.2")
           .arg(cue->text()));
        cuesDirty = true; 
      }
      movieCueList->addItem(cue); 
      cue = new MovieCue;
    } 
  } while  (cue->isValid);
  
  delete cue;
  
  setWindowTitle(QString("Movie Cue Manager: ")+filename.c_str()); 
  
  cueFileDirty(cuesDirty); 
  return; 
}

//======================================================================
void MovieCueManager::SetCurrentCue(MovieSnapshot &snapshot) {
  if (!mCurrentCue) {
    return ;
  }
  if (snapshot.mSnapshotType == "MOVIE_SNAPSHOT_STARTFRAME") {
    startFrameField->setText(QString("%1").arg(snapshot.mFrameNumber+1)); 
  } else  if (snapshot.mSnapshotType == "MOVIE_SNAPSHOT_ENDFRAME") {
    endFrameField->setText(QString("%1").arg(snapshot.mFrameNumber+1)); 
  } else  if (snapshot.mSnapshotType == "MOVIE_SNAPSHOT_ALT_ENDFRAME") {
    cerr << "setting endframefield to " << snapshot.mNumFrames << endl; 
    endFrameField->setText(QString("%1").arg(snapshot.mNumFrames)); 
  } else {
    movieNameField->setText(snapshot.mFilename.c_str()); 
    // frameRateField->setText(QString("%1").arg(snapshot.mFrameRate)); 
    backwardCheckBox->setChecked(snapshot.mPlayStep < 0); 
    fullScreenCheckBox->setChecked(snapshot.mFullScreen); 
    sizeToMovieCheckBox->setChecked(snapshot.mSizeToMovie); 
    zoomField->setText(QString("%1").arg(snapshot.mZoom)); 
    zoomOneCheckBox->setChecked(false); 
    zoomToFitCheckBox->setChecked(snapshot.mZoomToFit); 
    startFrameField->setText(QString("%1").arg(snapshot.mStartFrame+1)); 
    currentFrameField->setText(QString("%1").arg(snapshot.mFrameNumber+1));
    endFrameField->setText(QString("%1").arg(snapshot.mEndFrame+1));    
    repeatOnceCheckBox->setChecked(snapshot.mRepeat == 1); 
    repeatForeverCheckBox->setChecked(snapshot.mRepeat == -1); 
    pingpongCheckBox->setChecked(snapshot.mPingPong); 
    windowWidthField->setText(QString("%1").arg(snapshot.mScreenWidth)); 
    windowHeightField->setText(QString("%1").arg(snapshot.mScreenHeight)); 
    windowXPosField->setText(QString("%1").arg(snapshot.mScreenXpos)); 
    windowYPosField->setText(QString("%1").arg(snapshot.mScreenYpos)); 
    imageXPosField->setText(QString("%1").arg(snapshot.mImageXpos)); 
    imageYPosField->setText(QString("%1").arg(snapshot.mImageYpos)); 
    LODField->setText(QString("%1").arg(snapshot.mLOD)); 
    EnableDisableFields(true); 
  }
  return; 
}


//======================================================================
void MovieCueManager::on_saveCuesButton_clicked(){
  if (mCueFileName == "") {
    on_saveCuesAsButton_clicked(); // this calls us back again!  careful
    return; 
  }
  
  QFile theFile(mCueFileName); 
  if (!theFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    QMessageBox::warning(this,  "Error",
                         "The  cue file could not be opened for writing");    
    return;
  }

  // the file is open and ready for writing
  //QDataStream stream(&theFile); 
  int row = 0; 
  MovieCue *theCue = NULL; 
  do {
    theCue = dynamic_cast<MovieCue *>(movieCueList->item(row)); 
    if (theCue){
      try {
        theFile << *theCue; 
      } catch (QString errmsg) {
        QMessageBox::warning(this,  "Error in on_saveCuesButton_clicked",
                             errmsg  );    
        return;     
      }
    }
    ++row; 
  } while (theCue); 
  theFile.close(); 
  cueFileDirty(false); 
  return; 
}

//======================================================================
void MovieCueManager::on_saveCuesAsButton_clicked(){
  QString filename = QFileDialog::
    getSaveFileName(this, "Choose a cue file name",
                    "cuefile.cues",
                    "Cue Files (*.cus *.cues *.cue)");

  if (filename == "") return; 
  if (!(filename.endsWith(".cues") || filename.endsWith(".cus") || filename.endsWith(".cue"))) {
    filename += ".cues";
  }
  mCueFileName = filename; 
  on_saveCuesButton_clicked(); 
  
  return; 
} 

//======================================================================
void MovieCueManager::on_playCheckBox_clicked(){
  if (mCurrentCue)  mPlayChanged = mCurrentCue->mPlayMovie ^ playCheckBox->checkState();
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 

//======================================================================
void MovieCueManager::on_repeatOnceCheckBox_clicked(){
  mRepeatOnceChanged = (mCurrentCue && ((mCurrentCue->mRepeatFrames == 1) ^ repeatOnceCheckBox->isChecked()));
  if (repeatOnceCheckBox->isChecked()) {
    repeatForeverCheckBox->setChecked(false); 
    pingpongCheckBox->setChecked(false); 
  }
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 

//======================================================================
void MovieCueManager::on_repeatForeverCheckBox_clicked(){
  mRepeatForeverChanged = (mCurrentCue && ((mCurrentCue->mRepeatFrames == -1) ^ repeatForeverCheckBox->isChecked()));
  if (repeatForeverCheckBox->isChecked()) {
    repeatOnceCheckBox->setChecked(false); 
    pingpongCheckBox->setChecked(false); 
  }
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 

//======================================================================
void MovieCueManager::on_pingpongCheckBox_clicked(){
  if (mCurrentCue)  mPingPongChanged = mCurrentCue->mPingPong ^ pingpongCheckBox->checkState();
  if (pingpongCheckBox->isChecked()) {
    repeatForeverCheckBox->setChecked(false); 
    repeatOnceCheckBox->setChecked(false); 
  }
  applyChangesButton->setEnabled(cueChanged()); 
  
  return; 
} 


//======================================================================
void MovieCueManager::on_backwardCheckBox_clicked(){
  if (mCurrentCue)  mBackwardChanged = mCurrentCue->mPlayBackward ^ backwardCheckBox->checkState();
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 

//======================================================================
void MovieCueManager::on_showControlsCheckBox_clicked(){
  if (mCurrentCue)  mShowChanged = mCurrentCue->mShowControls ^ showControlsCheckBox->checkState();
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 
//======================================================================
void MovieCueManager::on_loadCheckBox_clicked(){
  if (mCurrentCue)  mLoadMovieChanged = mCurrentCue->mLoadMovie ^ loadCheckBox->isChecked();
  applyChangesButton->setEnabled(cueChanged()); 
  movieNameField->setEnabled(loadCheckBox->isChecked());
  browseButton->setEnabled(loadCheckBox->isChecked());
  return;  
} 
//======================================================================
void MovieCueManager::on_browseButton_clicked(){
  QString filename = QFileDialog::
    getOpenFileName(this, "Choose a movie file",
                    "",
                    "Movie Files (*.sm)");
  if (filename != "") {
    smBase *sm = smBase::openFile(filename.toStdString().c_str(), O_RDONLY, 1); 
    if (!sm) {
      throw str(boost::format("Cannot open SM file %s")%filename.toStdString()); 
    }
    float fps = sm->getFPS(); 
    frameRateField->setText(str(boost::format("%1%")%fps).c_str()); 
    movieNameField->setText(filename);
  } 
  return; 
} 

//======================================================================
void MovieCueManager::on_cueNameField_textChanged(){
  if (mCurrentCue)  mCueNameChanged = mCurrentCue->text() != cueNameField->text();
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 
//======================================================================
void MovieCueManager::on_movieNameField_textChanged(){
  if (mCurrentCue)  mMovieNameChanged = mCurrentCue->mMovieName != movieNameField->text();
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 
//======================================================================
void MovieCueManager::on_currentFrameField_textChanged(){  
  if (mCurrentCue)  mCurrentChanged = mCurrentCue->mCurrentFrame+1 != currentFrameField->text().toLong();
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 
//======================================================================
void MovieCueManager::on_startFrameField_textChanged(){  
  if (mCurrentCue)  mStartChanged = mCurrentCue->mStartFrame+1 != startFrameField->text().toLong();
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 
//======================================================================
void MovieCueManager::on_endFrameField_textChanged(){  
  if (mCurrentCue)  {
    mEndChanged = mCurrentCue->mEndFrame+1 != endFrameField->text().toLong();
  }
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 
//=======================================================================
void MovieCueManager::on_fullScreenCheckBox_clicked(){
  if (mCurrentCue)  mFullScreenChanged = (mCurrentCue->mFullScreen != fullScreenCheckBox->isChecked()); 
  EnableDisableFields(true); 
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
}

//=======================================================================
void MovieCueManager::on_sizeToMovieCheckBox_clicked(){
  if (mCurrentCue)  mSizeToMovieChanged = (mCurrentCue->mSizeToMovie != sizeToMovieCheckBox->isChecked()); 
  EnableDisableFields(true); 
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
}

//=======================================================================
void MovieCueManager::on_noStereoCheckBox_clicked(){
  if (mCurrentCue)  mNoStereoChanged = (mCurrentCue->mNoStereo != noStereoCheckBox->isChecked()); 
  EnableDisableFields(true); 
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
}

//=======================================================================
void MovieCueManager::on_zoomOneCheckBox_clicked(){
  if (mCurrentCue)  mZoomOneChanged = (mCurrentCue->mZoomOne != zoomOneCheckBox->isChecked()); 

  if (zoomOneCheckBox->isChecked()) {
    zoomToFitCheckBox->setChecked(false); 
  }
    
  EnableDisableFields(true); 
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
}

//=======================================================================
void MovieCueManager::on_zoomToFitCheckBox_clicked(){
  if (mCurrentCue)  mZoomToFitChanged = (mCurrentCue->mZoomToFit != zoomToFitCheckBox->isChecked()); 

  if (zoomToFitCheckBox->isChecked()) {
    zoomOneCheckBox->setChecked(false); 
  }
  EnableDisableFields(true); 
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
}
//======================================================================
void MovieCueManager::on_windowWidthField_textChanged(){
  if (mCurrentCue)  mWindowWidthChanged = (mCurrentCue->mWindowWidth != windowWidthField->text().toLong());
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 
//======================================================================
void MovieCueManager::on_windowHeightField_textChanged(){  
  if (mCurrentCue)  mWindowHeightChanged = (mCurrentCue->mWindowHeight != windowHeightField->text().toLong());
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 
//======================================================================
void MovieCueManager::on_windowXPosField_textChanged(){
  if (mCurrentCue)  mWindowXChanged = (mCurrentCue->mWindowXPos != windowXPosField->text().toLong());
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 
//======================================================================
void MovieCueManager::on_windowYPosField_textChanged(){
  if (mCurrentCue)  mWindowYChanged = mCurrentCue->mWindowYPos != windowYPosField->text().toLong();
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 
//======================================================================
void MovieCueManager::on_imageXPosField_textChanged(){
  if (mCurrentCue)  mImageXChanged = mCurrentCue->mImageXPos != imageXPosField->text().toLong();
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 
//======================================================================
void MovieCueManager::on_imageYPosField_textChanged(){
  if (mCurrentCue)  mImageYChanged = mCurrentCue->mImageYPos != imageYPosField->text().toLong();
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 

//======================================================================
void MovieCueManager::on_LODField_textChanged(){
  if (mCurrentCue)  mLODChanged = mCurrentCue->mLOD != LODField->text().toLong();
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 

//======================================================================
void MovieCueManager::on_frameRateField_textChanged(){
  if (mCurrentCue)  mFrameRateChanged = mCurrentCue->mFrameRate != frameRateField->text().toLong();
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 

//======================================================================
void MovieCueManager::on_zoomField_textChanged(){
  if (mCurrentCue)  mZoomChanged = mCurrentCue->mZoom != zoomField->text().toLong();
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 

//======================================================================
void MovieCueManager::on_resetStartButton_clicked() {
  if (mCurrentCue && mCurrentCue->mStartFrame != 1) {
    //    mCurrentCue->mStartFrame = 1;
    startFrameField->setText("1"); 
    // mStartChanged = true; 
  }
  return; 
}

//======================================================================
QTcpSocket &operator << (QTcpSocket &iSocket, const MovieCue &iCue){
  dbprintf(5, "Writing MovieCue to socket\n"); 
  MovieScript script; 
  iCue.GenerateScript(script); 
  std::vector<MovieEvent>::iterator current = script.begin(), end = script.end(); 
  int num = 0; 
  while (current != end && iSocket.isWritable()) {
    try {
      dbprintf(5, "Writing event %d\n", num);; 
      iSocket << *(current++); 
    } catch (QString errmsg) {
      errmsg = QString("Error sending cue #")+QString::number(num)+": "+errmsg;
      throw(errmsg);
    }
    ++num; 
  }
  return iSocket; 
}

//======================================================================
QTcpSocket  &operator >> (QTcpSocket &iSocket,  MovieCue &iCue){
  dbprintf(5, "Reading MovieCue from socket\n"); 
  MovieScript script; 
  MovieEvent event; 
  iCue.isValid = false; 
  int eventnum = 0; 
  while (event.mEventType != "MOVIE_CUE_END" && !iSocket.atEnd()) {
    try {
      dbprintf(5, "Reading event %d\n", eventnum ); 
      iSocket >> event;   
      script.push_back(event); 
    } catch (QString errmsg) {
      if (eventnum == 0) {  
        iCue.mEOF = true;  // assume that we have hit EOF
        return iSocket; 
      } else {
        errmsg = QString("Error receiving cue event ")+event.mEventType.c_str()+": "+errmsg;
        throw(errmsg);
      }
    }
    ++eventnum;
  }
  if (event.mEventType == "MOVIE_CUE_END") iCue.isValid = true; 
  iCue.ReadScript(script); 
  return iSocket; 
}

//======================================================================
QFile &operator << (QFile &iFile, const MovieCue &iCue){  
  iFile.write(QString("BEGINCUE CueName=%1 ").arg(iCue.text()).toStdString().c_str());
  if (iCue.mLoadMovie && iCue.mMovieName != "") {
    iFile.write(QString("LoadMovie=%1 ") 
                .arg(iCue.mMovieName).toStdString().c_str());
  }
  iFile.write(str(boost::format("Play=%d Loop=%d Backward=%d Controls=%d CurrentFrame=%d StartFrame=%d EndFrame=%d WindowWidth=%d WindowHeight=%d WindowX=%d WindowY=%d FullScreen=%d SizeToMovie=%d ImageX=%d ImageY=%d LOD=%d Rate=%0.5f Zoom=%0.5f ZoomOne=%d ZoomToFit=%d PingPong=%d NoStereo=%d ENDCUE\n")
                  %(iCue.mPlayMovie)
                  %(iCue.mRepeatFrames)
                  %(iCue.mPlayBackward)
                  %(iCue.mShowControls)
                  %(iCue.mCurrentFrame)
                  %(iCue.mStartFrame)
                  %(iCue.mEndFrame)
                  %(iCue.mWindowWidth)
                  %(iCue.mWindowHeight)
                  %(iCue.mWindowXPos)
                  %(iCue.mWindowYPos)
                  %(iCue.mFullScreen)
                  %(iCue.mSizeToMovie)
                  %(iCue.mImageXPos)
                  %(iCue.mImageYPos)
                  %(iCue.mLOD)
                  %(iCue.mFrameRate)
                  %(iCue.mZoom)
                  %(iCue.mZoomOne)
                  %(iCue.mZoomToFit)
                  %(iCue.mPingPong)
                  %(iCue.mNoStereo)).c_str()); 
  return iFile; 
  
  
}

//======================================================================
QFile  &operator >> (QFile &iFile,  MovieCue &iCue){
  
  QString line, token;
  QStringList tokens, tokenpair;                       
  QStringList::iterator pos; 
  iCue.isValid = false; 
  if (iFile.atEnd()) {
    iCue.mEOF = true;
    throw QString("EOF"); 
  }
  line = iFile.readLine();
  // dbprintf(5, "First line is \"%s\"\n", line.toStdString().c_str()); 
  if (line.endsWith("\n")) {
    line.chop(1); 
    // dbprintf(5, "Chopped to \"%s\"\n", line.toStdString().c_str()); 
  }
  tokens = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);              
  pos = tokens.begin(); 
  // dbprintf(5, "first token \"%s\"\n", pos->toStdString().c_str()); 
  if (*pos != "BEGINCUE") {
    throw QString("Bad cue -- BEGINCUE not found"); 
  }
  ++pos; 
  while (1) {   
    while (pos != tokens.end()) {
      // dbprintf(5, "next token: \"%s\"\n", pos->toStdString().c_str()); 
      if (*pos == "ENDCUE") {
        iCue.isValid = true; 
        return iFile; 
      }
      tokenpair = pos->split("="); 
      if (!tokenpair.size()) {
        ++pos;       
        continue; 
      }
      if (tokenpair.size() != 2) {
        throw QString("unexpected token: \"%1\"").arg(*pos); 
      }
      // dbprintf(5, "tokenpair is (%s, %s)\n", tokenpair[0].toStdString().c_str(), tokenpair[1].toStdString().c_str()); 
      if (tokenpair[0] == "LoadMovie" || tokenpair[0] == "CueName") {
        /* This is a complicated case, since name strings can contain spaces, 
           and might or might not be quoted
        */ 
        QString field = tokenpair[0]; // just for clarity
        int namestart = line.indexOf(field);  
        if (namestart == -1) {
          throw QString("programming error at %1:%2").arg(__FILE__).arg(__LINE__); 
        }
        namestart += field.size()+1; 

        int nameend; 
        if (line[namestart] == QChar('"')) {
          namestart++; 
          nameend = line.indexOf("\"", namestart);
          if (nameend == -1) {
            throw QString("missing terminal quotation mark around string"); 
          }
          nameend -= 1; 
        } else {
          nameend = line.indexOf("=", namestart);
          if (nameend != -1) {
            // get the previous space
            nameend = line.lastIndexOf(" ", nameend); 
          } 
        }
        if (nameend < namestart) { 
          throw QString("programming error at %1:%2").arg(__FILE__).arg(__LINE__); 
        }
        /* if no quote or "=" found, then nameend = -1, 
           which gets the rest of the line, which is the correct behavior 
        */ 
      
        QString value = line.mid(namestart, nameend-namestart+1);
        while (value.endsWith(" ")) value.chop(1); 

        // dbprintf(5, "%s is \"%s\"\n", field.toStdString().c_str(), value.toStdString().c_str()); 
        if (tokenpair[0] == "LoadMovie") {
          iCue.mLoadMovie = true; 
          iCue.mMovieName = value; 
        } else {
          iCue.setText(value);
        } 
        /* If there are spaces in the name, tokens would be all screwed
           up at this point, so reset tokens just in case:  
        */ 
        if (nameend != -1) {
          line = line.mid(nameend+1, -1); 
          tokens = line.split(QRegExp("\\s+"), QString::SkipEmptyParts); 
          pos = tokens.begin();          
        } else {
          pos = tokens.end();
        }
        continue; // do not increment pos        
      } else if (tokenpair[0] == "Loop") {
        iCue.mRepeatFrames = (tokenpair[1].toInt()); 
      } else if (tokenpair[0] == "PingPong") {
        iCue.mPingPong = (tokenpair[1].toInt()); 
      } else if (tokenpair[0] == "Play") {
        iCue.mPlayMovie = (tokenpair[1].toInt()); 
      } else if (tokenpair[0] == "Backward") {
        iCue.mPlayBackward = (tokenpair[1].toInt()); 
      } else if (tokenpair[0] == "Controls") {
        iCue.mShowControls = (tokenpair[1].toInt()); 
      } else if (tokenpair[0] == "CurrentFrame") {
        iCue.mCurrentFrame = tokenpair[1].toInt();
      } else if (tokenpair[0] == "StartFrame") {
        iCue.mStartFrame = tokenpair[1].toInt();
      } else if (tokenpair[0] == "EndFrame") {
        iCue.mEndFrame = tokenpair[1].toInt();
      } else if (tokenpair[0] == "WindowWidth" || 
                 tokenpair[0] == "FrameWidth" ) {
        iCue.mWindowWidth = tokenpair[1].toInt();
      } else if (tokenpair[0] == "WindowHeight" || 
                 tokenpair[0] == "FrameHeight" ) {
        iCue.mWindowHeight = tokenpair[1].toInt();
      } else if (tokenpair[0] == "FullScreen") {
        iCue.mFullScreen = (tokenpair[1].toInt()); 
      } else if (tokenpair[0] == "SizeToMovie") {
        iCue.mSizeToMovie = (tokenpair[1].toInt()); 
      } else if (tokenpair[0] == "NoStereo") {
        iCue.mNoStereo = (tokenpair[1].toInt()); 
      } else if (tokenpair[0] == "WindowX" || 
                 tokenpair[0] == "FrameX" ) {
        iCue.mWindowXPos = tokenpair[1].toInt();
      } else if (tokenpair[0] == "WindowY" || 
                 tokenpair[0] == "FrameY" ) {
        iCue.mWindowYPos = tokenpair[1].toInt();
      } else if (tokenpair[0] == "ImageX") {
        iCue.mImageXPos = tokenpair[1].toInt();
      } else if (tokenpair[0] == "ImageY") {
        iCue.mImageYPos = tokenpair[1].toInt();
      } else if (tokenpair[0] == "LOD") {
        iCue.mLOD = tokenpair[1].toInt();
      } else if (tokenpair[0] == "Rate") {
        iCue.mFrameRate = tokenpair[1].toFloat();
      } else if (tokenpair[0] == "Zoom") {
        iCue.mZoom = tokenpair[1].toFloat();
      } else if (tokenpair[0] == "ZoomOne") {
        iCue.mZoomOne = (tokenpair[1].toInt()); 
      } else if (tokenpair[0] == "ZoomToFit" || tokenpair[0] == "ZoomToFill" ) {
        iCue.mZoomToFit = (tokenpair[1].toInt()); 
      } else {
        throw QString("unexpected token: %1").arg(*pos); 
      }
      ++pos; 
    } /* looping over tokens in current line */ 
    if (iFile.atEnd()) {      
      throw QString("Failed to find ENDCUE"); 
    }

    line = iFile.readLine();
    // dbprintf(5, "next line is \"%s\"\n", line.toStdString().c_str()); 
    if (line.endsWith("\n")) {
      line.chop(1); 
      // dbprintf(5, "Chopped to \"%s\"\n", line.toStdString().c_str()); 
    }
    tokens = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);                       
    pos = tokens.begin(); 
    
  } /* looping over lines */ 

  // We should never get here:
  throw QString("Programming error on %1:%2").arg(__FILE__).arg(__LINE__); 
  return iFile; 
}
