#ifndef BLOCKBUSTER_SLAVE_H
#define BLOCKBUSTER_SLAVE_H

#include "canvas.h"
#include <QTcpSocket>
#include "settings.h"
#include <QWidget>

#ifdef USE_MPI
#include "mpi.h"
#endif

/*!
  This is the slave on the backend, which renders to the powerwall directly under direction of the master
*/ 
class Slave: public QWidget {
  Q_OBJECT
    public: 
  Slave(ProgramOptions *options); 
  ~Slave(); 

  bool InitNetwork(void); 
  bool GetDisplayName(void); 
  bool GetMasterMessage(QString &outMessage);
  void SendMessage(QString msg);
  void SendError(QString msg);
  bool LoadFrames(const char *files); 
  int Loop(void);

  public slots:
  void SocketStateChanged(QAbstractSocket::SocketState state);
  void SocketError(QAbstractSocket::SocketError ) ;

 private:
  ProgramOptions *mOptions; 
  int mSocketFD; // another view of the socket
  QTcpSocket mMasterSocket; // connection to the master blockbuster
  QDataStream *mMasterStream;  // still another view of the socket
  Canvas *mCanvas; 
#ifdef USE_MPI
  MPI_Comm workers;
#endif
}; 

#endif
