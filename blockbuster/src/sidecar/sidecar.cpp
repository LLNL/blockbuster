/*
  #
  # $RCSfile: sidecar.cpp,v $
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
  # $Id: sidecar.cpp,v 1.81 2009/05/07 21:52:44 wealthychef Exp $
  #
  #   Abstract: contains the object definitions related to the sidecar Qt GUI
  #
  #   Author: Rich Cook
  #
  # This work performed under the auspices of the U.S. Department of Energy by Lawrence Livermore National Laboratory under Contract DE-AC52-07NA27344.
  # This document was prepared as an account of work sponsored by an agency of the United States government. Neither the United States government nor Lawrence Livermore National Security, LLC, nor any of their employees makes any warranty, expressed or implied, or assumes any legal liability or responsibility for the accuracy, completeness, or usefulness of any information, apparatus, product, or process disclosed, or represents that its use would not infringe privately owned rights. Reference herein to any specific commercial product, process, or service by trade name, trademark, manufacturer, or otherwise does not necessarily constitute or imply its endorsement, recommendation, or favoring by the United States government or Lawrence Livermore National Security, LLC. The views and opinions of authors expressed herein do not necessarily state or reflect those of the United States government or Lawrence Livermore National Security, LLC, and shall not be used for advertising or product endorsement purposes.
  #
*/


/*  events.h: to pick up the #defines  for the movie events:
    This header should be placed AFTER all Qt headers, because X11 messes up 
    Qt on OS X  */
#include "events.h"
#include "MovieCues.h"
#include "RemoteControl.h"
#include "QMessageBox"
#include "QHBoxLayout"
#include "QInputDialog"
#include <QFileDialog>
#include <QApplication>
#include <algorithm>
#include "sidecar.h"
#include "common.h"
#include "events.h"
#include <iostream> 
#include "timer.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include "stringutil.h"
using namespace std; 
QString HostProfile::mUserHostProfileFile; 

bool CompareHostProfiles(const HostProfile * h1, const HostProfile * h2) {
  bool retval = (*h1 < *h2); 
  return retval; 
} 

//===============================================================
SideCar::SideCar(QApplication *app, Preferences *prefs, QWidget *parent)
  : QMainWindow(parent), mPrefs(prefs), mApp(app), mLaunchDialog(NULL), mBlockbusterProcess(NULL), mKeyPressIntercept(NULL), mDoStressTests(false), mState(BB_DISCONNECTED), 
    mBlockbusterSocket(NULL),
    mNextCommandID(1), 
    mCueManager(NULL),
    mRemoteControl(NULL), mExiting(false), mCueExecuting(false) {
  
  HostProfile::mUserHostProfileFile = QString(mPrefs->GetValue("prefsdir").c_str()) +"/hostProfiles.cnf";
  
  setupUi(this); 
  mCueManager = new MovieCueManager(NULL); 
  connect(mCueManager, SIGNAL(closed()), this, SLOT(on_showCuesButton_clicked())); 
  mCueManager->hide();

  // set up remote control interface portion of dialog
  mRemoteControl = new RemoteControl; 
  QHBoxLayout *controlLayout = new QHBoxLayout(remoteControlWidget); 
  controlLayout->setMargin(0); 
  controlLayout->addWidget(mRemoteControl); 

  // listen for mCueManager signals
  connect(mCueManager, SIGNAL(executeCue(MovieCue*)), this, SLOT(executeCue(MovieCue*))); 
  connect(mCueManager, SIGNAL(takeSnapshot()), this, SLOT(takeSnapshot())); 
  connect(mCueManager, SIGNAL(movieSnapshotEndFrame()), this, SLOT(movieSnapshotEndFrame())); 
  connect(mCueManager, SIGNAL(movieSnapshotAltEndFrame()), this, SLOT(movieSnapshotAltEndFrame())); 
  connect(mCueManager, SIGNAL(movieSnapshotStartFrame()), this, SLOT(movieSnapshotStartFrame())); 
  connect(mCueManager, SIGNAL(currentCueChanged(MovieCue*)), this, SLOT(cueSelectionChanged(MovieCue*))); 

  connect(&mBlockbusterServer, SIGNAL(newConnection()), this, SLOT(incomingBlockbusterConnection()));
 
  connect(&mKeyPressIntercept, SIGNAL(InterestingKey(QKeyEvent *)), 
          this, SLOT(InterestingKey(QKeyEvent *)));
  
  mApp->setQuitOnLastWindowClosed(false); 
  connect(mApp, SIGNAL(lastWindowClosed()), this, SLOT(aboutToQuit())); 
  connect(actionQuit, SIGNAL(triggered()), this, SLOT(aboutToQuit())); 
  PortField->setInputMask("99999"); 
  setState(BB_DISCONNECTED); 

  connect(mCueManager, SIGNAL(stopCurrentCue()), 
          this, SLOT(stopButton_clicked())); 

  //enable the remote control: 
  connect(mRemoteControl->quitButton, SIGNAL(clicked()), 
          this, SLOT(quitButton_clicked())); 
  connect(mRemoteControl->openButton, SIGNAL(clicked()), 
          this, SLOT(openButton_clicked())); 
  connect(mRemoteControl->stereoCheckBox, SIGNAL(stateChanged(int)), 
          this, SLOT(stereoCheckBox_stateChanged(int))); 
  connect(mRemoteControl->centerPushButton, SIGNAL(clicked()), 
          this, SLOT(centerPushButton_clicked())); 
  connect(mRemoteControl->fullSizeButton, SIGNAL(clicked()), 
          this, SLOT(fullSizeButton_clicked())); 
  connect(mRemoteControl->zoomToFitCheckBox, SIGNAL(clicked()), 
          this, SLOT(zoomToFitCheckBox_clicked())); 
  connect(mRemoteControl->fullScreenCheckBox, SIGNAL(clicked()), 
          this, SLOT(fullScreenCheckBox_clicked())); 
  connect(mRemoteControl->startButton, SIGNAL(clicked()), 
          this, SLOT(startButton_clicked())); 
  connect(mRemoteControl->backStepButton, SIGNAL(clicked()), 
          this, SLOT(backStepButton_clicked())); 
  connect(mRemoteControl->reverseButton, SIGNAL(clicked()), 
          this, SLOT(reverseButton_clicked())); 
  connect(mRemoteControl->stopButton, SIGNAL(clicked()), 
          this, SLOT(stopButton_clicked())); 
  connect(mRemoteControl->playButton, SIGNAL(clicked()), 
          this, SLOT(playButton_clicked())); 
  connect(mRemoteControl->stepButton, SIGNAL(clicked()), 
          this, SLOT(stepButton_clicked())); 
  connect(mRemoteControl->endButton, SIGNAL(clicked()), 
          this, SLOT(endButton_clicked())); 
  connect(mRemoteControl->frameSlider, SIGNAL(valueChanged(int)), 
          this, SLOT(frameSlider_valueChanged(int))); 
  connect(mRemoteControl->frameField, SIGNAL(returnPressed()), 
          this, SLOT(frameField_returnPressed())); 
  connect(mRemoteControl->saveImageButton, SIGNAL(clicked()), 
          this, SLOT(saveImageButton_clicked())); 
  connect(mRemoteControl->zoomSpinBox, SIGNAL(valueChanged(double)), 
          this, SLOT(zoomSpinBox_valueChanged(double))); 
  connect(mRemoteControl->lodSpinBox, SIGNAL(valueChanged(int)), 
          this, SLOT(lodSpinBox_valueChanged(int))); 
  connect(mRemoteControl->fpsSpinBox, SIGNAL(valueChanged(double)), 
          this, SLOT(fpsSpinBox_valueChanged(double))); 
  connect(mRemoteControl->foreverCheckBox, SIGNAL(stateChanged(int)), 
          this, SLOT(foreverCheckBox_stateChanged(int))); 
  connect(mRemoteControl->noScreensaverCheckBox, SIGNAL(stateChanged(int)), 
          this, SLOT(noScreensaverCheckBox_stateChanged(int))); 
  connect(mRemoteControl->repeatCheckBox, SIGNAL(stateChanged(int)), 
          this, SLOT(repeatCheckBox_stateChanged(int))); 
  connect(mRemoteControl->pingpongCheckBox, SIGNAL(stateChanged(int)), 
          this, SLOT(pingpongCheckBox_stateChanged(int))); 

  return;
}

//===============================================================
void SideCar::ReadCueFile(std::string filename) {
  mCueManager->ReadCueFile(filename); 
  if (mCueManager->numCues()) {
    on_showCuesButton_clicked(); // this will show the cues and change the button text
  }
}

//===============================================================
void SideCar::cueSelectionChanged(MovieCue *iCue){
  mCueManager->enableSnapshotButton(iCue && mState == BB_CONNECTED); 
  return; 
}

//===============================================================
void SideCar::enableMovieActions(bool enabled) {
  actionToggle_Controls->setEnabled(enabled); 
  actionPlay->setEnabled(enabled); 
  actionStop->setEnabled(enabled); 
  actionStepAhead->setEnabled(enabled); 
  actionStepBackward->setEnabled(enabled); 
  actionGo_To_Frame->setEnabled(enabled); 
  actionGo_To_Beginning->setEnabled(enabled); 
  actionGo_To_End->setEnabled(enabled); 
  mCueManager->enableExecuteButton(mCueManager->getCurrentCue()); 

  enableRemoteControl(enabled); 
  return; 
}

//===============================================================
void SideCar::enableRemoteControl(bool enable) {
  mRemoteControl->setEnabled(enable); 
  return;
}

//===============================================================
void SideCar::checkCaptureKeystrokes(void){ 
  // turn on remote control mode if we are connected and the checkbox is clicked or connected and the cues are invisible
  captureKeystrokesCheckBox->setEnabled(mState == BB_CONNECTED); 
  if(mState == BB_CONNECTED && captureKeystrokesCheckBox->isChecked()) {
    dbprintf(5, "start listening for blockbuster keystrokes\n"); 
    mApp->installEventFilter(&mKeyPressIntercept);
  } else { 
    dbprintf(5, "stop listening for blockbuster keystrokes\n"); 
    mApp->removeEventFilter(&mKeyPressIntercept); 
  }
  mCueManager->setEnabled(mState != BB_CONNECTED || mCueManager->isHidden() || !captureKeystrokesCheckBox->isChecked()); 
  
  return; 
}
    

