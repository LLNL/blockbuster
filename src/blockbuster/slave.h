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
  void SendMessage(QString msg);
  void SendError(QString msg);
  bool LoadFrames(const char *files); 
  bool GetMasterMessage(QString &outMessage);
  int Loop(void);

  public slots:
  void SocketStateChanged(QAbstractSocket::SocketState state);
  void SocketError(QAbstractSocket::SocketError ) ;

 private:
  ProgramOptions *mOptions; 
  int mSocketFD; 
  QTcpSocket mMasterSocket; // connection to the master blockbuster
  Canvas *mCanvas; 
  MPI_Comm workers;
}; 

#endif
