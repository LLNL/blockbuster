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
#include "xwindow.h"
#include "canvas.h"
#include "dmxglue.h"
#include "SidecarServer.h"

//  =============================================================
//  dmxRenderer -- launch and connect remote slaves at startup, manage them
//  =============================================================
dmxRenderer::dmxRenderer(ProgramOptions *opt, Canvas *canvas, Window parentWindow, QObject* parent):
  QObject(parent), NewRenderer(opt, canvas, parentWindow, "dmx"), mAllowIdleSlaves(true), 
  mNumActiveSlaves(0), mSlavesReady(false),
  haveDMX(0),  dmxWindowInfos(NULL) {

  canvas->ResizePtr = dmx_Resize;
  canvas->MovePtr = dmx_Move;

  connect(&mSlaveServer, SIGNAL(newConnection()), this, SLOT(SlaveConnected()));  
  mSlaveServer.listen(QHostAddress::Any);  //QTcpServer will choose a port for us.
  mPort = mSlaveServer.serverPort();
  
  // from dmx_Initialize():  
  uint16_t i;
  ECHO_FUNCTION(5);
   
  if(mOptions->backendRendererName == "") {
    mOptions->backendRendererName = "gl"; 
  } 

  QString msg = QString("Using renderer %1 for the backend\n")
    .arg(mOptions->backendRendererName); 
  cerr << msg.toStdString(); 
  INFO(msg);
  
  
  /* Get DMX info */
  if (IsDMXDisplay(display)) {
    /* This will reset many of the values in gRenderer */
    GetBackendInfo();
  }
  else {
#ifdef FAKE_DMX
    FakeBackendInfo();
#else
    ERROR("'%s' is not a DMX display, exiting.",
          DisplayString(display));
    exit(1);
#endif
  }
  for (i = 0; i < dmxScreenInfos.size(); i++) {
    QHostInfo info = QHostInfo::fromName(QString(dmxScreenInfos[i]->displayName).split(":")[0]);
    QHostAddress address = info.addresses().first();
    DEBUGMSG(QString("initializeing display name from %1 to %2 with result %3").arg(dmxScreenInfos[i]->displayName).arg(info.hostName()).arg(address.toString())); 
    dmxHostAddresses.push_back(address); 
    DEBUGMSG(QString("put on stack as %1").arg(dmxHostAddresses[i].toString())); 
  }
  /* Get a socket connection for each back-end instance of the player.
     For each dmxScreenInfo, launch one slave and create one QHostInfo from the name.
     Note that the slave will not generally match the HostInfo... we don't know
     what order the slaves will connect back to us.  That's in fact the point of 
     creating the QHostInfo in the first place.
  */ 
  DEBUGMSG("Launching slaves..."); 
  
  setNumDMXDisplays(dmxScreenInfos.size());
  for (i = 0; i < dmxScreenInfos.size(); i++) {
    
    if (i==0 || mOptions->slaveLaunchMethod != "mpi") {
      QString host(dmxScreenInfos[i]->displayName);
      /* remove :x.y suffix */
      if (host.contains(":")) {
        host.remove(host.indexOf(":"), 100); 
      }
      
      LaunchSlave(host);
    }
  }
  
  /*!
    Wait for all slaves to phone home
  */ 
  uint64_t msecs = 0;
  while (!slavesReady() && msecs < 30000) {// 30 secs
    //gCoreApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    gCoreApp->processEvents();
    QueueSlaveMessages(); 
    msecs += 10; 
    usleep(10000); 
  }
  if (!slavesReady()) {
    ERROR("Slaves not responding after 30 seconds"); 
    exit(10); 
  }
  
  UpdateBackendCanvases();
  
}

//  =============================================================
dmxRenderer::~dmxRenderer() {
  ECHO_FUNCTION(5);
  if (!numValidWindowInfos) return; 
  if (mCanvas != NULL) {    
    int i;
    for (i = 0; i < numValidWindowInfos; i++) {
      /* why send this message?  It does nothing in the slave! */ 
      mActiveSlaves[dmxWindowInfos[i].screen]->
        SendMessage( QString("Destroy Canvas"));
    }
    if (dmxWindowInfos)
      delete [] dmxWindowInfos;
  }
  return; 
}

//  =============================================================
int dmxRenderer::IsDMXDisplay(Display *dpy) {
  ECHO_FUNCTION(5);
  Bool b;
  int major, event, error;
  b = XQueryExtension(dpy, "DMX", &major, &event, &error);
  return (int) b;
}

