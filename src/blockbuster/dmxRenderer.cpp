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
#ifdef USE_DMX
#include "dmxRenderer.h"
#include <QTimer>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>


//  =============================================================
//  dmxRenderer -- launch and connect remote slaves at startup, manage them
//  =============================================================
dmxRenderer::dmxRenderer(ProgramOptions *opt, Canvas *canvas, QObject* parent):
  QObject(parent), NewRenderer(opt, canvas, "dmx"),
  mOptions(opt), mAllowIdleSlaves(true), 
  mNumActiveSlaves(0), mSlavesReady(false),
  haveDMX(0), mBackendRenderer("gl"), 
  dmxWindowInfos(NULL) {
    connect(&mSlaveServer, SIGNAL(newConnection()), this, SLOT(SlaveConnected()));  
    mSlaveServer.listen(QHostAddress::Any);  //QTcpServer will choose a port for us.
    mPort = mSlaveServer.serverPort();
}

//  =============================================================
void dmxRenderer::Render(int frameNumber,
            const Rectangle *imageRegion,
                         int destX, int destY, float zoom, int lod) {
  return; 
}


//  =============================================================
void dmxRenderer::SetFrameList(FrameList *frameList) {
  return; 
}
//  =============================================================
void dmxRenderer::Preload(uint32_t frameNumber,
                          const Rectangle *imageRegion, uint32_t levelOfDetail) {
  return; 
}


//====================================================================
/*!
  Check all slaves for incoming network messages.  This is intended to be called from the main event loop regularly
*/ 
void dmxRenderer::CheckNetwork(void) {
  if (!numValidWindowInfos) return; 
  int slavenum=0, numslaves = mActiveSlaves.size(); 
  if (!numslaves || !mActiveSlaves[0]) return; 
  while (slavenum < numslaves) {
	mActiveSlaves[slavenum++]->QueueNetworkEvents(); 
  }
  return; 
}
//====================================================================
// launch a slave and love it forever
void dmxRenderer::LaunchSlave(QString hostname) {
  
  DEBUGMSG(QString("LaunchSlave(%1)").arg(hostname)); 
  QProcess *slaveProcess = new QProcess; 
  
  QString rshCommand("rsh"); 
  const char *remoteShell = getenv("BLOCKBUSTER_RSH");
  if (remoteShell) {
    rshCommand = remoteShell;
  }
  QString localHostname = QHostInfo::localHostName(); 
  if (localHostname == "") {
    ERROR("DMXSlave::Launch -- cannot determine local host name"); 
    return ; 
  }
  
  if (mBackendRenderer == "") {
	mBackendRenderer = "gl"; 
  }
  
  //===============================================================
  if (mOptions->slaveLaunchMethod == "mpi") {
    /* rsh to the backend node and run mpiScript with mpiScriptArg as args */ 
    QStringList args; 
    args << hostname  
         << mOptions->mpiScript << mOptions->mpiScriptArgs
         << mOptions->executable;
    if  (mOptions->messageLevel && !strcmp(mOptions->messageLevel->name,"debug")) {
      args << " -messageLevel debug";
    } 
    if (mOptions->readerThreads > 0) {
      args << QString(" -threads %1 ").arg(mOptions->readerThreads);
    } 
    if (mOptions->frameCacheSize > 0) {
      args << QString(" -cache %1 ").arg(mOptions->frameCacheSize);
    } 
    if (mOptions->preloadFrames > 0) {
      args << QString(" -preload %1 ").arg(mOptions->preloadFrames);
    } 
      
    args  <<  " -slave " <<  QString("%1:%2:mpi").arg(localHostname).arg(mPort)              
          << "-u" <<  "x11"  // no reason to have GTK up and it screws up stereo
          << "-r" << mBackendRenderer ;
    
    INFO(QString("Running command('%1 %2')\n")
         .arg(rshCommand).arg(args.join(" ")));
#ifndef USE_MPI
    WARNING("You are running the slaves in MPI mode, but the master is not compiled with MPI.  If the slaves are not compiled with MPI (using USE_MPI=1 when running make), then they will not work and you will not get a picture. "); 
#endif
    
    slaveProcess->start(rshCommand, args); 
    
  }  
  else if (mOptions->slaveLaunchMethod == "rsh") {
	// Qt book p 289: 
	QStringList args; 
	args << hostname  << mOptions->executable
         << "-u" <<  "x11" // no reason to have GTK up and it screws up stereo
         << "-r" << mBackendRenderer  
         << "-messageLevel debug -slave" 
         << QString("%1:%2").arg(localHostname).arg(mPort)
         << QString(" >~/.blockbuster/slave-%1.out 2>&1").arg(localHostname);
	INFO(QString("Running command('%1 %2')\n")
         .arg(rshCommand).arg(args.join(" ")));
	
	slaveProcess->start(rshCommand, args);
	
  }
  else if (mOptions->slaveLaunchMethod == "manual") {
	/* give instructions for manual start-up */
	printf(QString("Here is the command to start blockbuster on host 1:  'blockbuster -s %2:%3 -r %4 -d $DMX_DISPLAY' \n").arg( hostname).arg( localHostname).arg(mPort).arg( mBackendRenderer).toAscii());
  }
  return ;
}