//===============================================================
void SideCar::setState(connectionState state){  
  //dbprintf(5, "set state %d\n", state); 
  if (mExiting) return; 
  switch (state) {
  case BB_DISCONNECTED:
  case BB_FINISHED:
  case BB_ERROR:
    StatusLabel->setText("Disconnected"); 
    connectButton->setText("Connect"); 
    connectButton->setEnabled(true); 
    launchBlockbusterButton->setText("Launch Blockbuster"); 
    launchBlockbusterButton->setEnabled(true); 
    enableMovieActions(false); 
    if (mBlockbusterSocket) {
      mBlockbusterSocket->disconnect(this); 
      if (mBlockbusterSocket->state() == QAbstractSocket::ConnectedState) { 
        mBlockbusterSocket->disconnectFromHost(); 
      }
      mBlockbusterSocket->abort(); 
      mBlockbusterSocket->deleteLater(); 
      mBlockbusterSocket = NULL; 
    }
    endBlockbusterProcess();
    if (state == BB_ERROR) {
      state = BB_DISCONNECTED; 
    }
    if (mLaunchDialog) mLaunchDialog->hide(); 
    break;
  case BB_STARTING:
  case BB_WAIT_CONNECTION:
    //  case BB_WAIT_BACKCHANNEL:
    StatusLabel->setText(QString("Connecting to host ")); 
    connectButton->setText("Disconnect"); 
    connectButton->setEnabled(true); 
    launchBlockbusterButton->setText("Launch Blockbuster"); 
    launchBlockbusterButton->setEnabled(false); 
    enableMovieActions(false); 
    break;
  case BB_CONNECTED:
    StatusLabel->setText(QString("Connected")); 
    connectButton->setText("Disconnect"); 
    connectButton->setEnabled(true); 
    launchBlockbusterButton->setText("Terminate Blockbuster"); 
    launchBlockbusterButton->setEnabled(true); 
    enableMovieActions(true); 
    break;
  case BB_DISCONNECTING:
    StatusLabel->setText(QString("Disconnecting from host ")+mHost); 
    connectButton->setText("Disconnecting..."); 
    connectButton->setEnabled(false); 
    launchBlockbusterButton->setText("Terminate Blockbuster"); 
    launchBlockbusterButton->setEnabled(false); 
    enableMovieActions(false); 
    break;
  default:
    cerr << "Error: bad state in setState: " << state << endl; 
  }
  mState = state; 
  mCueManager->enableSnapshotButton
    (mCueManager->getCurrentCue() && mState == BB_CONNECTED); 
  checkCaptureKeystrokes(); 
  return; 
}

//===============================================================
void SideCar::setBlockbusterSocket(QTcpSocket *newSocket) {
  mBlockbusterSocket = newSocket; 

  connect(mBlockbusterSocket, SIGNAL(error(QAbstractSocket::SocketError)), 
          this, SLOT(socketError(QAbstractSocket::SocketError)));
  connect(mBlockbusterSocket, SIGNAL(disconnected()), 
          this, SLOT(disconnected()));
  connect(mBlockbusterSocket, SIGNAL(readyRead()), 
          this, SLOT(checkBlockbusterData()));
  if (mBlockbusterSocket->state() == QAbstractSocket::ConnectedState) {
    dbprintf(5, "new socket is connected\n"); 
    setState(BB_CONNECTED); 
  } else {
    connect(mBlockbusterSocket, SIGNAL(connected()),  
            this, SLOT(connectedToBlockbuster()));
  }
  /*
    We require lower latency on every packet sent so enable TCP_NODELAY.
  */ 
  int fd = mBlockbusterSocket->socketDescriptor(); 
  int option = 1; 
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                 &option, sizeof(option)) < 0) {
    DEBUGMSG("TCP_NODELAY setsockopt error");
  }
  return; 
}

//===============================================================
void SideCar::setBlockbusterPort(QString port) {
  PortField->setText(port); 
  mPortString = port; 
  return; 
}

//===============================================================
void SideCar::connectToBlockbuster(){
  dbprintf(5, "connectToBlockbuster\n"); 
  if (mBlockbusterSocket) {
    cerr << "Logic error:  connectToBlockbuster called with already active socket " << endl; 
    exit(1); 
  }

  setBlockbusterSocket(new QTcpSocket); 
  setBlockbusterPort(PortField->text()); // not as silly as it looks

  mHost = HostField->text();
  if (mPortString.toInt() == 00) {
    QMessageBox::warning(this,  "Error",
                         "Port number must be a positive integer");
    return; 
  }
  setState(BB_WAIT_CONNECTION);     
  dbprintf(5, QString("connectToBlockbuster, host %1, port %2\n").arg(mHost).arg(mPortString)); 
  mBlockbusterSocket->connectToHost(mHost, mPortString.toInt()); 
  dbprintf(5, "end connectToBlockbuster\n"); 
  return; 
}

//===============================================================
void SideCar::connectedToBlockbuster() {
  dbprintf(5, "blockbuster accepted connection\n"); 
  if (mBlockbusterSocket->state() != QAbstractSocket::ConnectedState) {
    cerr << "ERROR!  connectedToBlockbuster called without valid connection" << endl; 
    return; 
  }
  setState(BB_CONNECTED); 
  SendEvent(MovieEvent("MOVIE_SIDECAR_STATUS"));
  raise(); // not an exception --  This is to raise the window to the front
  return; 
}

//===============================================================
void SideCar::incomingBlockbusterConnection() {
  dbprintf(5, "incomingBlockbusterConnection\n"); 
  setBlockbusterSocket(mBlockbusterServer.nextPendingConnection());
  setState(BB_CONNECTED); 

  return; 
}

//===============================================================
void SideCar::checkBlockbusterData() {
  MovieEvent event; 
  while (mBlockbusterSocket && mBlockbusterSocket->canReadLine()) {
    //dbprintf(5, "checkBlockbusterData got event"); 
    try {
      *mBlockbusterSocket >> event; 
    } catch (QString errmsg) {
      cerr << "Exception thrown in checkBlockbusterData from operator >>: \"" << errmsg.toStdString() << "\"" << endl;
    }
    dbprintf(5, "checkBlockbusterData: Got event %s\n", event.mEventType.c_str()); 
    if (event.mEventType == "MOVIE_SIDECAR_MESSAGE") {
      dbprintf(5, QString("Message received from blockbuster:  \"%1\"").arg(event.mString.c_str())); 
    } 
    else if (event.mEventType == "MOVIE_PWD") {
      dbprintf(5, QString("Blockbuster changed directories to %1\n").arg(event.mString.c_str())); 
      mBlockbusterCWD = event.mString.c_str(); 
    } 
    else if (event.mEventType == "MOVIE_SIDECAR_STATUS") {
      MovieSnapshot snapshot(event.mString); 
      if (snapshot.mSnapshotType == "MOVIE_SNAPSHOT" || 
          snapshot.mSnapshotType == "MOVIE_SNAPSHOT_ENDFRAME" || 
          snapshot.mSnapshotType == "MOVIE_SNAPSHOT_STARTFRAME" || 
          snapshot.mSnapshotType == "MOVIE_SNAPSHOT_ALT_ENDFRAME") {
        if (!mCueManager->getCurrentCue()) {
          cerr<<"Error in snapshot request: no current cue selected"<<endl; 
        } else {
          mCueManager->SetCurrentCue(snapshot);    
        }
      } 
      else { 
        mRemoteControl->updateFromSnapshot(snapshot); 
      }
    }
    else if (event.mEventType == "MOVIE_CUE_COMPLETE") {
      mCueExecuting = false;
    } 
    else if (event.mEventType == "MOVIE_STOP_ERROR") {
      QMessageBox::critical(this, "Blockbuster error", 
                            QString("Message from blockbuster:  %1.")
                            .arg(event.mString.c_str())); 
    }
    else {
      cerr << "Bad event: " << string(event) << " in checkBlockbusterData()\n"<< endl; 
    }
    
  }
  return; 
}

//===============================================================
void SideCar::socketError(QAbstractSocket::SocketError socketError){
  dbprintf(5, "socketError from blockbuster communication socket\n"); 
  if (mState == BB_ERROR || mState == BB_DISCONNECTED 
      || mState == BB_DISCONNECTING || mState == BB_FINISHED) {
    dbprintf(5, "Ignoring socket error in state %d\n", mState); 
    return; 
  }
  setState(BB_ERROR); 

  switch (socketError) {
  case QAbstractSocket::RemoteHostClosedError:
    QMessageBox::warning(this,  "Error",
                         "The remote host closed the connection");
    break;
  case QAbstractSocket::HostNotFoundError:
    QMessageBox::warning(this, "Error", 
                         "The host was not found. Please check the "
                         "host name and port settings.");
    break;
  case QAbstractSocket::ConnectionRefusedError:
    QMessageBox::warning(this, "Error", 
                         "The connection was refused by the server. ");
    break;
  default:
    QMessageBox::critical
      (this, "Error", 
       QString("An unknown socket error occurred: %1\n"
               "If this continues, please contact Rich Cook at 3-9605")
       .arg(mBlockbusterSocket->errorString())); 
  }
  return; 
}

//===============================================================
void SideCar::disconnectFromBlockbuster() {
  dbprintf(5, "disconnecting blockbuster socket\n"); 
  setState(BB_DISCONNECTED); 

  /*  if (mBlockbusterSocket && 
      mBlockbusterSocket->state() != QAbstractSocket::UnconnectedState) {
      mBlockbusterSocket->disconnectFromHost(); 
      } else {
      dbprintf(5, "...No socket to disconnect.\n"); 
      }
  */ 
  
  return; 
}

//===============================================================
void SideCar::disconnected() {
  dbprintf(5, "sidecar disconnected\n"); 
  if (mBlockbusterSocket) {
    mBlockbusterSocket->deleteLater(); 
    mBlockbusterSocket = NULL; 
  }
  setState(BB_DISCONNECTED);   
 
}


//=======================================================================
void SideCar::blockbusterProcessError(QProcess::ProcessError ) {
  blockbusterProcessExit(); 
  return; 
}

//=======================================================================
void SideCar::blockbusterProcessExit(int, QProcess::ExitStatus ) {
  if (mState == BB_WAIT_CONNECTION || mState == BB_STARTING) {
    QMessageBox::warning(this,  "Blockbuster exit",
                         "Blockbuster process exited.");
  }
  dbprintf(5, "Process exited."); 
  blockbusterReadStdErr(); 
  blockbusterReadStdOut(); 
  endBlockbusterProcess(); 
  //disconnectFromBlockbuster(); 
  setState(BB_DISCONNECTED); 
}