//  =============================================================
void dmxRenderer::Render(int frameNumber,const Rectangle *imageRegion,
                         int destX, int destY, float zoom, int lod)
{
  
  ECHO_FUNCTION(5);
  if (!numValidWindowInfos) return; 
  
  int i;
     
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
  for (i = 0; i < numValidWindowInfos; i++) {
    const int scrn = dmxWindowInfos[i].screen;
#if 0
    if (handle[scrn]) {
      printf("  offset %d, %d\n",
             dmxScreenInfos[scrn].xoffset,
             dmxScreenInfos[scrn].yoffset);
      printf("  origin %d, %d\n",
             dmxScreenInfos[scrn].xorigin,
             dmxScreenInfos[scrn].yorigin);
      
      printf("Window %d:\n", i);
      printf("  screen: %d\n", dmxWindowInfos[i].screen);
      printf("  pos: %d, %d  %d x %d\n",
             dmxWindowInfos[i].pos.x,
             dmxWindowInfos[i].pos.y,
             dmxWindowInfos[i].pos.width,
             dmxWindowInfos[i].pos.height);
      printf("  vis: %d, %d  %d x %d\n",
             dmxWindowInfos[i].vis.x,
             dmxWindowInfos[i].vis.y,
             dmxWindowInfos[i].vis.width,
             dmxWindowInfos[i].vis.height);
    }
#endif
    if (dmxWindowInfos[i].window) {
      const XRectangle *vis = &dmxWindowInfos[i].vis;
      Rectangle newRegion;
      int newDestX, newDestY;
      
      ClipImageRegion(destX, destY, imageRegion, vis, zoom,
                      &newDestX, &newDestY, &newRegion);
      
      mActiveSlaves[scrn]->SetCurrentFrame(frameNumber); 
      mActiveSlaves[scrn]->
        SendMessage(QString("Render %1 %2 %3 %4 %5 %6 %7 %8 %9")
                    .arg(frameNumber)
                    .arg(newRegion.x) .arg(newRegion.y)
                    .arg(newRegion.width) .arg(newRegion.height)
                    .arg(newDestX) .arg(newDestY)
                    .arg(zoom).arg(lod));
      
      
    }
  }
}

//  =============================================================
void dmxRenderer::SwapBuffers(void){
  ECHO_FUNCTION(5);
  static int32_t swapID = 0; 
  if (!numValidWindowInfos) return; 
  
  /* Only check the first slave and the hell with the rest , to ensure we stay roughly in sync */ 
  int numslaves = numValidWindowInfos;
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
        
        mActiveSlaves[dmxWindowInfos[slavenum].screen];
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
  for (slaveNum=0; slaveNum< mActiveSlaves.size(); slaveNum++ ) {
    /* Not all slaves have valid buffers to swap, but they will figure this out, and if MPI is involved, it's important they all get to call MPI_Barrer(), so send swapbuffers to everybody */    
	mActiveSlaves[slaveNum]->SwapBuffers(swapID, mCanvas->playDirection, mCanvas->preloadFrames, mCanvas->startFrame, mCanvas->endFrame);
  }
  swapID ++; 
  return; 
}