void dmxRenderer::SlaveConnected() {
  /*!
    Called when a slave launched previously connects to us
  */ 
  /*  Match the slave to one of the stored DMXDisplays  */ 
  DEBUGMSG("SlaveConnected called"); 
  QTcpSocket *theSocket = mSlaveServer.nextPendingConnection(); 
  QHostAddress hostAddress = theSocket->peerAddress(); 
  QString newAddressString = hostAddress.toString(), 
    dmxAddressString; 
  DMXSlave *theSlave =  new DMXSlave(newAddressString, theSocket, mOptions->preloadFrames);
  int screenNum = dmxScreenInfos.size();
  DEBUGMSG(QString("SlaveConnect from host %1").arg( hostAddress.toString())); 
  bool matched = false; 
  while (screenNum -- && !matched) {
    dmxAddressString = dmxHostAddresses[screenNum].toString();
    /* compare IP address of socket with IP address from domain name */ 
    DEBUGMSG(QString("Comparing IP address %1 with %2")
             .arg(newAddressString) .arg(dmxAddressString)); 
    if (hostAddress == dmxHostAddresses[screenNum] && mActiveSlaves[screenNum]==NULL) {
      mActiveSlaves[screenNum] = theSlave;
      mNumActiveSlaves++; 
      if (mNumActiveSlaves == dmxHostAddresses.size()) {
        mSlavesReady = true; 
      }
      QStringList disp  = QString(dmxScreenInfos[screenNum]->displayName).split(":"); 
      QString remoteDisplay = disp[0];
      theSlave->setHostName(disp[0]); 
      if (disp.size() == 2) {
        remoteDisplay = QString(":%1").arg(disp[1]); // use hardware
      }
      theSlave->SendMessage(QString("DISPLAY %1").arg(remoteDisplay)); 
      theSlave->SendMessage("SlaveIdle false");       
      matched = true; 
      DEBUGMSG("Found matching DMX display %d", screenNum); 
    }
  }    
  
  if (!matched) {
    DEBUGMSG("Failed to find matching DMX display"); 
    if (mAllowIdleSlaves) {
      theSlave->SendMessage("SlaveIdle true"); 
      mIdleSlaves.push_back(theSlave); 
    } else {
      ERROR(QString("Slave  host %1 not found in DMX screens")
            .arg(QString::fromStdString(theSlave->GetHost())));
    }
  }
}

void dmxRenderer::UnexpectedDisconnect(DMXSlave *theSlave) {
  ERROR(QString("Slave  host %1 disconnected unexpectedly")
        .arg(QString::fromStdString(theSlave->GetHost())));
  exit(1); 
}

