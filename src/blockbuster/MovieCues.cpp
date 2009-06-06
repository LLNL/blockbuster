/* 

*/
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
  mZoomOne = other->mZoomOne; 
  mPingPong = other->mPingPong; 
  mCurrentFrame = other->mCurrentFrame; 
  mStartFrame = other ->mStartFrame; 
  mEndFrame = other->mEndFrame;
  mLoopFrames = other->mLoopFrames;
  mWindowWidth = other->mWindowWidth;
  mWindowHeight = other->mWindowHeight;
  mWindowXPos = other->mWindowXPos;
  mWindowYPos = other->mWindowYPos;
  mImageXPos = other->mImageXPos;
  mImageYPos = other->mImageYPos;
  mLOD = other->mLOD;
  mFrameRate = other->mFrameRate;
  mZoom = other->mZoom;
  isValid = other->isValid;
  mEOF = other->mEOF;
  return; 
}

//======================================================================
void MovieCue::ReadScript(const MovieScript &iScript) {
  std::vector<MovieEvent>::const_iterator pos = iScript.begin(), end = iScript.end(); while (pos != end) {
    switch (pos->eventType) {
    case MOVIE_CUE_BEGIN:
      setText(pos->mString); 
      break; 
    case MOVIE_OPEN_FILE_NOCHANGE:
      mLoadMovie = true; 
      mMovieName = pos->mString; 
      mCurrentFrame = pos->number; 
      break; 
    case MOVIE_CUE_PLAY_ON_LOAD: 
      mPlayMovie = pos->height; 
      break; 
    case MOVIE_SET_PINGPONG: 
      mPingPong = pos->height; 
      break; 
    case MOVIE_SET_LOOP: 
      mLoopFrames = pos->height; 
      break; 
    case MOVIE_SHOW_INTERFACE:
	  mShowControls = true;
	  break;
    case MOVIE_HIDE_INTERFACE:
	  mShowControls = false;
	  break;
    case MOVIE_CUE_PLAY_BACKWARD: 
      mPlayBackward = pos->height; 
      break; 
    case MOVIE_GOTO_FRAME:
      mCurrentFrame = pos->number; 
      break; 
    case MOVIE_START_END_FRAMES: 
      mStartFrame = pos->width; 
      mEndFrame = pos->height; 
      break; 
   case MOVIE_ZOOM_SET:
      mZoom = pos->rate;
      break; 
    case MOVIE_SET_RATE: 
	  mFrameRate = pos->rate; 
	  break; 
    case MOVIE_FULLSCREEN:
      mFullScreen = true;
      mWindowXPos = 0;
      mWindowYPos = 0; 
      mWindowWidth = 0; 
      mWindowHeight = 0; 
    case MOVIE_ZOOM_ONE: 
      mZoomOne = true; 
      mWindowXPos = 0;
      mWindowYPos = 0; 
    case MOVIE_MOVE_RESIZE: 
      mFullScreen = false;
      mWindowXPos = pos->x; 
      mWindowYPos = pos->y; 
      mWindowWidth = pos->width; 
      mWindowHeight = pos->height; 
      break; 
    case MOVIE_IMAGE_MOVE: 
      mImageXPos = pos->x; 
      mImageYPos = pos->y; 
      break; 
    case MOVIE_SET_LOD:
      mLOD = pos->x; 
      break; 
    default:
      cerr << "Warning:  unknown event in Cue script: " << pos->eventType << endl; 
      break; 
    }
    ++pos; 
  }
  return; 
}

