#ifndef MOVIE_CUES_H
#define MOVIE_CUES_H
#include "QFile"
#include "QTcpSocket"

#include "events.h"
#include <vector>
#include "ui_MovieCueWidget.h"
#include <QKeyEvent>

class MovieCue; 
typedef std::vector<MovieEvent> MovieScript; 


//===========================================================
/* this thing knows how to build a user interface and interact with it to populate a list of MovieCues. */
class MovieCueManager: public QWidget, Ui::MovieCueWidget {
  Q_OBJECT
    public:
  MovieCueManager(QWidget *parent =NULL);
  void blockExecution(void) {mBlockExecution = true; }
  void enableExecuteButton(bool enable){ executeCueButton->setEnabled(enable); }
  void enableSnapshotButton(bool enable) { 
    takeSnapshotButton->setEnabled(enable); 
  }
  const MovieCue *getCurrentCue(void) const { 
    return mCurrentCue; 
  }
  /*  const MovieCue *getExecutingCue(void) const { 
      return mExecutingCue; 
      }
  */
   void ReadCueFile(std::string filename); 
  int numCues(void) { return movieCueList->count(); }

  // Set current cue fields from a blockbuster snapshot 
  void SetCurrentCue(MovieSnapshot &snapshot);
  
  // check if the cues are dirty -- if they are, ask user whether to save
  bool okToQuit(void);
  // check if the current cue has changed -- if so, ask user whether to save
  bool okToCloseWindow(void);
  // Override the widget's closeEvent handler 
  void closeEvent(QCloseEvent *event);

  void setCueRunning(bool running); 
  void stopLooping(void) { mStopLooping = true; }
  bool looping(void) { return !mStopLooping; }

  //=========================================
  // SIGNALS
 signals:
  void closed(); 
  void executeCue(MovieCue* iCue); 
  void stopCurrentCue(); 
  void takeSnapshot(); 
  void currentCueChanged(MovieCue *iCue); 
  void movieSnapshotStartFrame(); 
  void movieSnapshotEndFrame(); 
  void movieSnapshotAltEndFrame(); 
  /* void  endFrameButton_clicked(); 
     void  startFrameButton_clicked(); */
  

  //=========================================
  // SLOTS 
  public slots: 
  // not really a signal, it's a helper for Validate()
  void fixOrder(QLineEdit *smaller, QString smalltext, 
                QLineEdit *greater, QString bigtext);
  void Validate(); 
  void userDoubleClickedItem(QListWidgetItem*) {
    on_executeCueButton_clicked(); 
  }
  void on_movieCueList_itemSelectionChanged();
  void on_moveToTopButton_clicked(); 
  void on_moveUpButton_clicked(); 
  void on_moveDownButton_clicked(); 
  void on_moveToBottomButton_clicked(); 
  void on_newCueButton_clicked(); 
  void on_duplicateCueButton_clicked(); 
  void on_deleteCueButton_clicked(); 
  void on_loopCuesButton_clicked(); 
  void on_deleteAllCuesButton_clicked(); 
  void cueActivatedSlot(const QModelIndex & index); 
  void listItemSlot( QListWidgetItem * index);
  void on_executeCueButton_clicked() ;
  void on_takeSnapshotButton_clicked() ;
  void on_readCueFileButton_clicked();
 
  void on_saveCuesButton_clicked(); // save the cue list to a file (by default the current file)
  void on_saveCuesAsButton_clicked(); // save the cue list to a file (by default the current file)
  void on_applyChangesButton_clicked(); // save the current cue into its slot in mCueQueue (does not save to disk) 

  void on_playCheckBox_clicked(); 
  void on_repeatOnceCheckBox_clicked(); 
  void on_repeatForeverCheckBox_clicked(); 
  void on_pingpongCheckBox_clicked(); 
  void on_backwardCheckBox_clicked(); 
  void on_showControlsCheckBox_clicked(); 
  void on_cueNameField_textChanged(); 
  void on_loadCheckBox_clicked(); 
  void on_browseButton_clicked(); 
  void on_movieNameField_textChanged(); 
  void on_currentFrameField_textChanged(); 
  void on_startFrameField_textChanged(); 
  void on_endFrameField_textChanged(); 