//=======================================================================
void SideCar::blockbusterReadStdErr() {
  //dbprintf(5, "Sidecar: ReadStdErr()... \n"); 
  if (!mBlockbusterProcess) return; 
  mBlockbusterProcess->setReadChannel(QProcess::StandardError); 
  while (mBlockbusterProcess && mBlockbusterProcess->canReadLine()) {
    QString line = mBlockbusterProcess->readLine(); 
    if (line == "") {
      dbprintf(3,"got null line from stderr\n"); 
      return; 
    }  
    dbprintf(1, "Got blockbuster stderr: %s\n", line.toStdString().c_str()); 
    if (line.contains("ERROR")  ||  line.contains("Could not") || 
        line.contains("not found") || 
        (line.contains("Unknown host") && !line.contains("squeue"))) {
      QMessageBox::warning(this, "Blockbuster Error:", line); 
      endBlockbusterProcess(); 
      setState(BB_ERROR); 
    } else if (line.contains("sidecar port:")) {
      QStringList tokens = line.split(" ",QString::SkipEmptyParts); 
      dbprintf(5, QString("blockbuster sidecar port detected: \"%1\", tokens = %2\n").arg(line).arg(tokens.size())); 
      setBlockbusterPort(tokens[3]); 
    } else {                             
      if ( line.contains(QRegExp("[Nn]o such file"))
           &&  !line.contains("scanning")) {
        QMessageBox::warning(this, "Error", 
                             "Path to blockbuster is incorrect"); 
        setState(BB_ERROR);
      }
      
    }
  }
  return; 
}

//=======================================================================
void SideCar::blockbusterReadStdOut(){ 
  //dbprintf(5, "Sidecar: ReadStdOut()\n"); 
  if (!mBlockbusterProcess) return; 
  mBlockbusterProcess->setReadChannel(QProcess::StandardOutput); 
  while (mBlockbusterProcess && mBlockbusterProcess->canReadLine()) {
    QString line = mBlockbusterProcess->readLine(); 
    if (line == "") {
      dbprintf(3, "got null line from stdout\n"); 
      return; 
    }
    dbprintf(1, "Got blockbuster stdout: %s\n", line.toStdString().c_str()); 
  }
  return; 
} 
  
//===============================================================
void SideCar::on_captureKeystrokesCheckBox_clicked() {
  checkCaptureKeystrokes(); 
  return;
}

//===============================================================
void SideCar::on_launchBlockbusterButton_clicked() {
  if (launchBlockbusterButton->text() == "Launch Blockbuster") {
    const MovieCue *theCue =  mCueManager->getCurrentCue();    
    askLaunchBlockbuster(theCue, "GUILAUNCH"); 
  } else {
    dbprintf(5, "Sending QUIT signal to blockbuster\n"); 
    SendEvent(MovieEvent ("MOVIE_QUIT")); 
    setState(BB_DISCONNECTING); 
    gCoreApp->processEvents(); 
    disconnectFromBlockbuster(); 
  }
  return; 
}

//===============================================================
void SideCar::on_connectButton_clicked() {
  if (mState == BB_DISCONNECTED) {
    connectToBlockbuster(); 
  } else if (mState == BB_CONNECTED) { 
    disconnectFromBlockbuster(); 
  }  else {
    cerr << "Ignoring connect button in state " << mState << endl; 
  }
  
  return; 
}

//===============================================================
void SideCar::on_PortField_returnPressed() {
  on_HostField_returnPressed(); 
  return; 
}

//===============================================================
void SideCar::on_HostField_returnPressed() {
  dbprintf(5, "on_HostField_returnPressed\n"); 
  if (HostField->text() == "" || PortField->text() == "") return; 
  if (mState == BB_CONNECTED) {
    QMessageBox::warning(this, "Error", 
                         "Already connected to host.");
    return ;
  }
  on_connectButton_clicked(); 
  return; 
}

//===============================================================
QProcess *SideCar::createNewBlockbusterProcess(void) {
  endBlockbusterProcess(); 

  mBlockbusterProcess = new QProcess;
  //qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
  //connect(mProcess, SIGNAL(started()), this, SLOT(ProcessStarted())); 
  connect(mBlockbusterProcess, SIGNAL(error(QProcess::ProcessError)), 
          this, SLOT(blockbusterProcessError(QProcess::ProcessError))); 
  connect(mBlockbusterProcess, SIGNAL(finished(int, QProcess::ExitStatus)), 
          this, SLOT(blockbusterProcessExit(int, QProcess::ExitStatus)));
  connect(mBlockbusterProcess, SIGNAL(readyReadStandardError()), 
          this, SLOT(blockbusterReadStdErr())); 
  connect(mBlockbusterProcess, SIGNAL(readyReadStandardOutput()), 
          this, SLOT(blockbusterReadStdOut())); 
  return mBlockbusterProcess; 
}

//===============================================================
/* Give user opportunity to launch blockbuster, or actually launch blockbuster. 
   If called from on_launchBlockbusterButton_clicked(), moviename = "GUILAUNCH".
   If called from executeCue(), moviename = "CUELAUNCH".
   If called from main(),  moviename is set to the name of a real movie with an @ sign for hostname.  
*/
void SideCar::askLaunchBlockbuster(const MovieCue* iCue, QString moviename, bool executeWithoutAsking) {

  mBlockbusterServer.close(); // in case it was listening, don't any more. 
  if (!iCue && moviename.contains("@")) {
    dbprintf(5, "found @ symbol\n"); 
    QStringList tokens = moviename.split("@"); 
    if (tokens.size() != 2) {
      QMessageBox::warning(this,  "Error",
                           QString("Could not parse movie name %1").arg(moviename));
      return; 
    }  
    moviename = tokens[0];
    HostField->setText(tokens[1]); 
  } 
    
  if (moviename == "GUILAUNCH") {
    moviename = ""; 
  }

  BlockbusterLaunchDialog dialog(this, HostField->text(), PortField->text(), moviename, mState, mPrefs->GetValue("rsh").c_str(), mPrefs->GetLongValue("verbose"));
  // restore the last used profile or the default if first launch
  dialog.trySetProfile(mPrefs->GetValue("SIDECAR_DEFAULT_PROFILE").c_str()); 

  setBlockbusterPort(PortField->text()); 
  connect(&mBlockbusterServer, SIGNAL(newConnection()), &dialog, SLOT(blockbusterConnected())); 
  connect (&dialog, SIGNAL(stateChanged(connectionState)), this, SLOT(setState(connectionState))); 
  if (mBlockbusterServer.isListening()) {
    mBlockbusterServer.close();  // in case it's listening on the wrong port
  }
  
  if (mPrefs->GetBoolValue("dmx")) {
    dialog.useDMXCheckBox->setChecked(true); 
  }


  string junk; 
  if (mPrefs->TryGetValue("play", junk)) {
    dialog.playCheckBox->setChecked(true); 
  }

  if (iCue) {
    dialog.fullScreenCheckBox->setChecked(iCue->mFullScreen); 
  }
  if (!executeWithoutAsking) {
    dialog.exec(); 
    if (mState == BB_WAIT_CONNECTION) {
      PortField->setText(dialog.getPort()); 
      HostField->setText(dialog.getHost()); 
      connectToBlockbuster(); 
    }  
    if (dialog.mCurrentProfile) {
      // save the user's preference as default for next launch.  
      mPrefs->SetValue("SIDECAR_DEFAULT_PROFILE", dialog.mCurrentProfile->mName.toStdString()); 
    }
  } else {    
    dialog.on_launchButton_clicked(); 
  }
  mLaunchDialog = &dialog; 
  while (mState == BB_WAIT_CONNECTION || mState == BB_STARTING) {
    gCoreApp->processEvents(); 
    usleep(10*1000); //10 ms
  }
       
  if (mState != BB_CONNECTED ) {
    QMessageBox::warning(this,  "Not connected",
                         QString("Blockbuster was not launched"));       
    endBlockbusterProcess(); 
    mBlockbusterServer.close(); 
  }  
  mLaunchDialog = NULL; 
    
  return; 
}

//===============================================================
/* this is a callback, when mCueManager's execute button is clicked */
void SideCar::executeCue(MovieCue* iCue) {
  if (!mBlockbusterSocket ||
      mBlockbusterSocket->state() != QAbstractSocket::ConnectedState) {
    askLaunchBlockbuster(iCue, "CUELAUNCH");
    if (mState != BB_CONNECTED) {
      mCueManager->setCueRunning(false); 
      return; // user canceled or connect failed
    }
  }
  
  int iter=1; 
  if (this->mDoStressTests) {
    dbprintf(5, "Executing 20 iterations of cue\n"); 
    iter=20; 
  } else {
    dbprintf(5, "Executing cue\n"); 
  }

  mCueExecuting = true; 

  try {
    MovieScript script; 
    iCue->GenerateScript(script); 
    // send the cue at least once. 
    while (iter--) {
      std::vector<MovieEvent>::iterator current = script.begin(), end = script.end(); 
      int num = 0; 
      try {
        while (current != end) {
          SendEvent(*current); 
          ++ current; 
          ++num; 
        }
      } catch (QString errmsg) {
        errmsg = QString("Error sending event #")+QString::number(num)+": "+errmsg;
        throw(errmsg);
      }
    }    
  } catch (QString qerr) {
	cerr << "executeCue error sending cue: "  << qerr.toStdString() << endl; 
  }
  // wait for cue to end: 
  while (mBlockbusterSocket &&
         mBlockbusterSocket->state() == QAbstractSocket::ConnectedState &&
         mCueExecuting) {
    gCoreApp->processEvents(); 
    usleep(100*1000); // 1/10 sec to be good cpu citizen
  }
  if (mState != BB_CONNECTED) {
    mCueManager->setCueRunning(false); 
  }

  mCueExecuting = false; 
  return;
}

//===============================================================
void SideCar::on_actionOpen_Cue_File_triggered() {
  mCueManager->on_readCueFileButton_clicked(); 
  return; 
}

//===============================================================
void SideCar::on_actionSave_Cue_File_triggered() {
  mCueManager->on_saveCuesButton_clicked(); 
  return; 
}

//===============================================================
void SideCar::on_actionSave_Cue_File_As_triggered() {
  mCueManager->on_saveCuesAsButton_clicked(); 
  return; 
}

