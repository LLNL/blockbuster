#include "QDataStream"
#include "SidecarServer.h"
#include <iostream> 
#include "errmsg.h"
#include "QMessageBox"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>

// global variables for network communications.  Can these be not global, somehow?   
SidecarServer *gSidecarServer = NULL; 
bool SidecarServer::mPromptForConnections = true; 

//================================================================
void EventQueue ::AddEvent(MovieEvent &command) {
  mMutex.lock(); 
  mEventQueue.push_front(command); 
  mMutex.unlock(); 
  return; 
}

//================================================================
int EventQueue ::GetEvent(MovieEvent *event) {
  if (!mEventQueue.empty()) {
    mMutex.lock(); 
    *event = mEventQueue.back();       
    mEventQueue.pop_back(); 
    mMutex.unlock(); 
    
    DEBUGMSG("Got new event in eventqueue: %s\n", string(*event).c_str()); 
    return 1; 
  }
  return 0; 
}

//================================================================
void SidecarServer::NewSidecarSocket(QTcpSocket *newSocket) {
  mSidecarSocket = newSocket; 
  mLastReceivedCommandID = 0; 
  mLastSentCommandID = 0; 
  connect(mSidecarSocket, SIGNAL(readyRead()), this, SLOT(incomingSidecarData()));
  connect(mSidecarSocket, SIGNAL(error ( QAbstractSocket::SocketError  )), this, SLOT(socketError(QAbstractSocket::SocketError)));
  connect(mSidecarSocket, SIGNAL(disconnected()), this, SLOT(sidecarDisconnected()));
  /*
    We require lower latency on every packet sent so enable TCP_NODELAY.
  */ 
  int fd = mSidecarSocket->socketDescriptor(); 
  int option = 1; 
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                 &option, sizeof(option)) < 0) {
    DEBUGMSG("TCP_NODELAY setsockopt error");
  }
  incomingSidecarData(); 
  return; 
}

//===================================================================
void SidecarServer ::NewConnection(){
  /* split off a thread to handle the new command stream */
  dbprintf(1, "*** NewConnection\n"); 
  QTcpSocket *incomingSocket = mTcpServer.nextPendingConnection(); 
  if (mPromptForConnections) {
    QString msg = QString("Sidecar would like to connect from %1.  This will allow sidecar to have complete control and access information about your movie.  Allow it?").arg(incomingSocket->peerAddress().toString()); 
    if (QMessageBox::critical(NULL, "Incoming Connection", msg, QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Cancel) {
      incomingSocket->disconnectFromHost(); 
      delete incomingSocket; 
      return; 
    }
  }
  NewSidecarSocket(incomingSocket); 
  return;
}

//============================================================
void SidecarServer::incomingSidecarData() {
  dbprintf(5, "incomingSidecarData\n"); 
  MovieEvent event; 
  while (mSidecarSocket->canReadLine()) {
    try {
      (*mSidecarSocket) >> event; 
      /*!
        Enforce monotonically increasing Command IDs
      */
      if (event.mID <= mLastReceivedCommandID) {
        cerr << "Event ID " << event.mID << " is out of sequence, ignoring" << endl; 
      } else {
        mLastReceivedCommandID = event.mID; 
        mPendingEvents.AddEvent(event); 
        dbprintf(5, "Added sidecar event to pending events: %s\n", string(event).c_str()); 
      }
    }
    catch (QString msg) {
      std::cerr << std::endl << "********************************" << std::endl << msg.toStdString() << std::endl<< std::endl ; 
      //throw; 
    } catch (string msg) {
      std::cerr << msg << std::endl;
    } 
  }
  return; 
}

//============================================================
void SidecarServer::socketError(QAbstractSocket::SocketError /*err*/) {
  dbprintf(1, QString("socket error: %1.  Unable to communicate with sidecar.  Blockbuster will now exit.\n").arg(mSidecarSocket->errorString())); 
  exit(0); 
  return; 
} 

//============================================================
void SidecarServer::sidecarDisconnected() {
  dbprintf(1, "sidecarDisconnected\n"); 
  mSidecarSocket->deleteLater(); 
  mSidecarSocket = NULL; 
  return; 
}

//============================================================
void SidecarServer::AddEvent(MovieEvent &event){
  mPendingEvents.AddEvent(event);
  return;
}

//============================================================
/* return 0 if an event was NOT gotten, 1 if it WAS */
int SidecarServer::GetNetworkEvent(MovieEvent *event){
  event->mEventType = "MOVIE_NONE";
  int result = mPendingEvents.GetEvent(event); 
  return result; 
}


//============================================================
//============================================================
void SidecarServer::connectToSidecar(QString where) {
  DEBUGMSG("connectToSidecar: %s\n",where.toStdString().c_str()); 

  QStringList tokens = where.split(":"); 
  bool converted = false; 
  int port = 0; 
  if (tokens.size() == 2) {
    port = tokens[1].toInt(&converted); 
  }
  if (!converted) {
    cerr << "Bad host:port string in ConnectToSidecar" << endl; 
    return; 
  }
  NewSidecarSocket(new QTcpSocket); 
  mSidecarSocket->connectToHost(tokens[0], port); 
  return; 
} 