//  =============================================================
void dmxRenderer::SetFrameList(FrameList *frameList) {
  ECHO_FUNCTION(5);
  uint32_t framenum;
  uint32_t i; 
  QString previousName; 
  
  mCanvas->frameList = frameList;
  
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
  files.clear(); 
  for (framenum = 0; framenum < frameList->numActualFrames(); framenum++) {
    if (previousName != frameList->getFrame(framenum)->filename) {
      files.push_back(frameList->getFrame(framenum)->filename); 
      previousName = frameList->getFrame(framenum)->filename;        
    }
  }
  
  /* Tell back-end instances to load the files */
  for (i = 0; i < dmxScreenInfos.size(); i++) {
    if (dmxWindowInfos[i].window) {
      mActiveSlaves[dmxWindowInfos[i].screen]-> SendFrameList(files);
    }
  }
  return; 
}
//  =============================================================
void dmxRenderer::Preload(uint32_t ,
                          const Rectangle *, uint32_t ) {
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

//============================================================
void dmxRenderer::SpeedTest(void) {
  uint16_t i; 
  DEBUGMSG("dmx_SpeedTest() called with %d slaves", mActiveSlaves.size()); 
  if (!numValidWindowInfos) return; 
  for (i = 0; i < mActiveSlaves.size(); i++) {
    DMXSlave *theSlave = mActiveSlaves[i]; 
    if (!theSlave) {
      throw string("Tried to send Play message to nonexistent slave");
    }
    theSlave->SendMessage("SpeedTest"); 
  }
  return; 
}


//============================================================
void dmxRenderer::SendHeartbeatToSlaves(void) {
  ECHO_FUNCTION(5);
  uint16_t i; 
  if (!numValidWindowInfos) return; 
  for (i = 0; i < mActiveSlaves.size(); i++) {
    DMXSlave *theSlave = mActiveSlaves[i]; 
    if (!theSlave) {
      throw string("Tried to send heartbeat to nonexistent slave");
    }
    theSlave->SendMessage("Heartbeat"); 
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
          << "-r" << mOptions->backendRendererName ;
    
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
         << "-r" << mOptions->backendRendererName  
         << "-messageLevel debug -slave" 
         << QString("%1:%2").arg(localHostname).arg(mPort)
         << QString(" >~/.blockbuster/slave-%1.out 2>&1").arg(localHostname);
	INFO(QString("Running command('%1 %2')\n")
         .arg(rshCommand).arg(args.join(" ")));
	
	slaveProcess->start(rshCommand, args);
	
  }
  else if (mOptions->slaveLaunchMethod == "manual") {
	/* give instructions for manual start-up */
	printf(QString("Here is the command to start blockbuster on host 1:  'blockbuster -s %2:%3 -r %4 -d $DMX_DISPLAY' \n").arg( hostname).arg( localHostname).arg(mPort).arg( mOptions->backendRendererName).toAscii());
  }
  return ;
}

//===================================================================
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

//===================================================================
void dmxRenderer::UnexpectedDisconnect(DMXSlave *theSlave) {
  ERROR(QString("Slave  host %1 disconnected unexpectedly")
        .arg(QString::fromStdString(theSlave->GetHost())));
  exit(1); 
}

//===================================================================
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
void dmxRenderer::UpdateBackendCanvases(void)
{
    
   if (!numValidWindowInfos) return; 

    int i;

    for (i = 0; i < numValidWindowInfos; i++) {
	  const int scrn = dmxWindowInfos[i].screen;
	  
	  if (!haveDMX) {
		ERROR("UpdateBackendCanvases: no not have DMX!"); 
		abort(); 
	  }
	  /*
	   * Check if we need to create any back-end windows / canvases
	   */
	  if (dmxWindowInfos[i].window && !mActiveSlaves[scrn]->HaveCanvas()) {
		mActiveSlaves[scrn]->
		  SendMessage(QString("CreateCanvas %1 %2 %3 %4 %5 %6 %7")
					  .arg( dmxScreenInfos[scrn]->displayName)
					  .arg((int) dmxWindowInfos[i].window)
					  .arg(dmxWindowInfos[i].pos.width)
					  .arg(dmxWindowInfos[i].pos.height)
					  .arg(mCanvas->depth)
					  .arg(mOptions->frameCacheSize)
					  .arg(mOptions->readerThreads));

		mActiveSlaves[scrn]->HaveCanvas(true);
		if (files.size()) {
		  /* send list of image files too */
		  if (dmxWindowInfos[i].window) {
			mActiveSlaves[scrn]->SendFrameList(files);
		  }
		}
	  }
	  
	  /*
	   * Compute new position/size parameters for the backend subwindow.
	   * Send message to back-end processes to resize/move the canvases.
	   */
	  if (i < numValidWindowInfos && dmxWindowInfos[i].window) {
		mActiveSlaves[scrn]->
		  MoveResizeCanvas(dmxWindowInfos[i].vis.x,
						   dmxWindowInfos[i].vis.y,
						   dmxWindowInfos[i].vis.width,
						   dmxWindowInfos[i].vis.height);
	  }
    }
}

/*
 * Get the back-end window information for the given window on a DMX display.
 */
void dmxRenderer::GetBackendInfo(void)
{
  
  int i, numScreens; 
  haveDMX = 0;
    
  DMXGetScreenCount(display, &numScreens);
  if ((uint32_t)numScreens != dmxScreenInfos.size()) {
	ClearScreenInfos(); 
	for (i = 0; i < numScreens; i++) {
	  DMXScreenInfo *newScreenInfo = new DMXScreenInfo; 
	  dmxScreenInfos.push_back(newScreenInfo); 
	}
	dmxWindowInfos = new DMXWindowInfo[numScreens]; 
  }
  for (i = 0; i < numScreens; i++) {
	if (!DMXGetScreenInfo(display, i, dmxScreenInfos[i])) {
	  ERROR("Could not get screen information for screen %d\n", i);
	  ClearScreenInfos(); 
	  return;
	}
  }
  

  /*!
	Ask DMX how many screens our window is overlapping and by how much.
	There is one windowInfo info for each screen our window overlaps 
   */ 
  if (!DMXGetWindowInfo(display,
						window, &numValidWindowInfos,
						numScreens,
						dmxWindowInfos)) {
	ERROR("Could not get window information for 0x%x\n",
		  (int) window);
	ClearScreenInfos(); 
	return;
  }
  uint16_t winNum = 0; 
  for (winNum=numValidWindowInfos; winNum < dmxScreenInfos.size(); winNum++) {
	dmxWindowInfos[winNum].window = 0;	
  }
  haveDMX = 1;
}

#ifdef FAKE_DMX

void dmxRenderer::FakeBackendInfo(void)
{
    

    /* two screens with the window stradling the boundary */
    const int screenWidth = 1280, screenHeight = 1024;
    const int w = 1024;
    const int h = 768;
    const int x = screenWidth - w / 2;
    const int y = 100;
    int i;

    haveDMX = 0;

    numScreens = 2;
    numWindows = 2;


#if DMX_API_VERSION == 1
    dmxScreenInfo = (DMXScreenInformation *)
        calloc(1, numScreens * sizeof(DMXScreenInformation));
#else
    dmxScreenInfo = (DMXScreenAttributes *)
        calloc(1, numScreens * sizeof(DMXScreenAttributes));
#endif

    if (!dmxScreenInfo) {
        ERROR("FakeBackendDMXWindowInfo failed!\n");
        numScreens = 0;
        return;
    }


#if DMX_API_VERSION == 1
    dmxWindowInfo = (DMXWindowInformation *)
        calloc(1, numScreens * sizeof(DMXWindowInformation));
#else
    dmxWindowInfo = (DMXWindowAttributes *)
        calloc(1, numScreens * sizeof(DMXWindowAttributes));
#endif

    if (!dmxWindowInfo) {
        ERROR("Out of memory in FakeBackendDMXWindowInfo\n");
        free(dmxScreenInfo);
        dmxScreenInfo = NULL;
        numScreens = 0;
        return;
    }

    for (i = 0; i < numScreens; i++) {
       dmxScreenInfo[i].displayName = strdup("localhost:0");

       dmxScreenInfo[i].logicalScreen = 0;
#if DMX_API_VERSION == 1
       dmxScreenInfo[i].width = screenWidth;
       dmxScreenInfo[i].height = screenHeight;
       dmxScreenInfo[i].xoffset = 0;
       dmxScreenInfo[i].yoffset = 0;
       dmxScreenInfo[i].xorigin = i * screenWidth;
       dmxScreenInfo[i].yorigin = 0;
#else
       /* xxx untested */
       dmxScreenInfo[i].screenWindowWidth = screenWidth;
       dmxScreenInfo[i].screenWindowHeight = screenHeight;
       dmxScreenInfo[i].screenWindowXoffset = 0;
       dmxScreenInfo[i].screenWindowYoffset = 0;
       dmxScreenInfo[i].rootWindowXorigin = i * screenWidth;
       dmxScreenInfo[i].rootWindowYorigin = 0;
#endif

       dmxWindowInfo[i].screen = i;
       dmxWindowInfo[i].window = 0;

#if DMX_API_VERSION == 1
       dmxWindowInfo[i].pos.x = x - dmxScreenInfo[i].xorigin;
       dmxWindowInfo[i].pos.y = y - dmxScreenInfo[i].yorigin;
#else
       dmxWindowInfo[i].pos.x = x - dmxScreenInfo[i].rootWindowXorigin;
       dmxWindowInfo[i].pos.y = y - dmxScreenInfo[i].rootWindowYorigin;
#endif

       dmxWindowInfo[i].pos.width = w;
       dmxWindowInfo[i].pos.height = h;
       dmxWindowInfo[i].vis.x = i * w / 2;
       dmxWindowInfo[i].vis.y = 0;
       dmxWindowInfo[i].vis.width = w / 2;
       dmxWindowInfo[i].vis.height = h;
    }
}
#endif

void dmxRenderer::ClearScreenInfos(void) {
  uint32_t i=0; 
  while (i < dmxScreenInfos.size()) delete dmxScreenInfos[i]; 
  dmxScreenInfos.clear(); 
  if (dmxWindowInfos) delete [] dmxWindowInfos; 
  return; 
}


/*
 * If we're about to draw <imageRegion> at destX, destY in mural coordinates,
 * clip the <imageRegion> according to <vis> (the visible region on a
 * particular screen.
 */
void dmxRenderer::ClipImageRegion(int destX, int destY, 
                                  const Rectangle *imageRegion,
                                  const XRectangle *vis, float zoom,
                                  int *destXout, int *destYout, 
                                  Rectangle *regionOut) {
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