//===============================================================
void SideCar::takeSnapshot(){ 
  SendEvent(MovieEvent ("MOVIE_SNAPSHOT")); 
  return; 
}
//===============================================================
void SideCar::movieSnapshotStartFrame(){ 
  SendEvent(MovieEvent ("MOVIE_SNAPSHOT_STARTFRAME")); 
  return; 
}

//===============================================================
void SideCar::movieSnapshotEndFrame(){ 
  SendEvent(MovieEvent ("MOVIE_SNAPSHOT_ENDFRAME")); 
  return; 
}

//===============================================================
void SideCar::movieSnapshotAltEndFrame(){
  SendEvent(MovieEvent ("MOVIE_SNAPSHOT_ALT_ENDFRAME")); 
  return; 
}

//===============================================================
void SideCar::on_showCuesButton_clicked(){ 
  if (mCueManager->isHidden()) {    
    mCueManager->show(); 
    showCuesButton->setText("Hide Movie Cues"); 
  } else {
    mCueManager->hide(); 
    showCuesButton->setText("Show Movie Cues...");
  }
  checkCaptureKeystrokes(); 
  return; 
}

//===============================================================
void SideCar::on_actionToggle_Controls_triggered(){ 
  SendEvent(MovieEvent ("MOVIE_TOGGLE_INTERFACE")); 
  return; 
}

//===============================================================
void SideCar::on_actionPlay_triggered(){ 
  SendEvent(MovieEvent ("MOVIE_PLAY_FORWARD")); 
  return; 
}

//===============================================================
void SideCar::on_actionStepAhead_triggered(){ 
  SendEvent(MovieEvent ("MOVIE_STEP_FORWARD")); 
  return; 
}

//===============================================================
void SideCar::on_actionStepBackward_triggered(){ 
  SendEvent(MovieEvent ("MOVIE_STEP_BACKWARD")); 
  return; 
}


//===============================================================
void SideCar::on_actionStop_triggered(){  
  SendEvent(MovieEvent ("MOVIE_STOP"));
  return; 
}

//===============================================================
void SideCar::on_actionGo_To_Beginning_triggered(){  
  SendEvent(MovieEvent ("MOVIE_GOTO_START"));
  return; 
}

//===============================================================
void SideCar::on_actionGo_To_End_triggered(){  
  SendEvent(MovieEvent ("MOVIE_GOTO_END")); 
  return; 
}

//================================================================
void SideCar::on_actionGo_To_Frame_triggered() {
  bool ok; 
  uint32_t frameNum =  QInputDialog::
    getInt(this, tr("Go To Frame"),
           tr("Frame Number:"), 0, 0, 2147483647, 1, &ok);
  if (ok) {
    MovieEvent event("MOVIE_GOTO_FRAME"); 
    event.mNumber = frameNum-1;
    SendEvent(event); 
  }
  return ;
};
 

//================================================================
/* This function will open a dialog and then send the result to blockbuster */
void SideCar::openButton_clicked(){   
  QString filename =
    QFileDialog::  getOpenFileName(this, "Choose a movie file",
                                   "",
                                   "Movie Files (*.sm)");
  if (filename != "") {
    SendEvent(MovieEvent ("MOVIE_OPEN_FILE", filename)); 
  }
   
  return;
}

//================================================================
void SideCar::stereoCheckBox_stateChanged(int state){
  SendEvent(MovieEvent("MOVIE_SET_STEREO", state)); 
  return; 
}

//================================================================
void SideCar::quitButton_clicked(){   
  SendEvent(MovieEvent ("MOVIE_QUIT")); 
  return; 
}


//================================================================
void SideCar::centerPushButton_clicked(){   
  SendEvent(MovieEvent ("MOVIE_CENTER")); 
}

//================================================================
void SideCar::fullSizeButton_clicked(){   
  SendEvent(MovieEvent ("MOVIE_ZOOM_ONE")); 
}

//================================================================
void SideCar::zoomToFitCheckBox_clicked(){   
  SendEvent(MovieEvent ("MOVIE_ZOOM_TO_FIT", mRemoteControl->zoomToFitCheckBox->isChecked())); 
}

//================================================================
void SideCar::fullScreenCheckBox_clicked(){   
  SendEvent(MovieEvent ("MOVIE_FULLSCREEN", mRemoteControl->fullScreenCheckBox->isChecked())); 
}

//================================================================
void SideCar::startButton_clicked(){   
  SendEvent(MovieEvent ("MOVIE_GOTO_START")); 
}

//================================================================
void SideCar::backStepButton_clicked(){   
  SendEvent(MovieEvent ("MOVIE_STEP_BACKWARD"));
}

//================================================================
void SideCar::reverseButton_clicked(){   
  SendEvent(MovieEvent ("MOVIE_PLAY_BACKWARD")); 
}

//================================================================
void SideCar::stopButton_clicked(){   
  SendEvent(MovieEvent ("MOVIE_STOP")); 
  mCueManager->stopLooping();  
}

//================================================================
void SideCar::playButton_clicked(){   
  SendEvent(MovieEvent ("MOVIE_PLAY_FORWARD")); 
}

//================================================================
void SideCar::stepButton_clicked(){   
  SendEvent(MovieEvent ("MOVIE_STEP_FORWARD")); 
}

//================================================================
void SideCar::endButton_clicked(){   
  SendEvent(MovieEvent ("MOVIE_GOTO_END")); 
}


//================================================================
void SideCar::frameSlider_valueChanged(int value){   
  //DEBUGMSG("frameSlider_valueChanged %d", value); 
  if (value < mRemoteControl->mStartFrame+1) {
    mRemoteControl->frameSlider->setValue(mRemoteControl->mStartFrame+1); 
    return;
  }
  if (value > mRemoteControl->mEndFrame+1) {
    mRemoteControl->frameSlider->setValue(mRemoteControl->mEndFrame+1); 
    return;
  }
  SendEvent(MovieEvent ("MOVIE_GOTO_FRAME", value-1)); 
}

//================================================================
void SideCar::frameField_returnPressed() {    
  SendEvent(MovieEvent ("MOVIE_GOTO_FRAME",
                        mRemoteControl->frameField->text().toInt())); 
}

//================================================================
void SideCar::saveImageButton_clicked() {   
  mBlockbusterCWD = ""; 
  SendEvent(MovieEvent("MOVIE_PWD"));
  timer theTimer; 
  theTimer.start(); 
  while (mBlockbusterCWD == "") {
    if (theTimer.elapsed_time() > 5.0) {
      QMessageBox::critical(this, "Sidecar Error:", "Blockbuster did not respond to the request for a working directory");
      return; 
    }
    gCoreApp->processEvents(); 
  }
  bool ok;
  QString text = QInputDialog::getText(this, tr("Filename"),
                                       tr("Please give the image a name.  Location will be relative to blockbuster's working directory."), 
                                       QLineEdit::Normal,
                                       mBlockbusterCWD, &ok);
  if (ok && !text.isEmpty()) {
    SendEvent(MovieEvent ("MOVIE_SAVE_FRAME", text)); 
  }
}

//================================================================
void SideCar::zoomSpinBox_valueChanged(double value){  
  mRemoteControl->zoomSpinBox->Lock(value); 
  SendEvent(MovieEvent ("MOVIE_ZOOM_SET", (float)value)); 
}

//================================================================
void SideCar::lodSpinBox_valueChanged(int value){   
  mRemoteControl->lodSpinBox->Lock(value); 
  SendEvent(MovieEvent ("MOVIE_SET_LOD", value)); 
}

//================================================================
void SideCar::fpsSpinBox_valueChanged(double value){   
  mRemoteControl->fpsSpinBox->Lock(value); 
  SendEvent(MovieEvent ("MOVIE_SET_RATE", (float)value)); 
}

//================================================================
void SideCar::foreverCheckBox_stateChanged(int state) {
  MovieEvent event("MOVIE_SET_REPEAT", -1); 
  if (!state) {
    event.mNumber = mRemoteControl->repeatCheckBox->isChecked(); 
  }
  SendEvent(event); 
}

//================================================================
void SideCar::noScreensaverCheckBox_stateChanged(int state) {
  SendEvent(MovieEvent("MOVIE_NOSCREENSAVER", state)); 
}

//================================================================
void SideCar::repeatCheckBox_stateChanged(int state) {   
  if (state) mRemoteControl->pingpongCheckBox->setChecked(false); 
  if (state > 1) state = 1; 
  SendEvent(MovieEvent("MOVIE_SET_REPEAT", state)); 
  return; 
}

//================================================================
void SideCar::pingpongCheckBox_stateChanged(int state) {   
  if (state)  mRemoteControl->repeatCheckBox->setChecked(false); 
  SendEvent(MovieEvent ("MOVIE_SET_PINGPONG", state)); 
}

//================================================================
void SideCar::sendString() {  
  SendEvent(MovieEvent("MOVIE_MESSAGE")); 
  return; 
};

//================================================================
void SideCar::SendEvent(MovieEvent event) {
  try {
    event.mID = mNextCommandID++; 
    if (mBlockbusterSocket && mBlockbusterSocket->state() == QAbstractSocket::ConnectedState) {
      *mBlockbusterSocket << event; 
      mBlockbusterSocket->flush(); 
      mBlockbusterSocket->waitForBytesWritten(); 
    }
  } catch (QString qerr) {
	cerr << "SendEvent error sending event: " << event.mString << ": " << qerr.toStdString() << endl; 
  }
  return ;
}