//======================================================================
void MovieCue::GenerateScript(MovieScript &oScript) const{
  oScript.clear();   
  /* store events pertaining to the user interface */
  oScript.push_back(MovieEvent(MOVIE_CUE_BEGIN, text(), mEndFrame)); 
  oScript.push_back(MovieEvent(MOVIE_CUE_PLAY_ON_LOAD, mPlayMovie));
  if (mShowControls) {
    oScript.push_back(MovieEvent(MOVIE_SHOW_INTERFACE));
  } else {
    oScript.push_back(MovieEvent(MOVIE_HIDE_INTERFACE));
  } 
	
  oScript.push_back(MovieEvent(MOVIE_SET_LOOP, mLoopFrames));
  oScript.push_back(MovieEvent(MOVIE_SET_PINGPONG, mPingPong));
  oScript.push_back(MovieEvent(MOVIE_CUE_PLAY_BACKWARD, mPlayBackward)); 
  oScript.push_back(MovieEvent(MOVIE_START_END_FRAMES, mStartFrame, mEndFrame, 0,0)); 
  /* now store the events that actually make BlockBuster do things */
  if (mLoadMovie && mMovieName != "") {    
    oScript.push_back(MovieEvent(MOVIE_OPEN_FILE_NOCHANGE, mMovieName, mCurrentFrame));
  }  else {
    oScript.push_back(MovieEvent(MOVIE_GOTO_FRAME,mCurrentFrame-1));
  }
  if (mFullScreen) {
    oScript.push_back(MovieEvent(MOVIE_FULLSCREEN)); 
  } else if (mZoomOne) {
    oScript.push_back(MovieEvent(MOVIE_ZOOM_ONE)); 
  } else   {
    oScript.push_back(MovieEvent(MOVIE_MOVE_RESIZE, mWindowWidth, mWindowHeight, mWindowXPos, mWindowYPos)); 
  }
 
  if (!mFullScreen && !mZoomOne) {
    oScript.push_back(MovieEvent(MOVIE_ZOOM_SET, mZoom)); 
  }
  oScript.push_back(MovieEvent(MOVIE_SET_RATE, mFrameRate)); 
  oScript.push_back(MovieEvent(MOVIE_SET_LOD, mLOD)); 
  oScript.push_back(MovieEvent(MOVIE_IMAGE_MOVE, 0,0, mImageXPos, mImageYPos));
  if (mPlayMovie) {
    if (mPlayBackward){
      oScript.push_back(MovieEvent(MOVIE_PLAY_BACKWARD));
    }
    else{
      oScript.push_back(MovieEvent(MOVIE_PLAY_FORWARD));
    }
  }
  /* end of record marker */
  oScript.push_back(MovieEvent(MOVIE_CUE_END)); 
  return; 
}


