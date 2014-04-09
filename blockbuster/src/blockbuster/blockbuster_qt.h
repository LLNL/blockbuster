#ifndef BLOCKBUSTER_QT_H
#define BLOCKBUSTER_QT_H
#include "qmetatype.h"
#include "QWidget"
#include "ui_BlockbusterControl.h"
#include "ui_InfoWindow.h"
#include <deque>
#include "events.h"

//=================================================================
// BLOCKBUSTERINFOWINDOW class
//=================================================================
// A simple little window into the world of blockbuster statistics.  
class BlockbusterInfoWindow: public QWidget, public Ui::InfoWindow {
  Q_OBJECT
    public: 
  BlockbusterInfoWindow(QWidget *parent = NULL): QWidget(parent) {
    setupUi(this); 
  }
};

class BlockbusterInterface: public QWidget, public Ui::BlockbusterControl  {
  Q_OBJECT
    public: 
  BlockbusterInterface(QWidget *parent=NULL);
    
  virtual ~BlockbusterInterface() {}
    
  bool GetEvent (MovieEvent &event);

  void setFrameNumber(int frameNumber);   
  void setFrameRange(int32_t bottom, int32_t top);
  void setTitle(QString title); 
  void setLOD(int lod); 
  void setLODRange(int min, int max); 
  void setFrameRate(double rate); 
  void setFrameRateRange(double min, double max); 
  void setZoom(double zoom); 
  void setZoomToFit(bool ztf); 
  void setStereo(bool stereo); 
  void setRepeatBehavior (int behavior); 
  void setPingPongBehavior (int behavior); 

  void reportWindowMoved(int xpos, int ypos); 
  void reportWindowResize(int x, int y); 
  void reportMovieMoved(int xpos, int ypos); 
  void reportMovieFrameSize(int x, int y); 
  void reportMovieDisplayedSize(int x, int y); 
  void reportActualFPS(double rate); 
  void reportMovieCueStart(void); 
  void reportMovieCueComplete(void); 
  void reportStatusChanged(QString status); 
  void SetSizeToMovieCheckBox(bool check) {
    sizeToMovieCheckBox->setChecked(check); 
  }
  public slots:
  void on_quitButton_clicked();
  void on_openButton_clicked();
  void on_stereoCheckBox_stateChanged(int);  
  void on_centerPushButton_clicked();
  void on_fullSizeButton_clicked();
  void on_zoomToFitCheckBox_clicked();
  void on_fullScreenCheckBox_clicked();
  void on_sizeToMovieCheckBox_clicked();
  void on_infoButton_clicked();
  void on_startButton_clicked() ;
  void on_backStepButton_clicked();
  void on_reverseButton_clicked();
  void on_stopButton_clicked();
  void on_playButton_clicked();
  void on_stepButton_clicked();
  void on_endButton_clicked() ;
  void on_saveImageButton_clicked();

  void hideButtonClicked(); // special case, see constructor 
 
  void on_scrubCheckBox_stateChanged(int state);
  void on_repeatCheckBox_stateChanged(int state);

  void on_frameSlider_valueChanged(int);   
  void on_frameField_returnPressed();   
  void on_zoomSpinBox_valueChanged(double);   
  void on_lodSpinBox_valueChanged(int);   
  void on_fpsSpinBox_valueChanged(double);   
  void on_foreverCheckBox_stateChanged(int) ;   
  void on_pingpongCheckBox_stateChanged(int);  
 private: 
  deque<MovieEvent> mEventQueue; 
  QIntValidator mFrameValidator; 
  BlockbusterInfoWindow *mInfoWindow; 

  int mStartFrame, mEndFrame, mFrameNumber; 
  int mLOD; 
  double mFrameRate, mZoom; 
  int mPingPong, mRepeat, mZoomToFit; 
  QString mPreviousDir; 

}; 
#endif