//================================================================
/* keystrokes that blockbuster is interested in go here: */
void SideCar::InterestingKey(QKeyEvent *event){
  int modkey = event->modifiers(); 
  switch(event->key()) {
  case Qt::Key_Left : // back by step/20/25% 
    if (modkey == Qt::SHIFT) {
      SendEvent(MovieEvent("MOVIE_SKIP_BACKWARD")); 
    } else if (modkey == Qt::CTRL) {
      SendEvent(MovieEvent("MOVIE_SECTION_BACKWARD")); 
    } else {
      SendEvent(MovieEvent("MOVIE_STEP_BACKWARD")); 
    } 
    break; 
  case Qt::Key_Right:  // forward by step/20/25%
    if (modkey == Qt::SHIFT) {
      SendEvent(MovieEvent("MOVIE_SKIP_FORWARD")); 
    } else if (modkey == Qt::CTRL) {
      SendEvent(MovieEvent("MOVIE_SECTION_FORWARD")); 
    } else {
      SendEvent(MovieEvent("MOVIE_STEP_FORWARD")); 
    }
    break; 
  case Qt::Key_Home: // go to start of movie
    SendEvent(MovieEvent("MOVIE_GOTO_START"));
    break;

  case Qt::Key_End: // end of movie
    SendEvent(MovieEvent("MOVIE_GOTO_END"));
    break;

  case Qt::Key_C : // center
    SendEvent(MovieEvent("MOVIE_CENTER"));
    break;

  case Qt::Key_F : // zoom to fit window
    SendEvent(MovieEvent("MOVIE_ZOOM_TO_FIT"));
    break;

  case Qt::Key_H : //show keyboard help
  case Qt::Key_Question : 
    SendEvent(MovieEvent("MOVIE_KEYBOARD_HELP"));
    break;

  case Qt::Key_I : // hide/show controls
    SendEvent(MovieEvent("MOVIE_TOGGLE_INTERFACE"));
    break;

  case Qt::Key_L : // l/L == decrease/increase LOD
    if (modkey == Qt::SHIFT) {
      SendEvent(MovieEvent("MOVIE_INCREASE_LOD"));
    } else {
      SendEvent(MovieEvent("MOVIE_DECREASE_LOD"));
    }
    break;

  case Qt::Key_M : // toggle hiding/showing the cursor
    SendEvent(MovieEvent("MOVIE_TOGGLE_CURSOR"));
    break;

  case Qt::Key_P : // reverse
    SendEvent(MovieEvent("MOVIE_PLAY_FORWARD"));
    break;

  case Qt::Key_Escape: // exit
  case Qt::Key_Q: // exit 
    SendEvent(MovieEvent("MOVIE_QUIT"));
    break;

  case Qt::Key_R : // reverse
    SendEvent(MovieEvent("MOVIE_PLAY_BACKWARD"));
    break;

  case Qt::Key_Z : // zoom to fit window
    if (modkey == Qt::SHIFT) {
      SendEvent(MovieEvent("MOVIE_ZOOM_DOWN"));
    } else {
      SendEvent(MovieEvent("MOVIE_ZOOM_UP"));
    } 
    break;

  case Qt::Key_Space: // play/stop
    SendEvent(MovieEvent("MOVIE_PAUSE"));
    break;

  case Qt::Key_Plus: // play/stop
    SendEvent(MovieEvent("MOVIE_INCREASE_RATE"));
    break;

  case Qt::Key_Minus: // play/stop
    SendEvent(MovieEvent("MOVIE_DECREASE_RATE"));
    break;

  case Qt::Key_1 : // zoom to 1.0
    SendEvent(MovieEvent("MOVIE_ZOOM_ONE"));
    break;

  case Qt::Key_2 : // zoom to 2 or 1/2 (with shift)
    if (modkey == Qt::SHIFT) {
      SendEvent(MovieEvent("MOVIE_ZOOM_SET", 0.5f));
    } else {
      SendEvent(MovieEvent("MOVIE_ZOOM_SET", 2.0f));
    }
    break;

  case Qt::Key_4 : // zoom to 4 or 1/4 (with shift)
    if (modkey == Qt::SHIFT) {
      SendEvent(MovieEvent("MOVIE_ZOOM_SET", 0.25f));
    } else {
      SendEvent(MovieEvent("MOVIE_ZOOM_SET", 4.0f));
    }
    break;

  default:
    throw QString("Intercepted Key called IMPROPERLY with value = %1 and mod = %2").arg((int)event->key()).arg((int)event->modifiers()); 
    return; 
  }
  // qDebug("Intercepted Key called properly"); 
  return; 
  
}
// =============================================================
HostProfile& HostProfile::operator =(const HostProfile& other){
  *this << string(other); 
  return *this; 
}

// =============================================================
bool HostProfile::operator <(const HostProfile& other) const {
  bool retval = false; 
  QString compareResult = ">="; 
  QString key1 = this->mName, key2 = other.mName; 
  // sort by provenance, then by profile name; makes saving easier
  if (other.mProfileFile != this->mProfileFile ) { 
    key1 = this->mProfileFile; 
    key2 = other.mProfileFile; 
  } 
  retval = (key1 < key2); 
  if (retval) compareResult = ">"; 
  
  //  dbprintf(5, QString("%1 %2 %3").arg(key1).arg(compareResult).arg(key2)); 
  
  return retval; 
}

// =============================================================
bool HostProfile::operator >(const HostProfile& other) const {
  return (other < *this); 
}
  
// =============================================================
void HostProfile::init(void) {
  mName = "Blank profile"; 
  mProfileFile = mUserHostProfileFile;
  mSidecarHost = QHostInfo::localHostName(); 
  mReadOnly = mPlay = mFullScreen = mShowControls = mUseDMX = mMpiFrameSync = false; 
  mAutoSidecarHost = mAutoBlockbusterPath = mNoSmallWindows = true;   
  return; 
}


// =============================================================
HostProfile& HostProfile::operator << (string s)  {
  QString profileString = QString(s.c_str()).simplified();
  QStringList tokens = profileString.split(QRegExp("\\s+"), QString::SkipEmptyParts); 
  mValid = false; 
  if (!tokens.size()) {
    return *this; 
  }
  else if (tokens.size() > 3 && tokens[0] == "<<" && tokens[1] == "HostProfile:"&& tokens[tokens.size()-1] == ">>")  {
    for (uint16_t t = 2; t < tokens.size()-1; t++) {
      QStringList items = tokens[t].split("=", QString::SkipEmptyParts); 
      if (items.size() != 2) {
        dbprintf(0, QString("Error: profile string token %1 is malformed\n").arg(tokens[t]));
        dbprintf(5, QString("%1 items in token: ").arg(items.size())); 
        for (uint16_t i = 0; i< items.size(); i++) {
          dbprintf(5, QString("%1 ").arg(items[i])); 
        }
        return *this; 
      } 
      //  OLD FORMAT:  QString("<< HostProfile: mReadOnly=%1, mProfileFile=%2, mHostName=%3, mName=%4, mPort=%5, mVerbosity=%6, mRsh=%7, mSetDisplay=%8, mDisplay=%9, mBlockbusterPath=%10, mAutoSidecarHost=%11, mSidecarHost=%12, mPlay=%13, mFullScreen=%14, mShowControls=%15, mUseDMX=%16, mMpiFrameSync=%17, mNoSmallWindows=%18, mAutoBlockbusterPath=%19 >>")      
      
      QString key = items[0], value = items[1]; 
      if (value.endsWith(",")) value.truncate(value.size()-1); 

      if (key == "mReadOnly")  mReadOnly = (value != "0"); 
      else if (key == "mProfileFile") mProfileFile=value;
      else if (key == "mHostName") mHostName=value;
      else if (key == "mName") mName=value;
      else if (key == "mPort") mPort=value;
      else if (key == "mVerbosity") mVerbosity=value;
      else if (key == "mRsh") mRsh = value; 
      else if (key == "mSetDisplay") mSetDisplay = (value != "false" && value != "0");
      else if (key == "mDisplay") mDisplay = value; 
      else if (key == "mBlockbusterPath") mBlockbusterPath = value.replace("%20", " "); 
      else if (key == "mAutoSidecarHost") mAutoSidecarHost  = (value != "false" && value != "0");
      else if (key == "mSidecarHost") mSidecarHost = value; 
      else if (key == "mPlay") mPlay = (value != "false" && value != "0");
      else if (key == "mFullScreen") mFullScreen = (value != "false" && value != "0");
      else if (key == "mShowControls") mShowControls = (value != "false" && value != "0");
      else if (key == "mUseDMX") mUseDMX = (value != "false" && value != "0");
      else if (key == "mMpiFrameSync") mMpiFrameSync = (value != "false" && value != "0");
      else if (key == "mNoSmallWindows") mNoSmallWindows = (value != "false" && value != "0");
      else if (key == "mAutoBlockbusterPath") mAutoBlockbusterPath = (value != "false" && value != "0"); 
      else {
        dbprintf(1, QString("Error: profile string token %1 contains unknown key %2\n").arg(tokens[t]).arg(key));
        return *this; 
      } 
    }
  }
  else  { // possible old-style profile
    dbprintf(1, QString("WARNING:  Possible old-style profile string detected.  When writing this out, we'll use newer syntax.\n")); 
    if (tokens.size() != 15) {
      dbprintf(1, QString("Warning:  HostProfile uses 15 tokens in initializer but I'm only seeing %1, so I'll be using default values for some items.\n").arg(tokens.size()));
    }
    
    if (tokens.size() > 0)  mName = tokens[0]; 
    if (tokens.size() > 1)  mHostName = tokens[1];
    if (tokens.size() > 2)  mPort = tokens[2];  
    if (tokens.size() > 3)  mVerbosity = tokens[3]; 
    if (tokens.size() > 4)  mRsh = tokens[4]; 
    if (tokens.size() > 5)  mSetDisplay = (tokens[5] == "setDisplay=true"); 
    if (tokens.size() > 6)  mDisplay = tokens[6]; 
    if (tokens.size() > 7)  mBlockbusterPath = QString(tokens[7]).replace("%20", " "); 
    if (tokens.size() > 8)  mAutoSidecarHost = (tokens[8] == "autoSidecarHost=true"); 
    if (tokens.size() > 9)  mSidecarHost = tokens[9]; 
    if (tokens.size() > 10)  mPlay = (tokens[10] == "play=true"); 
    if (tokens.size() > 11)  mFullScreen = (tokens[11] == "fullScreen=true"); 
    if (tokens.size() > 12)  mShowControls = (tokens[12] == "showControls=true"); 
    if (tokens.size() > 13)  mUseDMX = (tokens[13] == "useDMX=true"); 
    if (tokens.size() > 14)  mMpiFrameSync = (tokens[14] == "mpiFrameSync=true"); 
  } // end old-style profile string

  dbprintf(5, "Parse host profile entry without detecting any error\n"); 
  mValid = true;   
  return *this; 

}

// =============================================================
HostProfile::operator string() const {
  return QString(*this).toStdString(); 
}