//======================================================================
MovieCueManager::MovieCueManager(QWidget *parent ) :
  QWidget(parent), mCurrentCue(NULL), mExecutingCue(NULL) {
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
      QMessageBox::critical(this, tr("Save Cue File"),
                            "The cue file has changed.  Save before quitting?",
                            QMessageBox::Save | 
                            QMessageBox::Discard | QMessageBox::Cancel);
    if (reply == QMessageBox::Save){
      on_saveCuesButton_clicked(); 
      return true;
    }  else if (reply == QMessageBox::Discard) {
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
bool MovieCueManager::cueRunning(void) {
  return executeCueButton->text() != "Execute"; 
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
              tr("Are you sure you want to delete the selected Cues?"
                 "There is no undo capability!"),
              QMessageBox::No, QMessageBox::Yes);
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
  loopOnceCheckBox->setEnabled(enable); 
  loopForeverCheckBox->setEnabled(enable); 
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
  zoomOneCheckBox->setEnabled(enable);
  
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
  zoomLabel->setEnabled(enable); 

  movieCueLabel->setEnabled(enable); 

  bool fullscreen = fullScreenCheckBox->isChecked(), zoom = zoomOneCheckBox->isChecked(); 

  windowWidthField->setEnabled(enable && !fullscreen && !zoom); 
  windowHeightField->setEnabled(enable && !fullscreen && !zoom); 

  windowXPosField->setEnabled(enable && !fullscreen); 
  windowYPosField->setEnabled(enable && !fullscreen); 

  zoomField->setEnabled(enable && !fullscreen); 
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
  loopOnceCheckBox->setChecked(tmp->mLoopFrames == 1);
  loopForeverCheckBox->setChecked(tmp->mLoopFrames == -1);
  backwardCheckBox->setChecked(tmp->mPlayBackward);
  showControlsCheckBox->setChecked(tmp->mShowControls);
  applyChangesButton->setEnabled(false); 
  fullScreenCheckBox->setChecked(tmp->mFullScreen);
  zoomOneCheckBox->setChecked(tmp->mZoomOne);
 
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
void MovieCueManager::Validate() {
  /*! 
    Validate that current frame is not less than minimum frame
  */
  if (currentFrameField->text().toLong() <  
      startFrameField->text().toLong()) {
    QMessageBox::warning
      (this, tr("Invalid input"),
       tr("The current frame cannot be less than the start frame."
          "  Setting current frame to start frame."));
    currentFrameField->setText(startFrameField->text());
  }
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
  mCurrentCue->mZoomOne = zoomOneCheckBox->isChecked(); 
  mCurrentCue->mFrameRate = frameRateField->text().toFloat(); 
  mCurrentCue->mZoom = zoomField->text().toFloat(); 

  mCurrentCue->mLoadMovie = (loadCheckBox->isChecked());
  mCurrentCue->mPlayMovie = (playCheckBox->isChecked());
  if  (loopOnceCheckBox->isChecked()) {
    mCurrentCue->mLoopFrames = 1;
  } else if  (loopForeverCheckBox->isChecked()) {
    mCurrentCue->mLoopFrames = -1;
  } else {
    mCurrentCue->mLoopFrames = 0;
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
  if (executeCueButton->text() != "Execute") {
    setCueRunning(false); 
    return; 
  }

  setCueRunning(true); 

  int i = 0; 
  while (i < movieCueList->count() && cueRunning()) {
 
    MovieCue *theCue = dynamic_cast<MovieCue *>(movieCueList->item(i)); 
    if (theCue->isSelected()) {
      mExecutingCue = theCue; 
      emit executeCue(theCue); // executeCue needs to call ProcessEvents to maintain interactivity, of course
    }
    ++i;
  }

  setCueRunning(false); 
  mExecutingCue = NULL; 
  return; 
}


//======================================================================
/* what is means to "execute" a cue depends on our context; let our parent subscribe to this signal and deal with it */
void MovieCueManager::on_loopCuesButton_clicked() {

  setCueRunning(true); 

  // need to set up a dialog and start looping -- right now this just 
  while (cueRunning()) {
    int i = 0; 
    while (i < movieCueList->count()) {
      MovieCue *theCue = dynamic_cast<MovieCue *>(movieCueList->item(i)); 
      if (theCue->isSelected()) {
        mExecutingCue = theCue; 
        emit executeCue(theCue); // executeCue needs to call ProcessEvents to maintain interactivity, of course, and to allow user to cancel
      }
      ++i;
    }
  }
 // re-enable whatever buttons are appropriate for the selection:
  setCueRunning(false); 
  mExecutingCue = NULL; 
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
        fprintf(stderr, qPrintable(errmsg+"\n")); 
      }
    }
    if (cue->isValid) {
      movieCueList->addItem(cue); 
      cue = new MovieCue;
    } 
  } while  (cue->isValid);
  
  delete cue;
  
  setWindowTitle(QString("Movie Cue Manager: ")+filename.c_str()); 
  
  cueFileDirty(false); 
  return; 
}

 //======================================================================