//  =============================================================
//  END dmxRenderer 
//  =============================================================
 
//  ============================================================================
//  DMXSlave
//  ============================================================================
deque<MovieEvent> DMXSlave::mIncomingEvents; 

DMXSlave::DMXSlave(QString hostname, QTcpSocket *mSocket, int preloadFrames):
  mPreloadFrames(preloadFrames), mRemoteHostname(hostname.toStdString()), 
  mHaveCanvas(false), 
  mCurrentFrame(0),  mLastSwapID(-1), 
  mSlaveAwake(false), mShouldDisconnect(false), 
  mSlaveSocket(mSocket), mSlaveProcess(NULL) {
  /* QObject::connect(mSocket, 
     SIGNAL(stateChanged(QAbstractSocket::SocketState)), 
     this, 
     SLOT(SlaveSocketStateChanged(QAbstractSocket::SocketState ))); 
  */ 
  QObject::connect(mSocket, SIGNAL(disconnected()), 
                   this, SLOT(SlaveSocketDisconnected())); 
  QObject::connect(mSocket, SIGNAL(error(QAbstractSocket::SocketError)), 
                   this, SLOT(SlaveSocketError(QAbstractSocket::SocketError ))); 
  mRemoteHostInfo = QHostInfo::fromName(hostname);

  int fd = mSocket->socketDescriptor(); 
  /*
    We require lower latency on every packet sent so enable TCP_NODELAY.
  */ 
  int option = 1; 
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                 &option, sizeof(option)) < 0) {
    DEBUGMSG("TCP_NODELAY setsockopt error");
  }
  
  return; 

}

//========================================================================
DMXSlave::~DMXSlave() {
  if (mSlaveProcess && mSlaveProcess->state() != QProcess::NotRunning) {
	//mSlaveProcess.terminate(); // SIGTERM (use kill() if not strong enough) 
	cerr << "killing process " << mSlaveProcess->pid()<< endl; 
	SendMessage("Exit"); 
	
	//mSlaveProcess.close(); // close I/O
	//mSlaveProcess.kill(); // SIGKILL 
    //delete mSlaveProcess; 
  }
  return; 
}


//=========================================================================
void DMXSlave::SlaveSocketDisconnected(){
  DEBUGMSG("SlaveSocketDisconnected: host %s", mRemoteHostname.c_str());
  if (!mShouldDisconnect) {
    emit SlaveDisconnect(this); 
  }
  return; 
}

//=========================================================================
void DMXSlave::SlaveSocketError(QAbstractSocket::SocketError ){
  //  AddMessageToMovieQueue(MovieEvent(MOVIE_SLAVE_ERROR, mSlaveSocket->errorString())); 
  DEBUGMSG("SlaveSocketError: host %s got error: \"%s\",state: %d", 
           mRemoteHostname.c_str(), 
           (const char *)mSlaveSocket->errorString().toAscii(), 
           (int)mSlaveSocket->state());
  if (!mShouldDisconnect) {
    emit SlaveDisconnect(this); 
  }
  return; 
}


//=========================================================================
/*void DMXSlave::SlaveSocketStateChanged(QAbstractSocket::SocketState state) {
  DEBUGMSG("SlaveSocketStateChanged: host %s, state %d", mRemoteHostname.c_str(), (int)state); 
  return; 
  }
*/ 

//=========================================================================
int DMXSlave::EventFromMessage(const QString &, MovieEvent &) {
  return 0; 
}

