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
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <QTimer>
#include <QHostInfo>
#include "MovieCues.h"
#include "SidecarServer.h"
#include "errmsg.h"
#include "xwindow.h"
#include "dmxglue.h"
#include "frames.h"
#include "util.h"
#include "errmsg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdbe.h>
#include <dmxext.h>
#ifndef DmxBadXinerama
#define DMX_API_VERSION 1
#else
#define DMX_API_VERSION 2
#endif

deque<MovieEvent> DMXSlave::mIncomingEvents; 

#define MAX_SCREENS 300

#if DMX_API_VERSION == 1
typedef DMXScreenInformation DMXScreenInfo; 
typedef DMXWindowInformation DMXWindowInfo; 
#define DMXGetScreenInfo  DMXGetScreenInformation
#define DMXGetWindowInfo  DMXGetWindowInformation
#else
typedef DMXScreenAttributes DMXScreenInfo;   
typedef DMXWindowAttributes DMXWindowInfo;   
#define DMXGetScreenInfo  DMXGetScreenAttributes
#define DMXGetWindowInfo  DMXGetWindowAttributes
#endif

struct RenderInfo {
  RenderInfo(const ProgramOptions *options): 
    haveDMX(0), mBackendRenderer("gl"), 
    dmxWindowInfos(NULL), mSlaveServer(options) {}
    int haveDMX;

  QString mBackendRenderer; 
  vector<DMXScreenInfo *> dmxScreenInfos;  /* [numScreens] */
  vector<QHostAddress > dmxHostAddresses; // Qt goodness for convenience
  DMXWindowInfo *dmxWindowInfos;  /* up to numScreens */
  int numValidWindowInfos; 

  //  vector<DMXSlave *> Slaves ;  /* to conform with the local conventions */
  //int backendSockets[MAX_SCREENS];/* socket [screen] */
  //int handle[MAX_SCREENS];          /* canvas handle [screen] */
  vector<string> files;
  int sentSwapBuffers[MAX_SCREENS]; /* is there an outstanding SwapBuffers? */
  SlaveServer mSlaveServer; 
} ;
  
static RenderInfo *gRenderInfo; 

//  ============================================================================
//  DMXSlave
//  ============================================================================

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


//  =============================================================
//  SlaveServer -- launch and connect remote slaves at startup
//  =============================================================
//====================================================================
// launch a slave and love it forever
void SlaveServer::LaunchSlave(QString hostname) {
  
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
  
  if (gRenderInfo->mBackendRenderer == "") {
	gRenderInfo->mBackendRenderer = "gl"; 
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
          << "-r" << gRenderInfo->mBackendRenderer ;
    
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
         << "-r" << gRenderInfo->mBackendRenderer  
         << "-messageLevel debug -slave" 
         << QString("%1:%2").arg(localHostname).arg(mPort)
         << QString(" >~/.blockbuster/slave-%1.out 2>&1").arg(localHostname);
	INFO(QString("Running command('%1 %2')\n")
         .arg(rshCommand).arg(args.join(" ")));
	
	slaveProcess->start(rshCommand, args);
	
  }
  else if (mOptions->slaveLaunchMethod == "manual") {
	/* give instructions for manual start-up */
	printf(QString("Here is the command to start blockbuster on host 1:  'blockbuster -s %2:%3 -r %4 -d $DMX_DISPLAY' \n").arg( hostname).arg( localHostname).arg(mPort).arg( gRenderInfo->mBackendRenderer).toAscii());
  }
  return ;
}