// =============================================================
HostProfile::operator QString() const {
  return QString("<< HostProfile: mName=%1, mHostName=%2, mReadOnly=%3, mProfileFile=%4, mPort=%5, mVerbosity=%6, mRsh=%7, mSetDisplay=%8, mDisplay=%9, mBlockbusterPath=%10, mAutoSidecarHost=%11, mSidecarHost=%12, mPlay=%13, mFullScreen=%14, mShowControls=%15, mUseDMX=%16, mMpiFrameSync=%17, mNoSmallWindows=%18, mAutoBlockbusterPath=%19 >>")
    .arg(mName)
    .arg(mHostName)
    .arg((int)mReadOnly)
    .arg(mProfileFile)
    .arg(mPort)
    .arg(mVerbosity)
    .arg(mRsh)
    .arg((int)mSetDisplay)
    .arg(mDisplay)
    .arg(QString(mBlockbusterPath).replace(" ", "%20"))
    .arg(mAutoSidecarHost)
    .arg(mSidecarHost)
    .arg((int)mPlay)
    .arg((int)mFullScreen)
    .arg((int)mShowControls)
    .arg((int)mUseDMX)
    .arg((int)mMpiFrameSync)
    .arg((int)mNoSmallWindows)
    .arg((int)mAutoBlockbusterPath); 
}


//======================================================================
BlockbusterLaunchDialog::BlockbusterLaunchDialog(SideCar *sidecar, QString host, QString port, QString filename, connectionState state, QString rshCmd, long bbVerbose): 
  mSidecar(sidecar), mState(state), mBlockbusterPort(port.toInt()), mCurrentProfile(NULL) {

  setupUi(this); 
  rshCommandField->setText(rshCmd); 
  hostNameField->setText(host); 
  hostPortField->setText(port); 
  verboseField->setText(QString("%1").arg(bbVerbose)); 
  initMovieComboBox(filename); 
  readAndSortHostProfiles(); 
  connect(hostProfilesComboBox,  SIGNAL(editTextChanged (const QString &)),
          this, SLOT(hostProfileModified(const QString&)));
  connect(hostNameField,  SIGNAL(textEdited ( const QString & )),
          this, SLOT(hostProfileModified(const QString&)));
  connect(hostPortField, SIGNAL(textEdited ( const QString & )),
          this, SLOT(hostProfileModified(const QString&)));
  connect(verboseField, SIGNAL(textEdited ( const QString & )),
          this, SLOT(hostProfileModified(const QString&)));
  connect(rshCommandField, SIGNAL(textEdited ( const QString & )),
          this, SLOT(hostProfileModified(const QString&)));
  connect(blockbusterDisplayField, SIGNAL(textEdited ( const QString & )),
          this, SLOT(hostProfileModified(const QString&)));
  connect(blockbusterPathField, SIGNAL(textEdited ( const QString & )),
          this, SLOT(hostProfileModified(const QString&)));
  connect(fileNameComboBox, SIGNAL(editTextChanged (const QString &)),
          this, SLOT(hostProfileModified(const QString&)));
  connect(playCheckBox, SIGNAL(clicked()),
          this, SLOT(hostProfileModified()));
  connect(fullScreenCheckBox, SIGNAL(clicked()),
          this, SLOT(hostProfileModified()));
  connect(showControlsCheckBox, SIGNAL(clicked()),
          this, SLOT(hostProfileModified()));
  connect(useDMXCheckBox, SIGNAL(clicked()),
          this, SLOT(hostProfileModified()));
  connect(autoSidecarHostCheckBox, SIGNAL(clicked()),
          this, SLOT(hostProfileModified()));
  connect(autoBlockbusterPathCheckBox, SIGNAL(clicked()),
          this, SLOT(hostProfileModified()));
  connect(mpiFrameSyncCheckBox, SIGNAL(clicked()),
          this, SLOT(hostProfileModified()));
  
  return; 
}

//======================================================================
void BlockbusterLaunchDialog::on_browseButton_clicked(){
  QString filename = QFileDialog::
    getOpenFileName(this, "Choose a movie file",
                    "",
                    "Movie Files (*.sm)");
  if (filename == "") return; 
  int init = fileNameComboBox->findText(filename, Qt::MatchExactly); 
  if (init != -1) {
    fileNameComboBox->setCurrentIndex(init); 
  } else {
    fileNameComboBox->addItem(filename); 
    fileNameComboBox->setCurrentIndex(fileNameComboBox->count()-1); 
  }
  return; 
} 
//======================================================================
void BlockbusterLaunchDialog::on_deleteProfilePushButton_clicked(){
  if (!mCurrentProfile) {
    dbprintf(0, "Error:  delete Profile called with no current profile!\n"); 
    abort(); 
  }
  if (mCurrentProfile->mReadOnly) {
    QMessageBox::warning(this,  "Read-only profile",
                         QString("You cannot delete profile \"%1\".").arg(mCurrentProfile->displayName())); ; 
    return; 
  }
  QString profname = hostProfilesComboBox->currentText(); 
  QMessageBox::StandardButton answer = QMessageBox::question
    (this, tr("Confirm Deletion"), 
     tr("Are you sure you want to delete?"), 
     QMessageBox::Yes | QMessageBox::Cancel, 
     QMessageBox::Cancel); 
  if (answer == QMessageBox::Yes) {
    vector<HostProfile *>:: iterator pos = mHostProfiles.begin();
    while (pos != mHostProfiles.end() && 
           (*pos)->displayName() != profname) {
      ++pos; 
    }
    if (pos == mHostProfiles.end()) {
      dbprintf(0, "Error:  cannot find profile to match name %1\n"); 
      abort(); 
    }
    HostProfile *tmpProfile = *pos; 
    mHostProfiles.erase(pos); 
    saveAndRefreshHostProfiles(tmpProfile); 
    delete tmpProfile; 
  }    
  
  return; 
} 

bool BlockbusterLaunchDialog::ProfileNameExists(QString name){
  for (vector<HostProfile *>::iterator pos = mHostProfiles.begin();  pos != mHostProfiles.end(); pos++) {
    if ((*pos)->mName == name) return true; 
  }
  return false; 
}

// ======================================================================
void BlockbusterLaunchDialog::createNewProfile(const HostProfile *inProfile){
  HostProfile dummy; 
  if (!inProfile) inProfile = &dummy; 
  // let's find a unique name: 
  QString suggestedName = inProfile->mName; 
  if (suggestedName == "") {
    dbprintf(0, "Warning: createNewProfile():  inProfile->mName is empty\n"); 
    suggestedName = "profile"; 
  } 
  // make sure we don't suggest a duplicate name: 
  vector<HostProfile *>::iterator pos = mHostProfiles.begin(), endpos = mHostProfiles.end(); 
  while (pos != endpos) {
    if ((*pos)->mName == suggestedName) {
      suggestedName = QString("%1_copy").arg(suggestedName); 
      pos = mHostProfiles.begin(); 
    } else {
      ++pos; 
    }
  }
  bool ok = true;
  QString name = inProfile->mName; 
  while (ok && (name == dummy.mName || ProfileNameExists(name))) {
    name = QInputDialog::getText(this, tr("New Dialog"), "What do you want to call the new profile?",  QLineEdit::Normal, suggestedName, &ok); 
    if (ok && ProfileNameExists(name)) {
      QMessageBox::warning(this,  "Name already exists.",
                           QString("Sorry! Profile name \"%1\" is already taken.").arg(name)); 
    }
  }
  if (!ok) 
    return; 

  dbprintf(5, "Creating new profile from %s\n", string(*inProfile).c_str());
  mCurrentProfile = new HostProfile(name.replace(QRegExp("\\s+"), "_"), inProfile);
  dbprintf(5, "New profile is %s\n", string(*mCurrentProfile).c_str());
  mHostProfiles.push_back(mCurrentProfile); 
  saveAndRefreshHostProfiles(mCurrentProfile); 
  return; 
}


// ======================================================================
void BlockbusterLaunchDialog::on_saveProfilePushButton_clicked(){
  if (0 && mCurrentProfile->displayName() != hostProfilesComboBox->currentText()) {
    dbprintf(0, QString("Error:  current profile  %1 does not match corresponding name %2 in combo box at index %3!\n").arg(mCurrentProfile->displayName()).arg(hostProfilesComboBox->currentText()).arg(hostProfilesComboBox->currentIndex())); 
    abort(); 
  }
  mCurrentProfile->mHostName = hostNameField->text(); 
  mCurrentProfile->mPort = hostPortField->text(); 
  mCurrentProfile->mVerbosity = verboseField->text(); 
  mCurrentProfile->mRsh = rshCommandField->text(); 
  mCurrentProfile->mSetDisplay = setDisplayCheckBox->isChecked(); 
  mCurrentProfile->mDisplay = blockbusterDisplayField->text(); 
  mCurrentProfile->mBlockbusterPath = blockbusterPathField->text();
  mCurrentProfile->mAutoSidecarHost = autoSidecarHostCheckBox->isChecked(); 
  mCurrentProfile->mSidecarHost = sidecarHostNameField->text(); 
  mCurrentProfile->mPlay = playCheckBox->isChecked(); 
  mCurrentProfile->mFullScreen = fullScreenCheckBox->isChecked(); 
  mCurrentProfile->mShowControls = showControlsCheckBox->isChecked(); 
  mCurrentProfile->mUseDMX = useDMXCheckBox->isChecked(); 
  mCurrentProfile->mMpiFrameSync = mpiFrameSyncCheckBox->isChecked(); 
  mCurrentProfile->mAutoBlockbusterPath = autoBlockbusterPathCheckBox->isChecked(); 
  sortAndSaveHostProfiles(); 
  hostProfileModified(); 
  return; 
} 

// ======================================================================
void BlockbusterLaunchDialog::on_newProfilePushButton_clicked(){
  createNewProfile(NULL); 
  return; 
} 

//======================================================================
void BlockbusterLaunchDialog::on_duplicateProfilePushButton_clicked(){
  createNewProfile(mCurrentProfile); 
  return; 
}  

//======================================================================
void BlockbusterLaunchDialog::on_deleteMoviePushButton_clicked(){  
  fileNameComboBox->removeItem(fileNameComboBox->currentIndex());
  if (fileNameComboBox->count() == 0) {
    fileNameComboBox->addItem("/Type/movie/path/here"); 
  }  
  return; 
} 
//=======================================================================
void BlockbusterLaunchDialog::on_connectButton_clicked(){
  setState(BB_WAIT_CONNECTION); // Sidecar will check this
  this->hide(); 
  return; 
}