void MovieCueManager::SetCurrentCue(MovieSnapshot &snapshot) {
  if (!mCurrentCue) {
    return ;
  }
  if (snapshot.mSnapshotType == MOVIE_SNAPSHOT_STARTFRAME) {
    startFrameField->setText(QString("%1").arg(snapshot.mFrameNumber+1)); 
  } else  if (snapshot.mSnapshotType == MOVIE_SNAPSHOT_ENDFRAME) {
    endFrameField->setText(QString("%1").arg(snapshot.mFrameNumber+1)); 
  } else  if (snapshot.mSnapshotType == MOVIE_SNAPSHOT_ALT_ENDFRAME) {
    cerr << "setting endframefield to " << snapshot.mNumFrames << endl; 
    endFrameField->setText(QString("%1").arg(snapshot.mNumFrames)); 
  } else {
    movieNameField->setText(snapshot.mFilename); 
    frameRateField->setText(QString("%1").arg(snapshot.mFrameRate)); 
    zoomField->setText(QString("%1").arg(snapshot.mZoom)); 
    backwardCheckBox->setChecked(snapshot.mPlayStep < 0); 
    fullScreenCheckBox->setChecked(snapshot.mFullScreen); 
    zoomOneCheckBox->setChecked(snapshot.mZoomOne); 
    startFrameField->setText(QString("%1").arg(snapshot.mFrameNumber+1)); //use the current frame number, not blockbuster's looping start frame -- Scott says this is the right thing to do
    currentFrameField->setText(QString("%1").arg(snapshot.mFrameNumber+1));
    endFrameField->setText(QString("%1").arg(snapshot.mEndFrame+1));    
    loopOnceCheckBox->setChecked(snapshot.mLoop == 1); 
    loopForeverCheckBox->setChecked(snapshot.mLoop == -1); 
    pingpongCheckBox->setChecked(snapshot.mPingPong); 
    windowWidthField->setText(QString("%1").arg(snapshot.mScreenWidth)); 
    windowHeightField->setText(QString("%1").arg(snapshot.mScreenHeight)); 
    windowXPosField->setText(QString("%1").arg(snapshot.mScreenXpos)); 
    windowYPosField->setText(QString("%1").arg(snapshot.mScreenYpos)); 
    imageXPosField->setText(QString("%1").arg(snapshot.mImageXpos)); 
    imageYPosField->setText(QString("%1").arg(snapshot.mImageYpos)); 
    LODField->setText(QString("%1").arg(snapshot.mLOD)); 
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
void MovieCueManager::on_loopOnceCheckBox_clicked(){
  mLoopOnceChanged = (mCurrentCue && ((mCurrentCue->mLoopFrames == 1) ^ loopOnceCheckBox->isChecked()));
  if (loopOnceCheckBox->isChecked()) {
    loopForeverCheckBox->setChecked(false); 
    pingpongCheckBox->setChecked(false); 
  }
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 

//======================================================================
void MovieCueManager::on_loopForeverCheckBox_clicked(){
  mLoopForeverChanged = (mCurrentCue && ((mCurrentCue->mLoopFrames == -1) ^ loopForeverCheckBox->isChecked()));
  if (loopForeverCheckBox->isChecked()) {
    loopOnceCheckBox->setChecked(false); 
    pingpongCheckBox->setChecked(false); 
  }
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 

//======================================================================
void MovieCueManager::on_pingpongCheckBox_clicked(){
  if (mCurrentCue)  mPingPongChanged = mCurrentCue->mPingPong ^ pingpongCheckBox->checkState();
  if (pingpongCheckBox->isChecked()) {
    loopForeverCheckBox->setChecked(false); 
    loopOnceCheckBox->setChecked(false); 
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
  if (filename != "")
    movieNameField->setText(filename); 
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
  if (mCurrentCue)  mFullScreenChanged = mCurrentCue->mFullScreen ^ fullScreenCheckBox->isChecked(); 
  if (fullScreenCheckBox->isChecked()) zoomOneCheckBox->setChecked(false); 
  EnableDisableFields(true); 
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
}
//=======================================================================
void MovieCueManager::on_zoomOneCheckBox_clicked(){
  if (mCurrentCue)  mZoomOneChanged = mCurrentCue->mZoomOne ^ zoomOneCheckBox->isChecked(); 
  if (zoomOneCheckBox->isChecked()) fullScreenCheckBox->setChecked(false); 
  EnableDisableFields(true); 
  zoomField->setText("1.0");  
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
}
//======================================================================
void MovieCueManager::on_windowWidthField_textChanged(){
  if (mCurrentCue)  mWindowWidthChanged = mCurrentCue->mWindowWidth != windowWidthField->text().toLong();
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 
//======================================================================
void MovieCueManager::on_windowHeightField_textChanged(){  
  if (mCurrentCue)  mWindowHeightChanged = mCurrentCue->mWindowHeight != windowHeightField->text().toLong();
  applyChangesButton->setEnabled(cueChanged()); 
  return; 
} 
//======================================================================
void MovieCueManager::on_windowXPosField_textChanged(){
  if (mCurrentCue)  mWindowXChanged = mCurrentCue->mWindowXPos != windowXPosField->text().toLong();
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
  while (event.eventType != MOVIE_CUE_END && !iSocket.atEnd()) {
    try {
      dbprintf(5, "Reading event %d\n", eventnum ); 
      iSocket >> event;   
      script.push_back(event); 
    } catch (QString errmsg) {
      if (eventnum == 0) {  
        iCue.mEOF = true;  // assume that we have hit EOF
        return iSocket; 
      } else {
        errmsg = QString("Error receiving cue event ")+QString::number(MovieEventTypeToUint32(event.eventType))+": "+errmsg;
        throw(errmsg);
      }
    }
    ++eventnum;
  }
  if (event.eventType == MOVIE_CUE_END) iCue.isValid = true; 
  iCue.ReadScript(script); 
  return iSocket; 
}

//======================================================================
QFile &operator << (QFile &iFile, const MovieCue &iCue){  
  iFile.write(QString("BEGINCUE CueName=%1 ").arg(iCue.text()).toAscii());
  if (iCue.mLoadMovie && iCue.mMovieName != "") {
    iFile.write(QString("LoadMovie=%1 ") 
                .arg(iCue.mMovieName).toAscii());
  }
  iFile.write(QString("Play=%1 Loop=%2 Backward=%3 Controls=%4 CurrentFrame=%5 StartFrame=%6 EndFrame=%7 WindowWidth=%8 WindowHeight=%9 WindowX=%10 WindowY=%11 FullScreen=%12 ZoomOne=%13 ImageX=%14 ImageY=%15 LOD=%16 Rate=%17 Zoom=%18 PingPong=%19 ENDCUE\n")
              .arg(iCue.mPlayMovie)
              .arg(iCue.mLoopFrames)
              .arg(iCue.mPlayBackward)
              .arg(iCue.mShowControls)
              .arg(iCue.mCurrentFrame)
              .arg(iCue.mStartFrame)
              .arg(iCue.mEndFrame)
              .arg(iCue.mWindowWidth)
              .arg(iCue.mWindowHeight)
              .arg(iCue.mWindowXPos)
              .arg(iCue.mWindowYPos)
              .arg(iCue.mFullScreen)
              .arg(iCue.mZoomOne)
              .arg(iCue.mImageXPos)
              .arg(iCue.mImageYPos)
              .arg(iCue.mLOD)
              .arg(iCue.mFrameRate)
              .arg(iCue.mZoom)
              .arg(iCue.mPingPong)
              .toAscii());
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
  tokens = line.split(" ");              
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
          tokens = line.split(" "); 
          pos = tokens.begin();          
        } else {
          pos = tokens.end();
        }
        continue; // do not increment pos        
      } else if (tokenpair[0] == "Loop") {
        iCue.mLoopFrames = (tokenpair[1].toInt()); 
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
      } else if (tokenpair[0] == "ZoomOne") {
        iCue.mZoomOne = (tokenpair[1].toInt()); 
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
    tokens = line.split(" ");                       
    pos = tokens.begin(); 
    
  } /* looping over lines */ 

  // We should never get here:
  throw QString("Programming error on %1:%2").arg(__FILE__).arg(__LINE__); 
  return iFile; 
}