  void on_setStartCurrentButton_clicked() {
    emit movieSnapshotStartFrame();  
    return; 
  }
  void on_resetStartButton_clicked();
  
  void on_setEndCurrentButton_clicked() {
    emit movieSnapshotEndFrame(); 
    return; 
  }
  void on_resetEndButton_clicked() {
    emit movieSnapshotAltEndFrame(); 
    return; 
  }
  //===============================================================
  void on_fullScreenCheckBox_clicked();
  void on_sizeToMovieCheckBox_clicked();
  void on_noStereoCheckBox_clicked();
  void on_windowWidthField_textChanged(); 
  void on_windowHeightField_textChanged(); 
  void on_windowXPosField_textChanged(); 
  void on_windowYPosField_textChanged(); 
  void on_imageXPosField_textChanged(); 
  void on_imageYPosField_textChanged(); 
  void on_LODField_textChanged(); 
  void on_frameRateField_textChanged(); 
  void on_zoomField_textChanged(); 
  void on_zoomOneCheckBox_clicked();
  void on_zoomToFitCheckBox_clicked();
 private:
  void cueFileDirty(bool dirty);
  void SetCueUnchanged(void) {
    mCueNameChanged = mMovieNameChanged = mLoadMovieChanged = 
	  mPlayChanged = mRepeatOnceChanged = mRepeatForeverChanged = mPingPongChanged = 
      mBackwardChanged = mShowChanged = 
      mFullScreenChanged = mSizeToMovieChanged =        
      mNoStereoChanged = mCurrentChanged = mStartChanged = mEndChanged = 
      mWindowWidthChanged = mWindowHeightChanged = mWindowXChanged = mWindowYChanged = 
	  mImageXChanged = mImageYChanged = mLODChanged = mFrameRateChanged = 
      mZoomChanged = mZoomOneChanged = mZoomToFitChanged = false;
    return; 
  }
  bool cueChanged() {
    return mCueNameChanged || mMovieNameChanged || mLoadMovieChanged || mPlayChanged || mRepeatOnceChanged || mRepeatForeverChanged || mPingPongChanged || mBackwardChanged || mShowChanged || mNoStereoChanged || mFullScreenChanged || mSizeToMovieChanged || mCurrentChanged || mStartChanged || mEndChanged || mWindowWidthChanged || mWindowHeightChanged || mWindowXChanged || mWindowYChanged || mImageXChanged || mImageYChanged || mLODChanged || mFrameRateChanged || mZoomChanged || mZoomOneChanged || mZoomToFitChanged; 
  }
  
  void EnableDisableFields(bool enable); 
  void setupMovieCueEditor(MovieCue *); 
  void setCurrentCue(MovieCue *); 
  
  bool mCueNameChanged, mMovieNameChanged, mLoadMovieChanged, mPlayChanged, mRepeatOnceChanged, mRepeatForeverChanged, mPingPongChanged, mBackwardChanged, mShowChanged, mFullScreenChanged, mSizeToMovieChanged, mNoStereoChanged, mCurrentChanged, mStartChanged, mEndChanged, mEndChangedmEndChanged, mWindowWidthChanged, mWindowHeightChanged, mWindowXChanged, mWindowYChanged,  mImageXChanged, mImageYChanged, mLODChanged, mFrameRateChanged, mZoomChanged, mZoomOneChanged, mZoomToFitChanged; 
  bool mCueFileDirty; // cue file needs saving 
  //std::vector<MovieCue> mCueQueue; // I've always wanted to say that
  QString  mCueFileName; 
  MovieCue *mCurrentCue; // *mExecutingCue; 
  bool mStopLooping, // stop looping
    mBlockExecution; // block cues from executing during exit
}; 
//===========================================================
/* the attributes and data associated with a movie cue, for use by MovieCueManager.
   Will know how to talk to a QDataStream for network/file access.  
   It is a QListWidgetItem to make insertion into the list widget easy. 
   This means the cue's name is stored in the QListWidgetItem->text member. 
 */ 