//=======================================================================
void BlockbusterLaunchDialog::on_launchButton_clicked(){
  // launch blockbuster in a separate thread, display a "waiting dialog," and if there is success, close the dialog and set mConnect to true
   
  if (fileNameComboBox->currentText() == "") {
    QMessageBox::StandardButton answer = QMessageBox::question
      (this, tr("Confirm No Movie"), 
       tr("Are you sure you want to launch without a movie file?"), 
       QMessageBox::Yes | QMessageBox::Cancel, 
       QMessageBox::Cancel); 
    if (answer != QMessageBox::Yes) {
      return; 
    }
  }
   
  setState(BB_STARTING); 
  mBlockbusterPort = mSidecar->listenForBlockbuster(); 

  statusLabel->setText(QString("Connecting to host %1").arg(hostNameField->text())); 
  // create an ssh/rsh process, connect to the remote host, run blockbuster
  QString cmd, host=hostNameField->text(); 
  if (host != "" && host != "localhost") {
    QString rshcmd = "/usr/bin/rsh"; 
    if (rshCommandField->text() != "" ) rshcmd = rshCommandField->text();
    cmd += QString("%1 %2 ")
      .arg(rshcmd)
      .arg(host); 
  }  
  QString verboseFlag = verboseField->text(); 
  if (verboseFlag == "") verboseFlag = "0"; 

  cmd += blockbusterPathField->text() + QString(" --sidecar %1:%2 -v %3")
    .arg(sidecarHostNameField->text())
    .arg(mBlockbusterPort).arg(verboseFlag); 

  if (playCheckBox->isChecked()) {
    cmd += " --play "; 
  } 
  if (fullScreenCheckBox->isChecked()) {
    cmd += " --DecorationsDisable "; 
  } 
  if (noScreensaverCheckBox->isChecked()) {
    cmd += " --no-screensaver "; 
  } 
  if (noSmallWindowsCheckBox->isChecked()) {
    cmd += " --no-small-windows "; 
  } 
  if (!showControlsCheckBox->isChecked()) {
    cmd += " --no-controls "; 
  } 
  if (setDisplayCheckBox->isChecked()) {
    cmd +=  QString(" --display %1 ")
      .arg(blockbusterDisplayField->text()); 
  }
  if (useDMXCheckBox->isChecked()) {
    if (mpiFrameSyncCheckBox->isChecked()) {
      cmd += " --mpi "; 
    }
    cmd +=  QString(" -r dmx ");
  }
  
  if (fileNameComboBox->currentText() != "" && fileNameComboBox->currentText() != "CUELAUNCH") {
    if (host != "localhost") {
      cmd += QString("\"%1\"").arg(fileNameComboBox->currentText());//movie name
      cmd += "\""; 
    } else {
      cmd += (fileNameComboBox->currentText());//movie name
    }
  }
  dbprintf(5, "Launching command %s\n", cmd.toStdString().c_str()); 


  mProcess = mSidecar->createNewBlockbusterProcess(); 
  mProcess->start(cmd); 
  
  return; 
  
}


//=======================================================================
void BlockbusterLaunchDialog::on_useDMXCheckBox_clicked(){
  mpiFrameSyncCheckBox->setEnabled(useDMXCheckBox->isChecked()); 
  fullScreenCheckBox->setChecked(useDMXCheckBox->isChecked()); 
  hostProfileModified(); 
  return; 
}

//=======================================================================
void BlockbusterLaunchDialog::on_autoSidecarHostCheckBox_clicked(){
  sidecarHostNameField->setEnabled(!autoSidecarHostCheckBox->isChecked()); 
  if (autoSidecarHostCheckBox->isChecked()) {
    sidecarHostNameField->setText(QHostInfo::localHostName()); 
  }
  hostProfileModified(); 
  return; 
}

//=======================================================================
void BlockbusterLaunchDialog::on_autoBlockbusterPathCheckBox_clicked(){
  blockbusterPathField->setEnabled(!autoBlockbusterPathCheckBox->isChecked()); 
  if (autoBlockbusterPathCheckBox->isChecked()) {
    blockbusterPathField->setText(mSidecar->getDefaultBlockbusterPath()); 
  }
  
  hostProfileModified(); 
  return; 
}

//=======================================================================
void BlockbusterLaunchDialog::on_setDisplayCheckBox_clicked(){
  blockbusterDisplayField->setEnabled(setDisplayCheckBox->isChecked()); 
  hostProfileModified(); 
  return; 
}

//=======================================================================
void BlockbusterLaunchDialog::on_hostNameField_editingFinished( ) {
  if (hostNameField->text() != "localhost") {
    if (blockbusterDisplayField->text() == ":2001") {
      blockbusterDisplayField->setText(":0"); 
    }
  }
  hostProfileModified(); 
  return; 
}

//=======================================================================
bool BlockbusterLaunchDialog::hostProfileModified(void){
  bool dirty = 
    (hostNameField->text() != mCurrentProfile->mHostName ||
     hostPortField->text() != mCurrentProfile->mPort ||
     verboseField->text() != mCurrentProfile->mVerbosity ||
     rshCommandField->text() != mCurrentProfile->mRsh ||
     blockbusterDisplayField->text() != mCurrentProfile->mDisplay ||
     setDisplayCheckBox->isChecked() != mCurrentProfile->mSetDisplay ||

     autoSidecarHostCheckBox->isChecked() != mCurrentProfile->mAutoSidecarHost ||
     (!autoSidecarHostCheckBox->isChecked() && 
      sidecarHostNameField->text() != mCurrentProfile->mSidecarHost) ||

     autoBlockbusterPathCheckBox->isChecked() != mCurrentProfile->mAutoBlockbusterPath ||
     (!autoBlockbusterPathCheckBox->isChecked() && 
      blockbusterPathField->text()  != mCurrentProfile->mBlockbusterPath)); 

  if (fileNameComboBox->currentText() != "CUELAUNCH") {
    dirty |=
      (mCurrentProfile->mPlay != playCheckBox->isChecked() ||
       mCurrentProfile->mFullScreen != fullScreenCheckBox->isChecked() ||
       mCurrentProfile->mShowControls != showControlsCheckBox->isChecked()  ||
       mCurrentProfile->mUseDMX != useDMXCheckBox->isChecked() ||
      mCurrentProfile->mMpiFrameSync != mpiFrameSyncCheckBox->isChecked() ); 
  }

  saveProfilePushButton->setEnabled(dirty && !mCurrentProfile->mReadOnly); 
  return dirty; 
}



//=======================================================================
void BlockbusterLaunchDialog::trySetProfile (QString name) {
  dbprintf(4, QString("trySetProfile(%1)\n").arg(name)); 
  if (name == "" || mCurrentProfile->mName == name) {
    return; 
  }

  int pos = mHostProfiles.size(); 
  while (pos--) {
    if (mHostProfiles[pos]->mName == name) {
      setupGuiAndCurrentProfile(pos); 
      hostProfilesComboBox->setCurrentIndex(pos); 
      return; 
    }
  }

  return;
}

//=======================================================================
void BlockbusterLaunchDialog::saveHistory(QComboBox *box, QString filename){
  filename = QString(mSidecar->mPrefs->GetValue("prefsdir").c_str()) +"/"+ filename;
  QFile histfile(filename); 
  if (! histfile.open(QIODevice::Truncate | QIODevice::WriteOnly)) {
    dbprintf(5, QString("Could not open file %1 to save history\n").arg(filename)); 
    return; 
  }
  QString text; 
  int i = 0; 
  while (i < box->count()) {
    text = box->itemText(i).trimmed(); 
    if (text != "") {
      histfile.write((text + "\n").toStdString().c_str());     
    }
    ++i; 
  }
  dbprintf(5, QString("Wrote %1 history items to %2\n").arg(i).arg(filename)); 
  histfile.close(); 
  return; 
}
  

//=======================================================================
void BlockbusterLaunchDialog::saveAndRefreshHostProfiles(HostProfile *inProfile) {
  
  HostProfile profCopy(inProfile);
  dbprintf(5, str(boost::format ("saveAndRefreshHostProfiles(%1%), profCopy=%2%\n")% string(*inProfile) % string(profCopy))); 
  sortAndSaveHostProfiles(); // sorts and saves to files
  removeHostProfiles(); 
  readAndSortHostProfiles(); // rereads them in order
  if (!hostProfilesComboBox->count() || !inProfile) {
    return; 
  }
  
  int profnum = 0; 
  vector<HostProfile *>::iterator pos = mHostProfiles.begin(), endpos = mHostProfiles.end(); 
  while (pos != endpos && **pos < profCopy) {
    ++profnum; ++pos; 
  }
  if (pos == endpos) {
    if (profnum) 
      profnum--; 
  } else {
    dbprintf(5, str(boost::format("Found pos >= profCopy: pos=%1%, profCopy=%2%\n") % string(**pos) % string(profCopy))); 
  }
  dbprintf(5,  str(boost::format("hostProfilesComboBox->setCurrentIndex(%1%)")%(profnum))); 
  hostProfilesComboBox->setCurrentIndex(profnum); 
  return; 
}

//=======================================================================
void BlockbusterLaunchDialog::removeHostProfiles(void) {
  vector<HostProfile *>::iterator pos = mHostProfiles.begin(), endpos = mHostProfiles.end(); 
  while (pos != endpos) {
    delete (*pos); 
    pos++; 
  }
  mHostProfiles.clear(); 
  hostProfilesComboBox->blockSignals(true); 
  hostProfilesComboBox->clear(); 
  hostProfilesComboBox->blockSignals(false); 
  mCurrentProfile = NULL; 
  return ;
}

//=======================================================================
void BlockbusterLaunchDialog::sortAndSaveHostProfiles(void) {
  // first sort by output file and name:  
  sort(mHostProfiles.begin(), mHostProfiles.end(), CompareHostProfiles); 
  bool foundUserProfile = false; // if this is false at the end, we'll delete the user's host profile file.  
  QString profileFile =""; 
  FILE *fp = NULL; 
  dbprintf(5, "Examining profiles to save them.\n"); 
  vector<HostProfile *>::iterator pos = mHostProfiles.begin(), endpos = mHostProfiles.end(); 
  while (pos != endpos) {
    HostProfile *profile = *pos; // for readability
    dbprintf(5, QString("Examining profile %1.\n").arg(QString(*profile))); 
    
    if ( !profile->mReadOnly) {
      if (profile->mProfileFile == profile->mUserHostProfileFile) {
        foundUserProfile = true; 
      }
      if(!fp || (profile->mProfileFile != profileFile) ) {
        profileFile = profile->mProfileFile; 
        dbprintf(5, QString("Opening new profile file %1\n").arg(profileFile)); 
        if (fp) fclose(fp); 
        fp = fopen(profileFile.toStdString().c_str(), "w");
      }
      dbprintf(5, QString("Writing profile %1 (%2) to file %3\n").arg(profile->mName).arg(QString(*profile)).arg(profileFile)); 
      fprintf(fp, "%s\n", string(*profile).c_str()); 
    } else {
      dbprintf(5, "Profile is read-only\n"); 
    }
    ++pos; 
  }
  if (!foundUserProfile) {
    unlink(HostProfile::mUserHostProfileFile.toStdString().c_str()); 
  }
  if (fp) fclose(fp); 
  return; 
}

