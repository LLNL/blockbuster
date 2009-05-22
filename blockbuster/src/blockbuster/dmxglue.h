/* Copyright (c) 2003 Tungsten Graphics, Inc.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files ("the
 * Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:  The above copyright notice, the Tungsten
 * Graphics splash screen, and this permission notice shall be included
 * in all copies or substantial portions of the Software.  THE SOFTWARE
 * IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _DMXGLUE_H
#define _DMXGLUE_H 1

#include <QTcpSocket>
#include <QTcpServer>
#include <QThread>
#include <QMutex>
#include <QProcess>
#include <QHostInfo>
#include <deque>
#include <vector>
#include <iostream>
#include "events.h"
#include "canvas.h"
#include "frames.h"
#include "errmsg.h"
//#include "Renderers.h"
using namespace std; 

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "canvas.h"


#define DMX_NAME "dmx"
#define DMX_DESCRIPTION "Directly render to back-end DMX servers: Use -R to specify\n         backend renderer"
MovieStatus dmx_Initialize(Canvas *canvas, const ProgramOptions *options);
void dmx_InterruptBufferSwap(void); 

/* This file details the structure that the DMX Renderer requires
 * in order to render.  It is used by the DMX Renderer itself
 * (naturally), and also any UserInterface that wishes to support
 * the X11 Renderer (through the "glue" routines associated wtih
 * the UserInterface and the Renderer).  It contains all the information
 * from the UserInterface that the Renderer needs to render.
 *
 * A structure of this sort must be loaded into the Canvas'
 * gluePrivateData pointer during "glue" initialization.
 */

 struct DMXRendererGlue {
    Display *display;
    Window window;
    int fontHeight;
    int frameCacheSize;
    int readerThreads;
} ;

/*!
  This shall be set to be called at exit.  It looks at the last known renderInfo and deletes all the slaves that might be running. 
*/ 
 void dmx_AtExitCleanup(void);

/*! 
  This must be called frequently to catch incoming network messages
*/ 
 void dmx_CheckNetwork(void);

/* kind of bogus, but gets me by for now */  
void dmx_SetFrameList(Canvas *canvas, FrameList *frameList);

/* thump-thump */
void dmx_SendHeartbeatToSlaves(void); 

/* Test out a new way to do DMX:
   this makes the slaves just start playing and ignore Render commands 
*/ 
void dmx_SpeedTest(void); 

//========================================================================
/*! 
  This class will create and launch a remote slave and handle all communications with it.  All input and output will be buffered and asynchronous, as that is how Qt rolls.  There is no need for threads, but if there does become a need, remember to create the socket and the slave process in the thread they live in, otherwise they will not get events. 
  Slaves need to be aware of where the 
*/ 
class DMXSlave: public QObject {
  /* Q_OBJECT enables Qt's signals and slots to work */ 
  Q_OBJECT
 public:
  DMXSlave(QString hostname, QTcpSocket *mSocket);
  ~DMXSlave();

  string GetHost(void) { return mRemoteHostname; }
  void setHostName(QString name) { mRemoteHostname = name.toStdString(); }

  /*!
	Send a message to the slave
  */ 
  void SendMessage(QString msg); 

  /*!
	Launch the slave. 
  */ 
  static QProcess *LaunchSlave(QString hostname, int port, 
                          const ProgramOptions *options); 

  bool HaveCanvas(void) { return mHaveCanvas; }
  void HaveCanvas(bool setval) { mHaveCanvas = setval; }

  /*! 
	Check to see if a message is waiting from the slave.  Return 1 if a message, 0 if not.  Includes network and process error messages (e.g., if the process sends us something over the socket, or if it exits or otherwise reports something to stdout/stderr that we understand, etc).
	If the event is 
  */ 
  int GetEventFromQueue(MovieEvent &event); 

  /*!
	Check the socket for any new incoming events and queue them
  */ 
  int QueueNetworkEvents(void);  
	
  void SetSlaveProcess(QProcess *proc) {mSlaveProcess = proc; }

  void SendFrameList(vector<string> &frameFiles) {
	// this is the old code from UpdateBackendCanvases --
	// need to implement
	DEBUGMSG("SendFrameList called"); 
	QString filestring("SetFrameList"); 
	vector<string>::iterator pos = frameFiles.begin(), endpos = frameFiles.end(); 
	while (pos != endpos) {
	  filestring += " ";
	  filestring += pos->c_str();
	  ++pos;
	}  
	SendMessage(filestring);
	return; 
  }