void SlaveServer::SlaveConnected() {
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
  int screenNum = gRenderInfo->dmxScreenInfos.size();
  DEBUGMSG(QString("SlaveConnect from host %1").arg( hostAddress.toString())); 
  bool matched = false; 
  while (screenNum -- && !matched) {
    dmxAddressString = gRenderInfo->dmxHostAddresses[screenNum].toString();
    /* compare IP address of socket with IP address from domain name */ 
    DEBUGMSG(QString("Comparing IP address %1 with %2")
             .arg(newAddressString) .arg(dmxAddressString)); 
    if (hostAddress == gRenderInfo->dmxHostAddresses[screenNum] && gRenderInfo->mSlaveServer.mActiveSlaves[screenNum]==NULL) {
      mActiveSlaves[screenNum] = theSlave;
      mNumActiveSlaves++; 
      if (mNumActiveSlaves == gRenderInfo->dmxHostAddresses.size()) {
        mSlavesReady = true; 
      }
      QStringList disp  = QString(gRenderInfo->dmxScreenInfos[screenNum]->displayName).split(":"); 
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

void SlaveServer::UnexpectedDisconnect(DMXSlave *theSlave) {
  ERROR(QString("Slave  host %1 disconnected unexpectedly")
        .arg(QString::fromStdString(theSlave->GetHost())));
  exit(1); 
}

//  =============================================================
//  END SlaveServer 
//  =============================================================
 

//============================================================

MovieStatus dmx_Initialize(Canvas *canvas, const ProgramOptions *options) {
  DMXRendererGlue *glueInfo = (DMXRendererGlue *)canvas->gluePrivateData;
  uint16_t i;
  DEBUGMSG("dmx_Initialize()"); 
  gRenderInfo = new RenderInfo(options); 
  
  if(options->backendRendererName != "") {
    gRenderInfo->mBackendRenderer = options->backendRendererName;
    QString msg("User specified renderer %1 for the backend\n"); 
    cerr << msg.toStdString(); 
    INFO(msg.arg(gRenderInfo->mBackendRenderer));
  }
  else {
    QString msg("No user specified backend renderer.  Using default.\n");
    INFO(msg);
  }
  
  /* Plug in our special functions for canvas manipulations.
   * We basically override all the functions set in CreateXWindow.
   */
  canvas->SetFrameList = dmx_SetFrameList;
  canvas->Render = dmx_Render;
  canvas->Resize = dmx_Resize;
  canvas->Move = dmx_Move;
  canvas->DestroyRenderer = dmx_DestroyRenderer;
  canvas->SwapBuffers = dmx_SwapBuffers;
  /* If the UserInterface implements this routine, we should not use ours */
  if (canvas->DrawString == NULL) { 
    canvas->DrawString = dmx_DrawString;
  }
  
  /* Get DMX info */
  if (IsDMXDisplay(glueInfo->display)) {
    /* This will reset many of the values in gRenderInfo */
    GetBackendInfo(canvas);
  }
  else {
#ifdef FAKE_DMX
    FakeBackendInfo(canvas);
#else
    ERROR("'%s' is not a DMX display, exiting.",
          DisplayString(glueInfo->display));
    exit(1);
#endif
  }
  for (i = 0; i < gRenderInfo->dmxScreenInfos.size(); i++) {
    QHostInfo info = QHostInfo::fromName(QString(gRenderInfo->dmxScreenInfos[i]->displayName).split(":")[0]);
    QHostAddress address = info.addresses().first();
    DEBUGMSG(QString("initializeing display name from %1 to %2 with result %3").arg(gRenderInfo->dmxScreenInfos[i]->displayName).arg(info.hostName()).arg(address.toString())); 
    gRenderInfo->dmxHostAddresses.push_back(address); 
    DEBUGMSG(QString("put on stack as %1").arg(gRenderInfo->dmxHostAddresses[i].toString())); 
  }
  /* Get a socket connection for each back-end instance of the player.
     For each dmxScreenInfo, launch one slave and create one QHostInfo from the name.
     Note that the slave will not generally match the HostInfo... we don't know
     what order the slaves will connect back to us.  That's in fact the point of 
     creating the QHostInfo in the first place.
  */ 
  DEBUGMSG("Launching slaves..."); 
  
  gRenderInfo->mSlaveServer.setNumDMXDisplays(gRenderInfo->dmxScreenInfos.size());
  for (i = 0; i < gRenderInfo->dmxScreenInfos.size(); i++) {
    
    if (i==0 || options->slaveLaunchMethod != "mpi") {
      QString host(gRenderInfo->dmxScreenInfos[i]->displayName);
      /* remove :x.y suffix */
      if (host.contains(":")) {
        host.remove(host.indexOf(":"), 100); 
      }
      
      gRenderInfo->mSlaveServer.LaunchSlave(host);
    }
  }
  
  /*!
    Wait for all slaves to phone home
  */ 
  uint64_t msecs = 0;
  while (!gRenderInfo->mSlaveServer.slavesReady() && msecs < 30000) {// 30 secs
    //gCoreApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    gCoreApp->processEvents();
    gRenderInfo->mSlaveServer.QueueSlaveMessages(); 
    msecs += 10; 
    usleep(10000); 
  }
  if (!gRenderInfo->mSlaveServer.slavesReady()) {
    ERROR("Slaves not responding after 30 seconds");
    return MovieFailure; 
  }   
  
  UpdateBackendCanvases(canvas);
  
  return MovieSuccess; /* OK */
}


/*!
  This is set to be called at exit.  It looks at the last known
  gRenderInfo and deletes all the slaves that might be running.
*/ 
void
dmx_AtExitCleanup(void)
{
    if (!gRenderInfo || !gRenderInfo->numValidWindowInfos)
        return;

    int     slavenum = gRenderInfo->mSlaveServer.mActiveSlaves.size();

    while (slavenum--)
    {
        if (gRenderInfo->mSlaveServer.mActiveSlaves[slavenum])
            gRenderInfo->mSlaveServer.mActiveSlaves[slavenum]->
                SendMessage("Exit");
    }
    slavenum = gRenderInfo->mSlaveServer.mIdleSlaves.size();
    while (slavenum--)
    {
        gRenderInfo->mSlaveServer.mIdleSlaves[slavenum]->SendMessage("Exit");
    }
    return;
}

/*!
  Check all slaves for incoming network messages.  This is intended to be called from the main event loop regularly
*/ 
void dmx_CheckNetwork(void) {
  if (!gRenderInfo || !gRenderInfo->numValidWindowInfos) return; 
  int slavenum=0, numslaves = gRenderInfo->mSlaveServer.mActiveSlaves.size(); 
  if (!numslaves || !gRenderInfo->mSlaveServer.mActiveSlaves[0]) return; 
  while (slavenum < numslaves) {
	gRenderInfo->mSlaveServer.mActiveSlaves[slavenum++]->QueueNetworkEvents(); 
  }
  return; 
}

/*!
  just a block of code that gets used a few times when there is an error: 
*/ 
void ClearScreenInfos(void) {
  uint32_t i=0; 
  while (i < gRenderInfo->dmxScreenInfos.size()) delete gRenderInfo->dmxScreenInfos[i]; 
  gRenderInfo->dmxScreenInfos.clear(); 
  if (gRenderInfo->dmxWindowInfos) delete [] gRenderInfo->dmxWindowInfos; 
  return; 
}

/*
 * Get the back-end window information for the given window on a DMX display.
 */
void GetBackendInfo(Canvas *canvas)
{
  DMXRendererGlue *glueInfo = (DMXRendererGlue *)canvas->gluePrivateData;
  
  int i, numScreens; 
  gRenderInfo->haveDMX = 0;
    
  DMXGetScreenCount(glueInfo->display, &numScreens);
  if ((uint32_t)numScreens != gRenderInfo->dmxScreenInfos.size()) {
	ClearScreenInfos(); 
	for (i = 0; i < numScreens; i++) {
	  DMXScreenInfo *newScreenInfo = new DMXScreenInfo; 
	  gRenderInfo->dmxScreenInfos.push_back(newScreenInfo); 
	}
	gRenderInfo->dmxWindowInfos = new DMXWindowInfo[numScreens]; 
  }
  for (i = 0; i < numScreens; i++) {
	if (!DMXGetScreenInfo(glueInfo->display, i, gRenderInfo->dmxScreenInfos[i])) {
	  ERROR("Could not get screen information for screen %d\n", i);
	  ClearScreenInfos(); 
	  return;
	}
  }
  

  /*!
	Ask DMX how many screens our window is overlapping and by how much.
	There is one windowInfo info for each screen our window overlaps 
   */ 
  if (!DMXGetWindowInfo(glueInfo->display,
						glueInfo->window, &gRenderInfo->numValidWindowInfos,
						numScreens,
						gRenderInfo->dmxWindowInfos)) {
	ERROR("Could not get window information for 0x%x\n",
		  (int) glueInfo->window);
	ClearScreenInfos(); 
	return;
  }
  uint16_t winNum = 0; 
  for (winNum=gRenderInfo->numValidWindowInfos; winNum < gRenderInfo->dmxScreenInfos.size(); winNum++) {
	gRenderInfo->dmxWindowInfos[winNum].window = 0;	
  }
  gRenderInfo->haveDMX = 1;
}

#ifdef FAKE_DMX

void FakeBackendInfo(Canvas *canvas)
{
    

    /* two screens with the window stradling the boundary */
    const int screenWidth = 1280, screenHeight = 1024;
    const int w = 1024;
    const int h = 768;
    const int x = screenWidth - w / 2;
    const int y = 100;
    int i;

    gRenderInfo->haveDMX = 0;

    gRenderInfo->numScreens = 2;
    gRenderInfo->numWindows = 2;


#if DMX_API_VERSION == 1
    gRenderInfo->dmxScreenInfo = (DMXScreenInformation *)
        calloc(1, gRenderInfo->numScreens * sizeof(DMXScreenInformation));
#else
    gRenderInfo->dmxScreenInfo = (DMXScreenAttributes *)
        calloc(1, gRenderInfo->numScreens * sizeof(DMXScreenAttributes));
#endif

    if (!gRenderInfo->dmxScreenInfo) {
        ERROR("FakeBackendDMXWindowInfo failed!\n");
        gRenderInfo->numScreens = 0;
        return;
    }


#if DMX_API_VERSION == 1
    gRenderInfo->dmxWindowInfo = (DMXWindowInformation *)
        calloc(1, gRenderInfo->numScreens * sizeof(DMXWindowInformation));
#else
    gRenderInfo->dmxWindowInfo = (DMXWindowAttributes *)
        calloc(1, gRenderInfo->numScreens * sizeof(DMXWindowAttributes));
#endif

    if (!gRenderInfo->dmxWindowInfo) {
        ERROR("Out of memory in FakeBackendDMXWindowInfo\n");
        free(gRenderInfo->dmxScreenInfo);
        gRenderInfo->dmxScreenInfo = NULL;
        gRenderInfo->numScreens = 0;
        return;
    }

    for (i = 0; i < gRenderInfo->numScreens; i++) {
       gRenderInfo->dmxScreenInfo[i].displayName = strdup("localhost:0");

       gRenderInfo->dmxScreenInfo[i].logicalScreen = 0;
#if DMX_API_VERSION == 1
       gRenderInfo->dmxScreenInfo[i].width = screenWidth;
       gRenderInfo->dmxScreenInfo[i].height = screenHeight;
       gRenderInfo->dmxScreenInfo[i].xoffset = 0;
       gRenderInfo->dmxScreenInfo[i].yoffset = 0;
       gRenderInfo->dmxScreenInfo[i].xorigin = i * screenWidth;
       gRenderInfo->dmxScreenInfo[i].yorigin = 0;
#else
       /* xxx untested */
       gRenderInfo->dmxScreenInfo[i].screenWindowWidth = screenWidth;
       gRenderInfo->dmxScreenInfo[i].screenWindowHeight = screenHeight;
       gRenderInfo->dmxScreenInfo[i].screenWindowXoffset = 0;
       gRenderInfo->dmxScreenInfo[i].screenWindowYoffset = 0;
       gRenderInfo->dmxScreenInfo[i].rootWindowXorigin = i * screenWidth;
       gRenderInfo->dmxScreenInfo[i].rootWindowYorigin = 0;
#endif

       gRenderInfo->dmxWindowInfo[i].screen = i;
       gRenderInfo->dmxWindowInfo[i].window = 0;

#if DMX_API_VERSION == 1
       gRenderInfo->dmxWindowInfo[i].pos.x = x - gRenderInfo->dmxScreenInfo[i].xorigin;
       gRenderInfo->dmxWindowInfo[i].pos.y = y - gRenderInfo->dmxScreenInfo[i].yorigin;
#else
       gRenderInfo->dmxWindowInfo[i].pos.x = x - gRenderInfo->dmxScreenInfo[i].rootWindowXorigin;
       gRenderInfo->dmxWindowInfo[i].pos.y = y - gRenderInfo->dmxScreenInfo[i].rootWindowYorigin;
#endif

       gRenderInfo->dmxWindowInfo[i].pos.width = w;
       gRenderInfo->dmxWindowInfo[i].pos.height = h;
       gRenderInfo->dmxWindowInfo[i].vis.x = i * w / 2;
       gRenderInfo->dmxWindowInfo[i].vis.y = 0;
       gRenderInfo->dmxWindowInfo[i].vis.width = w / 2;
       gRenderInfo->dmxWindowInfo[i].vis.height = h;
    }
}
#endif

/*
 * Check if display is on a DMX server.  Return 1 if true, 0 if false.
 */
int IsDMXDisplay(Display *dpy)
{
   Bool b;
   int major, event, error;
   b = XQueryExtension(dpy, "DMX", &major, &event, &error);
   return (int) b;
 }


/* This SetFrameList method doesn't save a local framelist.
 */
 void dmx_SetFrameList(Canvas *canvas, FrameList *frameList){
    
    uint32_t framenum;
	uint32_t i; 
	QString previousName; 
	
    canvas->frameList = frameList;
	
    /* concatenate filenames.  We want to send only unique filenames 
     * back down to the slaves (or they'd try to load lots of files);
     * we cheat a little here and just send a filename if it's not
     * the same as the previous filename.
     *
     * Although this is not quite proper (it's allowable in the design
     * for the main program to rearrange frames so that frames from
     * different files are interleaved), it works for now.
     *
     * A more complex (but technically correct) solution would be to send
     * the list of frames as a list of {filename, frame number} pairs,
     * which is how the main program references frames.  But this would
     * require a lot more complexity and no more utility, since there's
     * no way the main program can take advange of such a feature now.
     */
    gRenderInfo->files.clear(); 
    for (framenum = 0; framenum < frameList->numActualFrames(); framenum++) {
      if (previousName != frameList->getFrame(framenum)->filename) {
	gRenderInfo->files.push_back(frameList->getFrame(framenum)->filename); 
	previousName = frameList->getFrame(framenum)->filename;        
      }
    }
	
    /* Tell back-end instances to load the files */
    for (i = 0; i < gRenderInfo->dmxScreenInfos.size(); i++) {
	  if (gRenderInfo->dmxWindowInfos[i].window) {
		gRenderInfo->mSlaveServer.mActiveSlaves[gRenderInfo->dmxWindowInfos[i].screen]->
		  SendFrameList(gRenderInfo->files);
	  }
    }
 }
 
void dmx_SetupPlay(int play, int preload, 
                   uint32_t startFrame, uint32_t endFrame) {
  uint32_t i = 0; 
  for (i = 0; i < gRenderInfo->dmxScreenInfos.size(); i++) {
    if (gRenderInfo->dmxWindowInfos[i].window) {
      gRenderInfo->mSlaveServer.mActiveSlaves
        [gRenderInfo->dmxWindowInfos[i].screen]->
        SendMessage(QString("SetPlayDirection %1 %2 %3 %4")
                    .arg(play).arg(preload).arg(startFrame).arg(endFrame));
    }
  }
  return; 
}
 
void   dmx_DestroyRenderer(Canvas *canvas){
   if (!gRenderInfo->numValidWindowInfos) return; 
   if (canvas != NULL) {
	 
     int i;
     for (i = 0; i < gRenderInfo->numValidWindowInfos; i++) {
       /* why send this message?  It does nothing in the slave! */ 
       gRenderInfo->mSlaveServer.mActiveSlaves[gRenderInfo->dmxWindowInfos[i].screen]->
         SendMessage( QString("Destroy Canvas"));
     }
     if (gRenderInfo->dmxWindowInfos)
       delete [] gRenderInfo->dmxWindowInfos;
   }
   
 }
 
 
/*
 * This function creates the back-end windows/canvases on the back-end hosts.
 *
 * The backend canvases should create child windows of the
 * window created by DMX (to work around NVIDIA memory issues,
 * and GLX visual compatibility).
 *
 * Also, update the subwindow sizes and positions as needed.
 * This is called when we create a canvas or move/resize it.
 */
void UpdateBackendCanvases(Canvas *canvas)
{
    
   if (!gRenderInfo->numValidWindowInfos) return; 

   DMXRendererGlue *glueInfo = (DMXRendererGlue *)canvas->gluePrivateData;
    int i;

    for (i = 0; i < gRenderInfo->numValidWindowInfos; i++) {
	  const int scrn = gRenderInfo->dmxWindowInfos[i].screen;
	  
	  if (!gRenderInfo->haveDMX) {
		ERROR("UpdateBackendCanvases: no not have DMX!"); 
		abort(); 
	  }
	  /*
	   * Check if we need to create any back-end windows / canvases
	   */
	  if (gRenderInfo->dmxWindowInfos[i].window && !gRenderInfo->mSlaveServer.mActiveSlaves[scrn]->HaveCanvas()) {
		gRenderInfo->mSlaveServer.mActiveSlaves[scrn]->
		  SendMessage(QString("CreateCanvas %1 %2 %3 %4 %5 %6 %7")
					  .arg( gRenderInfo->dmxScreenInfos[scrn]->displayName)
					  .arg((int) gRenderInfo->dmxWindowInfos[i].window)
					  .arg(gRenderInfo->dmxWindowInfos[i].pos.width)
					  .arg(gRenderInfo->dmxWindowInfos[i].pos.height)
					  .arg(canvas->depth)
					  .arg(glueInfo->frameCacheSize)
					  .arg(glueInfo->readerThreads));

		gRenderInfo->mSlaveServer.mActiveSlaves[scrn]->HaveCanvas(true);
		if (gRenderInfo->files.size()) {
		  /* send list of image files too */
		  if (gRenderInfo->dmxWindowInfos[i].window) {
			gRenderInfo->mSlaveServer.mActiveSlaves[scrn]->SendFrameList(gRenderInfo->files);
		  }
		}
	  }
	  
	  /*
	   * Compute new position/size parameters for the backend subwindow.
	   * Send message to back-end processes to resize/move the canvases.
	   */
	  if (i < gRenderInfo->numValidWindowInfos && gRenderInfo->dmxWindowInfos[i].window) {
		gRenderInfo->mSlaveServer.mActiveSlaves[scrn]->
		  MoveResizeCanvas(gRenderInfo->dmxWindowInfos[i].vis.x,
						   gRenderInfo->dmxWindowInfos[i].vis.y,
						   gRenderInfo->dmxWindowInfos[i].vis.width,
						   gRenderInfo->dmxWindowInfos[i].vis.height);
	  }
    }
}




void dmx_Resize(Canvas *canvas, int newWidth, int newHeight, int cameFromX)
{
    

    bb_assert(canvas);
    bb_assert(newWidth >= 0);
    bb_assert(newHeight >= 0);

    // don't forget to update the main window for Movie Cue events:
    ResizeXWindow(canvas, newWidth, newHeight, cameFromX); 
    /* I think these are redundant, as ResizeXWindow sets them
       canvas->width = newWidth;
       canvas->height = newHeight;
    */ 
    if (gRenderInfo->haveDMX) {
        GetBackendInfo(canvas);
    }

    UpdateBackendCanvases(canvas);
}


void dmx_Move(Canvas *canvas, int newX, int newY, int cameFromX)
{
  /* OLD command (by BP?) 
     nothing, Resize() should also have been called in response to
     window move/resize and the DMX stuff will get updated there.
  */
  
  /* Rich Cook -- now that we can use sidecar to move a window, we need to 
     tell the backend servers to move the windows appropriately
  */ 
  MoveXWindow(canvas, newX, newY, cameFromX); 
  if (gRenderInfo->haveDMX) {
    GetBackendInfo(canvas);
  }
  
  UpdateBackendCanvases(canvas);
  
  /* I think these are redundant, as MoveXWindow sets them
     canvas->XPos = newX;
     canvas->YPos = newY;
  */ 
} 


/*
 * If we're about to draw <imageRegion> at destX, destY in mural coordinates,
 * clip the <imageRegion> according to <vis> (the visible region on a
 * particular screen.
 */
void ClipImageRegion(int destX, int destY, const Rectangle *imageRegion,
                     const XRectangle *vis, float zoom,
                     int *destXout, int *destYout, Rectangle *regionOut)
{
    int dx, dy;

    /* Compute bounds of the image in mural space */
    int x0 = destX;
    int y0 = destY;
    int x1 = destX + (int)(imageRegion->width * zoom); /* plus epsilon? */
    int y1 = destY + (int)(imageRegion->height * zoom);

    /* Bounds of the image that's visible */
    int ix0 = imageRegion->x;
    int iy0 = imageRegion->y;
    int ix1 = imageRegion->x + imageRegion->width;
    int iy1 = imageRegion->y + imageRegion->height;

    /* initial dest position for this tile */
    dx = destX;
    dy = destY;

    /* X clipping */
    if (x1 < vis->x) {
        /* image is completely left of this tile */
        ix0 = ix1 = 0;
    }
    else if (x0 > vis->x + vis->width) {
        /* image is completely to right of this tile */
        ix0 = ix1 = 0;
    }
    else if (x1 > vis->x + vis->width) {
        /* right edge of image is clipped */
        int d = x1 - (vis->x + vis->width);
        x1 = vis->x + vis->width;
        ix1 -= (int)(d / zoom);
        if (x0 < vis->x) {
            /* left edge of image is also clipped */
            int d = vis->x - x0;
            x0 = vis->x;
            ix0 += (int)(d / zoom);
            dx = 0;
        }
        else {
            dx -= vis->x; /* bias by visible tile origin */
        }
    }
    else if (x0 < vis->x) {
        /* only left edge of image is clipped */
        int d = vis->x - x0;
        x0 = vis->x;
        ix0 += (int)(d / zoom);
        dx = 0;
    }
    else {
        /* no X clipping */
        /* bias dest pos by visible tile origin */
        bb_assert(x0 >= vis->x);
        bb_assert(x0 < vis->x + vis->width);
        dx -= vis->x;
    }

    /* Y clipping */
    if (y1 < vis->y) {
        /* image is completely above this tile */
        iy0 = iy1 = 0;
    }
    else if (y0 > vis->y + vis->height) {
        /* image is completely below this tile */
        iy0 = iy1 = 0;
    }
    else if (y1 > vis->y + vis->height) {
        /* bottom edge of image is clipped */
        int d = y1 - (vis->y + vis->height);
        y1 = vis->y + vis->height;
        iy1 -= (int)(d / zoom);
        if (y0 < vis->y) {
            /* top edge of image is also clipped */
            int d = vis->y - y0;
            y0 = vis->y;
            iy0 += (int)(d / zoom);
            dy = 0;
        }
        else {
            dy -= vis->y; /* bias by visible tile origin */
        }
    }
    else if (y0 < vis->y) {
        /* only top edge of image is clipped */
        int d = vis->y - y0;
        y0 = vis->y;
        iy0 += (int)(d / zoom);
        dy = 0;
    }
    else {
        /* no Y clipping */
        /* bias dest pos by visible tile origin */
        bb_assert(y0 >= vis->y);
        bb_assert(y0 < vis->y + vis->height);
        dy -= vis->y;
    }

    /* OK, finish up with new sub-image rectangle */
    regionOut->x = ix0;
    regionOut->y = iy0;
    regionOut->width = ix1 - ix0;
    regionOut->height = iy1 - iy0;
    *destXout = dx;
    *destYout = dy;

    /* make sure our values are good */
    bb_assert(regionOut->x >= 0);
    bb_assert(regionOut->y >= 0);
    bb_assert(regionOut->width >= 0);
    bb_assert(regionOut->height >= 0);
    bb_assert(dx >= 0);
    bb_assert(dy >= 0);
#if 0
    printf("  clipped window region: %d, %d .. %d, %d\n", x0, y0, x1, y1);
    printf("  clipped image region: %d, %d .. %d, %d\n", ix0, iy0, ix1, iy1);
    printf("  clipped render: %d, %d  %d x %d  at %d, %d  zoom=%f\n",
           x, y, w, h, dx, dy, zoom);
#endif
}


/* XXX these should really be passed to the Preload() function */
/* Works OK to save them from the previous Render call though. */
static int PrevDestX = 0;
static int PrevDestY = 0;
static float PrevZoom = 1.0;

void dmx_Render(Canvas *, int frameNumber,
                const Rectangle *imageRegion,
                int destX, int destY, float zoom, int lod)
{
    
  if (!gRenderInfo->numValidWindowInfos) return; 

    int i;


    PrevDestX = destX;
    PrevDestY = destY;
    PrevZoom = zoom;

#if 0
    printf("DMX::Render %d, %d  %d x %d  at %d, %d  zoom=%f\n",
           imageRegion->x, imageRegion->y,
           imageRegion->width, imageRegion->height,
           destX, destY, zoom);
#endif

    /*
     * Loop over DMX back-end windows (tiles) computing the region of
     * the image to display in each tile, and the clipped image's position.
     * This is a bit tricky.
     */
    for (i = 0; i < gRenderInfo->numValidWindowInfos; i++) {
        const int scrn = gRenderInfo->dmxWindowInfos[i].screen;
#if 0
        if (gRenderInfo->handle[scrn]) {
            printf("  offset %d, %d\n",
                   gRenderInfo->dmxScreenInfos[scrn].xoffset,
                   gRenderInfo->dmxScreenInfos[scrn].yoffset);
            printf("  origin %d, %d\n",
                   gRenderInfo->dmxScreenInfos[scrn].xorigin,
                   gRenderInfo->dmxScreenInfos[scrn].yorigin);

            printf("Window %d:\n", i);
            printf("  screen: %d\n", gRenderInfo->dmxWindowInfos[i].screen);
            printf("  pos: %d, %d  %d x %d\n",
                   gRenderInfo->dmxWindowInfos[i].pos.x,
                   gRenderInfo->dmxWindowInfos[i].pos.y,
                   gRenderInfo->dmxWindowInfos[i].pos.width,
                   gRenderInfo->dmxWindowInfos[i].pos.height);
            printf("  vis: %d, %d  %d x %d\n",
                   gRenderInfo->dmxWindowInfos[i].vis.x,
                   gRenderInfo->dmxWindowInfos[i].vis.y,
                   gRenderInfo->dmxWindowInfos[i].vis.width,
                   gRenderInfo->dmxWindowInfos[i].vis.height);
        }
#endif
        if (gRenderInfo->dmxWindowInfos[i].window) {
            const XRectangle *vis = &gRenderInfo->dmxWindowInfos[i].vis;
            Rectangle newRegion;
            int newDestX, newDestY;
			
            ClipImageRegion(destX, destY, imageRegion, vis, zoom,
                            &newDestX, &newDestY, &newRegion);
			
			gRenderInfo->mSlaveServer.mActiveSlaves[scrn]->SetCurrentFrame(frameNumber); 
            gRenderInfo->mSlaveServer.mActiveSlaves[scrn]->
			  SendMessage(QString("Render %1 %2 %3 %4 %5 %6 %7 %8 %9")
						  .arg(frameNumber)
						  .arg(newRegion.x) .arg(newRegion.y)
						  .arg(newRegion.width) .arg(newRegion.height)
						  .arg(newDestX) .arg(newDestY)
						  .arg(zoom).arg(lod));


        }
    }
}

void  dmx_DrawString(Canvas *canvas, int row, int column, const char *str)
{
  if (!gRenderInfo->numValidWindowInfos) return; 
    DMXRendererGlue *glueInfo = (DMXRendererGlue *) canvas->gluePrivateData;
    
    int x = (column + 1) * glueInfo->fontHeight;
    int y = (row + 1) * glueInfo->fontHeight;
    int i;

    /* Send DrawString to back-end renderers, with appropriate offsets */
    for (i = 0; i < gRenderInfo->numValidWindowInfos; i++) {
        if (gRenderInfo->dmxWindowInfos[i].window) {
            const int scrn = gRenderInfo->dmxWindowInfos[i].screen;

            int tx = x - gRenderInfo->dmxWindowInfos[i].vis.x;
            int ty = y - gRenderInfo->dmxWindowInfos[i].vis.y;
            int tcol = tx / glueInfo->fontHeight - 1;
            int trow = ty / glueInfo->fontHeight - 1;

            gRenderInfo->mSlaveServer.mActiveSlaves[scrn]->
			  SendMessage(QString("DrawString %1 %2")
						  .arg(trow).arg(tcol));
			gRenderInfo->mSlaveServer.mActiveSlaves[scrn]->
			  SendMessage(QString(str));
        }
    }
}


void dmx_SwapBuffers(Canvas *canvas){
  static int32_t swapID = 0; 
  if (!gRenderInfo->numValidWindowInfos) return; 

  /* Only check the first slave and the hell with the rest , to ensure we stay roughly in sync */ 
  int numslaves = gRenderInfo->numValidWindowInfos;
  vector<bool> incomplete; 
  incomplete.assign(numslaves, true); 
 int numcomplete = 0; 
  uint32_t usecs = 0, lastusecs = 0; 
  DEBUGMSG("Checking for pending swaps"); 
  while (numcomplete < numslaves) {
    //gCoreApp->processEvents(QEventLoop::ExcludeUserInputEvents); 
    gCoreApp->processEvents(); 
    int slavenum = numslaves;
    while (slavenum--) {
      DMXSlave *theSlave = 
        gRenderInfo->mSlaveServer.
        mActiveSlaves[gRenderInfo->dmxWindowInfos[slavenum].screen];
      if (incomplete[slavenum]) {
        incomplete[slavenum] = !theSlave->CheckSwapComplete(swapID-1); 
        if (!incomplete[slavenum]) {
          DEBUGMSG("Marking slave %d complete", slavenum); 
          numcomplete ++; 
        }
      }
      if (usecs - lastusecs > 10000) {
        DEBUGMSG("still checking swaps after %d usecs", usecs); 
        lastusecs = usecs;
      }
    }
    if (usecs > 10*1000*1000) {
      cerr << "Something is wrong. Slave has not swapped buffers after 10 seconds." << endl;
      if (gSidecarServer) {
        MovieEvent event(MOVIE_STOP_ERROR);
        gSidecarServer->AddEvent(event);
      }
      return; 
    }
    usecs += 500; 
    usleep (500);     
  }  
  
  uint16_t slaveNum = 0; 
  for (slaveNum=0; slaveNum< gRenderInfo->mSlaveServer.mActiveSlaves.size(); slaveNum++ ) {
    /* Not all slaves have valid buffers to swap, but they will figure this out, and if MPI is involved, it's important they all get to call MPI_Barrer(), so send swapbuffers to everybody */    
	gRenderInfo->mSlaveServer.mActiveSlaves[slaveNum]->SwapBuffers(swapID, canvas->playDirection, canvas->preloadFrames, canvas->startFrame, canvas->endFrame);
  }
  swapID ++; 
  return; 
}
 

//============================================================

void dmx_SendHeartbeatToSlaves(void) {
  uint16_t i; 
  if (!gRenderInfo || !gRenderInfo->numValidWindowInfos) return; 
  for (i = 0; i < gRenderInfo->mSlaveServer.mActiveSlaves.size(); i++) {
    DMXSlave *theSlave = gRenderInfo->mSlaveServer.mActiveSlaves[i]; 
    if (!theSlave) {
      throw string("Tried to send heartbeat to nonexistent slave");
    }
    theSlave->SendMessage("Heartbeat"); 
  }
  return; 
}

//============================================================
void dmx_SpeedTest(void) {
  uint16_t i; 
  DEBUGMSG("dmx_SpeedTest() called with %d slaves", gRenderInfo->mSlaveServer.mActiveSlaves.size()); 
  if (!gRenderInfo || !gRenderInfo->numValidWindowInfos) return; 
  for (i = 0; i < gRenderInfo->mSlaveServer.mActiveSlaves.size(); i++) {
    DMXSlave *theSlave = gRenderInfo->mSlaveServer.mActiveSlaves[i]; 
    if (!theSlave) {
      throw string("Tried to send Play message to nonexistent slave");
    }
    theSlave->SendMessage("SpeedTest"); 
  }
  return; 
}


#endif