//=======================================================================
void BlockbusterLaunchDialog::setupGuiAndCurrentProfile(int index){
  //  set mCurrentProfile correctly
  if (index == -1) return; 

  mCurrentProfile = mHostProfiles[index]; 
  if (!mCurrentProfile) {
    dbprintf(0, QString("Error: No current profile matching name %2 in combo box!\n").arg(mCurrentProfile->displayName()).arg(hostProfilesComboBox->currentText())); 
    abort(); 
  }  
  if (0 && mCurrentProfile->displayName() != hostProfilesComboBox->currentText()) {
    dbprintf(0, QString("Error:  current profile  %1 does not match corresponding name %2 in combo box at index %3!\n").arg(mCurrentProfile->displayName()).arg(hostProfilesComboBox->currentText()).arg(index)); 
    abort(); 
  }
  hostNameField->setText(mCurrentProfile->mHostName); 
  hostPortField->setText(mCurrentProfile->mPort); 
  verboseField->setText(mCurrentProfile->mVerbosity); 
  rshCommandField->setText(mCurrentProfile->mRsh); 
  setDisplayCheckBox->setChecked(mCurrentProfile->mSetDisplay); 
  blockbusterDisplayField->setText(mCurrentProfile->mDisplay); 
  blockbusterDisplayField->setEnabled(setDisplayCheckBox->isChecked());

  autoSidecarHostCheckBox->setChecked(mCurrentProfile->mAutoSidecarHost); 
  if (mCurrentProfile->mAutoSidecarHost || mCurrentProfile->mSidecarHost == ""){
    sidecarHostNameField->setText(QHostInfo::localHostName()); 
  } else {
    sidecarHostNameField->setText(mCurrentProfile->mSidecarHost);     
  }    
  sidecarHostNameField->setEnabled(!autoSidecarHostCheckBox->isChecked()); 

  if (fileNameComboBox->currentText() == "CUELAUNCH") {
    playCheckBox->setEnabled(false); 
    fullScreenCheckBox->setEnabled(false); 
    showControlsCheckBox->setEnabled(false); 
    useDMXCheckBox->setEnabled(false); 
    fileNameComboBox->setEnabled(false); 
    deleteMoviePushButton->setEnabled(false); 
    browseButton->setEnabled(false); 
  } else {
    playCheckBox->setChecked(mCurrentProfile->mPlay); 
    fullScreenCheckBox->setChecked(mCurrentProfile->mFullScreen); 
    showControlsCheckBox->setChecked(mCurrentProfile->mShowControls); 
    useDMXCheckBox->setChecked(mCurrentProfile->mUseDMX); 
    mpiFrameSyncCheckBox->setChecked(mCurrentProfile->mMpiFrameSync); 
  }
  autoBlockbusterPathCheckBox->setChecked(mCurrentProfile->mAutoBlockbusterPath); 
  if (mCurrentProfile->mAutoBlockbusterPath || mCurrentProfile->mBlockbusterPath == ""){
    blockbusterPathField->setText(mSidecar->getDefaultBlockbusterPath()); 
  } else {
    blockbusterPathField->setText(mCurrentProfile->mBlockbusterPath); 
  }    
  blockbusterPathField->setEnabled(!autoBlockbusterPathCheckBox->isChecked());
 
  hostProfileModified(); 
  return;
}

//=======================================================================
void BlockbusterLaunchDialog::on_hostProfilesComboBox_currentIndexChanged
(int index) {
  // cerr << "on_hostProfilesComboBox_currentIndexChanged( " << index << " )" << endl;
  if (hostProfileModified()) {
    if (mCurrentProfile->mReadOnly) {
      QMessageBox::StandardButton answer = QMessageBox::question
        (this, tr("Read-only profile changed"), 
         tr("You have made changes to a read-only profile.  These cannot be saved.  "
            "Would you like to save your changes to the current profile in a new profile?  If not, your changes will be lost."),      
         QMessageBox::Yes | QMessageBox::Cancel, 
         QMessageBox::Cancel); 
      if (answer == QMessageBox::Yes) {
        createNewProfile(mCurrentProfile); 
      }
    } else {
      QMessageBox::StandardButton answer = QMessageBox::question
        (this, tr("Profile changed"), 
         tr("Would you like to save your changes to the current profile?  If not, your changes will be lost."),      
         QMessageBox::Yes | QMessageBox::Discard | QMessageBox::Cancel, 
         QMessageBox::Cancel); 
      if (answer == QMessageBox::Cancel) {
        // find the current profile in the list and change to it.  
        hostProfilesComboBox->blockSignals(true); 
        hostProfilesComboBox->setCurrentIndex(hostProfilesComboBox->findText(mCurrentProfile->displayName())); 
        hostProfilesComboBox->blockSignals(false); 
        return; 
      } else if  (answer == QMessageBox::Yes) {
        on_saveProfilePushButton_clicked(); 
      } else {
        dbprintf(1, "current profile being reverted to new profile\n"); 
      }
    }
  }
  setupGuiAndCurrentProfile(index); 
  return;
}


//=======================================================================
void BlockbusterLaunchDialog::readAndSortHostProfiles(void) {
  
  char *globalProfile = getenv("SIDECAR_GLOBAL_HOST_PROFILE"); 
  int numread = 0; 
  if (globalProfile) {
    numread += readHostProfileFile(globalProfile, true); 
  }
  numread += readHostProfileFile(HostProfile::mUserHostProfileFile, false); 
  dbprintf(5, QString("Read a total of %1 profiles").arg(numread)); 
  if (mHostProfiles.size() == 0) {
    mCurrentProfile = new HostProfile;
    mHostProfiles.push_back(mCurrentProfile); 
  }

  sort(mHostProfiles.begin(), mHostProfiles.end(), CompareHostProfiles); 

  // now set up the combo box with the sorted values: 
  hostProfilesComboBox->blockSignals(true); 
  vector<HostProfile *>::iterator pos = mHostProfiles.begin(), endpos = mHostProfiles.end(); 
  while (pos != endpos) {
    HostProfile *profile = *pos; // for readability
    hostProfilesComboBox->addItem(profile->displayName());   
    dbprintf(5, QString("After adding item %1, hostProfilesComboBox->currentText() = %2\n").arg(profile->displayName()).arg(hostProfilesComboBox->currentText())); 
    ++pos; 
  }
  hostProfilesComboBox->blockSignals(false);   
  // on_hostProfilesComboBox_currentIndexChanged(hostProfilesComboBox->currentIndex()); 
  setupGuiAndCurrentProfile(hostProfilesComboBox->currentIndex()); 
  return;
}

//=======================================================================
int BlockbusterLaunchDialog::readHostProfileFile(QString filename, bool readonly) {
  // HostProfile profile; 
  int numread = 0; 
  QFile profileFile(filename); 
  if (!profileFile.open(QIODevice::ReadOnly)) {
    if (fileNameComboBox->count() == 0) {
      fileNameComboBox->addItem("/Type/movie/path/here"); 
    }
    dbprintf(5, QString("Could not load history from %1\n").arg(filename)); 
    return numread; 
  }
  int num = fileNameComboBox->findText("/Type/movie/path/here");
  while (num != -1) {
    fileNameComboBox->removeItem(num); 
    num = fileNameComboBox->findText("/Type/movie/path/here");
  }
    
  dbprintf(5, QString("Loading history from file %1\n").arg(filename)); 
  QString item, line; 
  while ((line = profileFile.readLine())!= "") {
    dbprintf(5, "Examining line: \"%s\"\n", line.toStdString().c_str());  
    QStringList tokens = line.split(QRegExp("\\s+"), QString::SkipEmptyParts); 
    if (!tokens.size()) {
      dbprintf(5, "empty line, skipping...\n"); 
      continue; 
    }
    if (tokens[0].startsWith("#") && item != "") {
      dbprintf(5, "Comment of null first token, skipping line...\n"); 
      continue; 
    }
    HostProfile *profile = new HostProfile();

    *profile << line.toStdString(); 
    profile->mProfileFile = filename;
    profile->mReadOnly = readonly;  

    mCurrentProfile = profile; 

    dbprintf(5, QString("Adding item %1: %2\n").arg(numread).arg(QString(*profile))); 
    mHostProfiles.push_back(profile); 
    numread++; 
  }

  dbprintf(5, QString("Read %1 profile strings.  %2 profiles exist now\n").arg(numread).arg( mHostProfiles.size())); 
  return numread; 
}

//=======================================================================
void BlockbusterLaunchDialog::initMovieComboBox(QString initialItem){
  fileNameComboBox->clear(); 

  QString filename = QString(mSidecar->mPrefs->GetValue("prefsdir").c_str()) +"/fileNameComboBox.history";
  QFile histfile(filename); 
  if (!histfile.open(QIODevice::ReadOnly)) {
    fileNameComboBox->addItem("/Type/movie/path/here"); 
    dbprintf(5, QString("Could not load history from %1\n").arg(filename)); 
    return; 
  }
  dbprintf(5, QString("Loading history from file %1\n").arg(filename)); 
  QString item; 
  int items = 0; 
  while ((item = histfile.readLine())!= "") {
    item = item.trimmed(); 
    if (!item.startsWith("#") && item != "" && item != "NO_MOVIE") {
      dbprintf(5, QString("Adding item: %1\n").arg(item)); 
      fileNameComboBox->addItem(item);     
      items++; 
    } else { 
      dbprintf(5, QString("Did not add item \"%1\"\n").arg(item)); 
    }
  }
  dbprintf(5, QString("Loaded %1 history items\n").arg(items)); 
  if (initialItem != "" && initialItem != "NO_MOVIE") {
    int init = fileNameComboBox->findText(initialItem, Qt::MatchExactly); 
    if (init != -1) {
      fileNameComboBox->setCurrentIndex(init); 
    } else {
      fileNameComboBox->addItem(initialItem); 
      fileNameComboBox->setCurrentIndex(fileNameComboBox->findText(initialItem, Qt::MatchExactly)); 
    }
  }

  return; 
}

