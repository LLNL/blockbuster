#ifndef REMOTE_CONTROL_H
#define REMOTE_CONTROL_H

#include "ui_BlockbusterControl.h"
#include "ui_InfoWindow.h"
#include "QKeyEvent"
#include "QObject"
#include "QIntValidator"
#include "QIcon"
#include "events.h"

//=============================================================
/* Allow intercepting of keypress events, to send arrow keys and space bars etc to blockbuster
 */ 

class KeyPressIntercept: public QObject {
 Q_OBJECT
   public:
  KeyPressIntercept(QObject *parent=NULL): QObject(parent) {}
 protected:
  bool eventFilter(QObject *obj, QEvent *event) {
    int eventType = event->type();
    if (eventType == QEvent::KeyPress ) {
      int keyval = static_cast<QKeyEvent *>(event)->key(), 
        modval =  static_cast<QKeyEvent *>(event)->modifiers();
      if (keyval == Qt::Key_Right   // forward by step/20/25%
          || keyval == Qt::Key_Left  // back by step/20/25% 
          || keyval == Qt::Key_Home // go to start of movie
          || (keyval == Qt::Key_Escape  && modval == Qt::ShiftModifier ) // exit
          || keyval == Qt::Key_End // end of movie
          || keyval == Qt::Key_C  // center
          || keyval == Qt::Key_F  // zoom to fit window
          || keyval == Qt::Key_H  // toggle hiding/showing the cursor
          || keyval == Qt::Key_I  // hide/show controls
          || keyval == Qt::Key_Question  // hide/show controls
          || keyval == Qt::Key_L  // l/L == decrease/increase LOD
          || keyval == Qt::Key_M  // l/L == decrease/increase LOD
          || keyval == Qt::Key_P  // reverse
          || (keyval == Qt::Key_Q  && modval == Qt::ShiftModifier ) // exit
          || keyval == Qt::Key_R  // reverse
          || keyval == Qt::Key_Z  // zoom
          || keyval == Qt::Key_Space // play/stop
          || keyval == Qt::Key_1  // zoom to 1.0
          || keyval == Qt::Key_2  // zoom to 2 or 1/2 (with shift)
          || keyval == Qt::Key_4  // zoom to 4 or 1/4 (with shift)
          )  {
      //dbprintf(" Got interesting keystroke %d", keyval); 
      emit InterestingKey(static_cast<QKeyEvent *>(event)); 
      return true;
    }
    // qDebug("Ignored key press %d", keyval);
    
    }   
    // standard event processing
    return QObject::eventFilter(obj, event);  
  }
 signals:
  void InterestingKey(QKeyEvent *event);  
}; 


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

//=================================================================
// REMOTE_CONTROL class
//=================================================================
// This class is very simple -- I leave it to the container classes to connect slots to the signals in ui_BlockbusterControl.h
class RemoteControl: public QWidget, public Ui::BlockbusterControl {
  Q_OBJECT
    public:
  RemoteControl(QWidget *parent =NULL): 
    QWidget(parent), mStartFrame(1), mEndFrame(1) , 
    mFrameValidator(1,1, parent)
    {
    setupUi(this); 
    mInfoWindow = new BlockbusterInfoWindow(NULL); 
    setFocusPolicy(Qt::StrongFocus); 
    frameField->setValidator(&mFrameValidator); 
    frameSlider->setTracking(false); 
    //startButton->setText(""); 
    //(QIcon(":/images/start.png")); 
    backStepButton->setIcon(QIcon(":/images/backStep.png")); 
    endButton->setIcon(QIcon(":/images/end.png")); 
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
    
  }
  ~RemoteControl() {}
  void SetFrameRange(int32_t bottom, int32_t top) {
    mFrameValidator.setRange(bottom, top); 
  }
  void updateFromSnapshot(MovieSnapshot &snapshot); 

  int mStartFrame, mEndFrame; 

  protected slots:
  void on_infoButton_clicked() {
    mInfoWindow->show(); 
  }
  void on_scrubCheckBox_stateChanged(int state) {
    frameSlider->setTracking(state); 
  }

  void on_repeatCheckBox_stateChanged(int state) {
    foreverCheckBox->setEnabled(state); 
  }
 protected:
    void xkeyPressEvent ( QKeyEvent * event ) {
    cerr << "keypress event" << endl;
    //if (event->key() == Qt::Key_Alt) {
      //startButton->setText("Set Start"); 
    //} else {
      QWidget::keyPressEvent(event); 
      //}
  } 
  void xkeyReleaseEvent ( QKeyEvent * event ) {
    if (event->key() == Qt::Key_Alt) {
      startButton->setText("Start"); 
    } else {
      QWidget::keyPressEvent(event); 
    }
  } 
  void xfocusOutEvent ( QFocusEvent *event  ) {
    startButton->setText("Start"); 
    QWidget::focusOutEvent(event); 
  }

  private:
  QIntValidator mFrameValidator; 
  BlockbusterInfoWindow *mInfoWindow; 
 };

#endif
