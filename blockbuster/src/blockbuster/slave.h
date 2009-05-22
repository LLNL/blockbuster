#ifndef BLOCKBUSTER_SLAVE_H
#define BLOCKBUSTER_SLAVE_H

#include "settings.h"
#include <QWidget>
class ErrorGrabber: public QWidget {
  Q_OBJECT
public:
  ErrorGrabber(QObject *parent=NULL);
  virtual ~ErrorGrabber();
  public slots:
  void SocketStateChanged(QAbstractSocket::SocketState state);
  
  void SocketError(QAbstractSocket::SocketError );
 private:
  int bust;
}; 

int SlaveLoop(ProgramOptions *options);
int SlaveInitialize(ProgramOptions *options);
#endif
