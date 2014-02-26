/*!
  SidecarServer serves up Movie Events in response to Sidecar commands. 
*/ 
#ifndef COMMAND_SERVER_H
#define COMMAND_SERVER_H

#include "events.h"
#include "QTcpServer"
#include "QTcpSocket"
#include "QStringList"
#include "QThread"
#include "QMutex"
#include "deque"
#include "errmsg.h"
#include "settings.h"
#include "Renderer.h"

#include "boost/shared_ptr.hpp"

extern class SidecarServer *gSidecarServer;

struct EventQueue {
  void AddEvent(MovieEvent &command);
  int GetEvent(MovieEvent *event);
    
  QMutex mMutex; // to protect access
  std::deque<MovieEvent> mEventQueue; // fifo
  int mEventDataSize; // useful info for device I/O operations
};

/* The TCPServer waits for incoming client connections and spawns them into threads.  Each thread can place something new in the queue when it receives it */
class SidecarServer : public QObject {
  Q_OBJECT
    public:
  SidecarServer(QObject *parent = 0): 
    QObject(parent), mSidecarSocket(NULL) {
    dbprintf(5, "SidecarServer\n"); 
    //mTcpServer.listen(QHostAddress::Any, 5959); 
    mTcpServer.listen(); 
    cerr << "Blockbuster listening for sidecar on port: " << mTcpServer.serverPort() << endl;
    connect(&mTcpServer, SIGNAL(newConnection()), this, SLOT(NewConnection()));
  }
  int GetNetworkEvent(MovieEvent *event);
  void AddEvent(MovieEvent &event);
 
  static void PromptForConnections(bool val) { mPromptForConnections = val; }

  void connectToSidecar(QString where);
  bool connected(void) { 
    return mSidecarSocket != NULL && 
      mSidecarSocket->state() == QAbstractSocket::ConnectedState; 
  }

  void SendEvent(MovieEvent event) {
    if (connected())  {
      event.mID = ++mLastSentCommandID; 
      *mSidecarSocket << event; 
      mSidecarSocket->flush(); 
    }
  }

  EventQueue mPendingEvents; 


  public slots:
  void NewSidecarSocket(QTcpSocket *newSocket); 
  void NewConnection(); 
  void incomingSidecarData(); 
  void socketError(QAbstractSocket::SocketError err); 
  void sidecarDisconnected();
 protected: 
  QTcpServer mTcpServer;
  static bool mPromptForConnections; 
  QTcpSocket *mSidecarSocket;
  uint32_t mLastReceivedCommandID, mLastSentCommandID; 
};

#endif