//=========================================================================
/*!
  Check the socket for any new incoming events and queue them
  return number of events queued. 
*/ 
int DMXSlave::QueueNetworkEvents(void) {
  //  DEBUGMSG(QString("QueueNetworkEvents() for host %1").arg(mRemoteHostname.c_str())); 
  int numqueued = 0;   
  while(mSlaveSocket && mSlaveSocket->state() == QAbstractSocket::ConnectedState 
        && mSlaveSocket->bytesAvailable()) {	
    QString msg; 
    QDataStream instream(mSlaveSocket); 
    instream >> msg; // This is so freaking handy:  data sent as a QString will arrive as a QString, no need to loop and check for various conditions. 
    MovieEvent event; 
    DEBUGMSG((QString("Got message from slave on host %1: ").arg(mRemoteHostname.c_str()) + msg)); 	  
    if (msg.startsWith("Slave awake")) {
      mSlaveAwake = true; 
    } else if (msg.startsWith("SwapBuffers complete")) {
      QStringList tokens=msg.split(" "); 
      if (tokens.size() != 4) {
        ERROR("Bad SwapBuffers complete message"); 
        continue;
      }
      DEBUGMSG("Slave %s completed frame swap ID %d.", mRemoteHostname.c_str(), tokens[3].toInt()); 
      mLastSwapID = tokens[3].toInt(); 
    }
    else if (EventFromMessage(msg, event)) {
      mIncomingEvents.push_back(event); 
      numqueued++; 
    }
    else {
      //DEBUGMSG(QString("Unknown message from slave: %1").arg(msg).toAscii()); 
      cerr << QString("Unknown message from slave on host %1: \"%2\"").arg(mRemoteHostname.c_str()).arg(msg).toStdString() << endl; 
    }
  }
  if (!mSlaveSocket) {
    DEBUGMSG("Nonexistent slave socket in QueueNetworkEvents for slave %s", mRemoteHostname.c_str()); 
  }
  else if (!mSlaveSocket->isValid()) {
    DEBUGMSG("invalid slave socket in QueueNetworkEvents for slave %s", mRemoteHostname.c_str()); 
    
  }
  return numqueued; 
}

//=========================================================================
/*!
  This is never called directly, it is only used when messages are queued before
  the slave has contacted us back. I use this system to prevent waiting for one slave
  when others are ready. 
  Send all queued message to the remote slave if possible, or reschedule them. 
*/ 
void DMXSlave::SendQueuedMessages() {
  /*!
	have messages already been sent? 
  */ 
  if (!mQueuedMessages.size()) return;
  /*! 
	Is the socket created yet?  If not, reschedule this whole thing for later
  */ 
  if (!mSlaveSocket) {
	QTimer::singleShot(500, this, SLOT(SendQueuedMessages())); 
	return; 
  }
  /*!
	OK, we can send the messages
  */ 
  while (mQueuedMessages.size()) {
	SendMessage(mQueuedMessages.front()); 
	mQueuedMessages.pop_front(); 
  }
  return; 
}
//=========================================================================
/*!
  Send a message to the remote slave, or schedule it for later.
  No special formatting is applied to the message here. 
*/ 
void DMXSlave::SendMessage(QString msg) {
  if (!mSlaveSocket) {
	mQueuedMessages.push_back(msg);  
	// wake up in 500 ms and try to send the messages
	QTimer::singleShot(500, this, SLOT(SendQueuedMessages())); 
	return; 
  }
  /*!
	Has something evil happened?  
  */ 
  DEBUGMSG(QString("checking for valid socket to host %1").arg(mRemoteHostname.c_str())); 
  if (!mSlaveSocket->isValid()) {
	ERROR("Bad socket in SendQueuedMessages"); 
	return;
  }
  QDataStream outstream(mSlaveSocket);
  DEBUGMSG(QString("Sending message to host %1 : \"%2\"").arg(mRemoteHostname.c_str()).arg(msg)); 
  
  outstream << msg; // data sent as a QString arrives as a QString, no terminating token necessary
  mSlaveSocket->flush(); 
  //  mSlaveSocket->waitForBytesWritten(-1); 
  DEBUGMSG(QString("Completed sending message to host %1 : \"%2\"").arg(mRemoteHostname.c_str()).arg(msg)); 
  
  return; 
}


//  =============================================================
//  End DMXSlave
//  =============================================================


#endif