  void MoveResizeCanvas(int x, int y, int width, int height) {
	SendMessage(QString("MoveResizeCanvas %1 %2 %3 %4")
				.arg(width).arg(height).arg(x).arg(y));
	return; 
  }

  void SetCurrentFrame(int32_t frameNum) {
	mCurrentFrame = frameNum; 
  }

  void SwapBuffers(int32_t swapID) {
    DEBUGMSG(QString("Slave %1: SwapBuffers(%2), lastSwap=%3\n").arg(GetHost().c_str()).arg(swapID).arg(mLastSwapID)); 
	SendMessage(QString("SwapBuffers %1 %2").arg(mCurrentFrame).arg(swapID));
	return; 
  } 

  bool SlaveActive(void) {
	return mSlaveSocket && mSlaveSocket->isValid() && mSlaveAwake;
     // if (mRemoteHostName == "") {
    //  QHostAddress a = mSlaveSocket.peerAddress();
        
        
  }

	
  /*! 
	Wait until the slave completes its swap or dies trying. 
  */ 
  bool WaitForSwapComplete(uint timeout, int32_t swapID) {
	uint64_t usecs=0;
	QueueNetworkEvents(); 
    while (mLastSwapID < swapID && usecs/1000000 < timeout) {	  
      usleep(10000);
      usecs += 10000; 	  
      QueueNetworkEvents(); 
    } 	
    if (mLastSwapID < swapID) {
      cerr << "Timeout for swap exceeded for slave on host " << mRemoteHostname << endl;
      return false; 
    }
	return true; 
  }
   
  // +================================================+
  public slots:

  /* send queued messages to the slave */ 
  void SendQueuedMessages(); 

  void SlaveSocketDisconnected(); 
  void SlaveSocketError(QAbstractSocket::SocketError err);
  void SlaveSocketStateChanged(QAbstractSocket::SocketState state);

  //==============================================================
 protected:
  void run(); //QThread -- this gets called inside a thread when start() is called
 private:
  int EventFromMessage(const QString &msg, MovieEvent &event);
  /* queue a message for the main movie loop: */ 
  //void AddMessageToMovieQueue(const MovieEvent msg){ }

  /* Startup Method can be 
     A --  "manual" : user launches them out of band, using a command line given by blockbuster.
     B --  "rsh" : blockbuster launches each slave using rsh or ssh, based on hostnames given by DMX 
     C --  "mpi" : blockbuster launches all processes using MPI
  */
     
  string mRemoteHostname; // remote host to launch the slave on 
  QHostInfo mRemoteHostInfo; // as extracted from socket
  
  bool mHaveCanvas; 

  int32_t mCurrentFrame, mLastSwapID; 

  bool mSlaveAwake; 
  QTcpSocket *mSlaveSocket; 
  //QTcpServer mSlaveServer; 
  QProcess *mSlaveProcess; 
  static deque<MovieEvent> mIncomingEvents;

  deque<QString> mQueuedMessages; //for when messages need to be delayed

}; // end DMXSlave class

/* This structure contains renderer-specific information that we need 
 * (in addition to the "glue" information initialized by the UserInterface)
 */
class SlaveServer: public QObject {
  Q_OBJECT
    public:
  SlaveServer(QObject *parent = NULL): 
    QObject(parent), mAllowIdleSlaves(true), 
    mNumActiveSlaves(0), mSlavesReady(false) {
    connect(&mSlaveServer, SIGNAL(newConnection()), this, SLOT(SlaveConnected()));  
    mSlaveServer.listen(QHostAddress::Any);  //QTcpServer will choose a port for us.
    mPort = mSlaveServer.serverPort();
    
  }
    
    bool slavesReady(void) { return mSlavesReady; }
    
    public slots:
  void SlaveConnected(); 
  void setNumDMXDisplays(int num) {
    mActiveSlaves.resize(num, NULL);     
  }

  void QueueSlaveMessages(void) {
    int i = mActiveSlaves.size(); 
    while (i--) {
      if (mActiveSlaves[i]) {
        mActiveSlaves[i]->QueueNetworkEvents(); 
      }
    }
    return ;
  }

 public:
  QTcpServer mSlaveServer;
  int mPort; 
  bool mAllowIdleSlaves; 
  vector<DMXSlave *>mActiveSlaves,  // stored in DMX screen order 
    mIdleSlaves; // stored first-come, first serve.  I expect actually only one of these, and right now only at ORNL
  uint32_t mNumActiveSlaves; 
  bool mSlavesReady; 
 };

#endif