class MovieCue: public QListWidgetItem {
 public: 
  MovieCue(MovieCue *other, QListWidget *parent = NULL);
  MovieCue(QString cueName="My Movie Cue", QListWidget *parent = NULL): 
    QListWidgetItem(cueName, parent), mMovieName("movie.sm"), mLoadMovie(false), 
    mPlayMovie(false), mPlayBackward(false), mShowControls(false), mFullScreen(true), mSizeToMovie(false), mNoStereo(false), mPingPong(false), 
    mCurrentFrame(0), mStartFrame(0), mEndFrame(-1), mRepeatFrames(0), mWindowWidth(0), mWindowHeight(0), mWindowXPos(0), mWindowYPos(0), mImageXPos(0), mImageYPos(0), mLOD(0), mFrameRate(100.0), mZoom(1.0), mZoomOne(false), mZoomToFit(false), isValid(true), mEOF(false)  {     return; }
  QString mMovieName; 
  bool mLoadMovie, mPlayMovie, mPlayBackward, mShowControls, 
    mFullScreen, mSizeToMovie, mNoStereo, mPingPong; 
  int32_t mCurrentFrame, mStartFrame, mEndFrame, // if backwards, then endframe is the actual start frame, of course
    mRepeatFrames, // could be 0 (play once, don't repeat), 1 (repeat once) or -1 (loop forever)
    mWindowWidth, mWindowHeight, // window size
    mWindowXPos, mWindowYPos, //window position
    mImageXPos, mImageYPos, mLOD; // position of movie image in window
  float mFrameRate, mZoom; 
  bool mZoomOne, mZoomToFit;
  QListWidget mListWidget; 

  void ReadScript(const MovieScript &iScript); //populate self from any recognized events in the given script -- ignore unknown events for compatibility with future releases
  void GenerateScript(MovieScript &oScript) const; // generates an ordered series of events to execute, including header and footer events, intended to be sent over a data stream
  bool isValid, mEOF; 
}; 

//===========================================================
 QFile &operator << (QFile &iStream, const MovieCue &iCue);
QFile &operator >> (QFile &iStream,  MovieCue &iCue);
QTcpSocket &operator << (QTcpSocket &iStream, const MovieCue &iCue);
QTcpSocket &operator >> (QTcpSocket &iStream,  MovieCue &iCue);

//===========================================================

#endif

/* notes about movie cues: 
   °  the number of attributes will increase over time, and old attributes might change. 
   Possible solutions:  the format on disk can have a start and end marker, 
   a generic field-value system can be used -- this is what I have used in the past.  It is a bit of overhead to implement, but is a good solution. 

   °  A movie cue brings us to a state of blockbuster.  Any movie cue I can think of is a series of BB events.  So a MovieCue is just a specific sort of MovieScript, which kind of suggests a parent-child relationship, but I don't see the use of the parent here.  Better to use "has-a" rather than "is-a". 

   °  It would be nice to not see what BB does as it executes.  Is there a way to suppress rendering while executing some events?  Yes, but this will have to be done carefully so you don't get stuck in a "black screen" mode if say a client gets a segfault in the middle of sending a Movie Cue, or a file is corrupt, or something. 

   °  Remember that MovieEvents already know how to write themselves to a QDataStream -- how convenient!  :-)  Thus the implementation of a MovieQueue is best done as literally a set of MovieEvents, bracketed by "MOVIE_CUE_HEADER" and "MOVIE_CUE_FOOTER" events that contain a string describing the cue.  

   °  Example movie cues: 
   launch movie X and start playing at frame 23 and stop at frame 90 
   launch movie Q and move to frame 50, but don't play
   
*/
