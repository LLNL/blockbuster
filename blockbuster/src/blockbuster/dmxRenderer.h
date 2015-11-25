#ifndef DMXNEWRENDERER_H
#define DMXNEWRENDERER_H yes
#ifdef USE_DMX

#include "Renderer.h" // not "Renderers.h"
#include "events.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdbe.h>
#include <dmxext.h>
#undef Bool
#include <QTcpSocket>
#include <QProcess>
#include <QHostInfo>


//========================================================================
/*! 
  This class will create and launch a remote slave and handle all communications with it.  All input and output will be buffered and asynchronous, as that is how Qt rolls.  There is no need for threads, but if there does become a need, remember to create the socket and the slave process in the thread they live in, otherwise they will not get events. 
  Slaves need to be aware of where the 
*/ 
class DMXSlave: public QObject {
  /* Q_OBJECT enables Qt's signals and slots to work */ 
  Q_OBJECT
    public:
  DMXSlave(QString hostname, QTcpSocket *mSocket, int preloadFrames);
  ~DMXSlave();

  string GetHost(void) { return mRemoteHostname; }
  void setHostName(QString name) { mRemoteHostname = name.toStdString(); }

  /*!
	Send a message to the slave
  */ 
  void SendMessage(QString msg); 


  bool HaveRenderer(void) { return mHaveRenderer; }
  void HaveRenderer(bool setval) { mHaveRenderer = setval; }

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

  void SendCacheInit(int readerThreads, int maxCachedImages) {
    SendMessage(QString("CacheInit %1 %2").arg(readerThreads).arg(maxCachedImages)); 
    return; 
  }

  void SendFrameList(vector<string> &frameFiles) {
	// this is the old code from UpdateBackendRendereres --
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
    SendMessage(QString("Preload %1").arg(mPreloadFrames)); 
	return; 
  }

  void MoveResizeRenderer(int x, int y, int width, int height) {
	SendMessage(QString("MoveResizeRenderer %1 %2 %3 %4")
				.arg(width).arg(height).arg(x).arg(y));
	return; 
  }

  void SetCurrentFrame(int32_t frameNum) {
	mCurrentFrame = frameNum; 
  }

  void SwapBuffers(int32_t swapID, int play, int preload, 
                   uint32_t startFrame, uint32_t endFrame) {
    DEBUGMSG(QString("Slave %1: SwapBuffers(%2), lastSwap=%3\n").arg(GetHost().c_str()).arg(swapID).arg(mLastSwapID)); 
	SendMessage(QString("SwapBuffers %1 %2 %3 %4 %5 %6")
                .arg(mCurrentFrame).arg(swapID)
                .arg(play).arg(preload).arg(startFrame).arg(endFrame));
	return; 
  } 

  bool SlaveActive(void) {
	return mSlaveSocket && mSlaveSocket->isValid() && mSlaveAwake;        
  }

	
  /*! 
	Check to see if the slave completed its swap.  
    Note that this does not call ProcessEvents.  
  */ 
  bool CheckSwapComplete(int32_t swapID) {
	QueueNetworkEvents(); 
    return (mLastSwapID >= swapID);
  }
   
  // +================================================+
  public slots:

  /* send queued messages to the slave */ 
  void SendQueuedMessages(); 

  void SlaveSocketDisconnected(); 
  void SlaveSocketError(QAbstractSocket::SocketError err);
  //void SlaveSocketStateChanged(QAbstractSocket::SocketState state);

 signals:
  void SlaveDisconnect(DMXSlave *); 
  void Error(DMXSlave *, QString host, QString msg, bool abort); 

  //=============-==================================
  // public data:

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

  int mPreloadFrames; 
  string mRemoteHostname; // remote host to launch the slave on 
  QHostInfo mRemoteHostInfo; // as extracted from socket
  string backendRendererName, slaveLaunchMethod, mpiScript, executable, messageLevel; 
  bool mHaveRenderer; 

  int32_t mCurrentFrame, mLastSwapID; 

  bool mSlaveAwake, mShouldDisconnect; 
  QTcpSocket *mSlaveSocket; 
  //QTcpServer mSlaveServer; 
  QProcess *mSlaveProcess; 
  static deque<MovieEvent> mIncomingEvents;

  deque<QString> mQueuedMessages; //for when messages need to be delayed

}; // end DMXSlave class


//========================================================================
// dmxRenderer class and associated data...
//========================================================================
#ifndef DmxBadXinerama
#define DMX_API_VERSION 1
#else
#define DMX_API_VERSION 2
#endif


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

//========================================================================

class dmxRenderer: public QObject, public Renderer {
  Q_OBJECT
    public:
  dmxRenderer(ProgramOptions *options, qint32 parentWindowID, BlockbusterInterface *gui = NULL):
    QObject(NULL), 
    Renderer(options, parentWindowID, gui), 
    mAllowIdleSlaves(true), 
    mNumActiveSlaves(0), 
    mSlavesReady(false),
    mHaveDMX(0), mPreloadFrames(0) {
    mName ="dmx";
    mBackendRendererName = options->backendRendererName;
    mSlaveLaunchMethod = options->slaveLaunchMethod;
    mMpiScript =options-> mpiScript;
    mMpiScriptArgs = options->mpiScriptArgs;
    mExecutable = options->executable;
    mMessageLevelName = options->messageLevel->name;
    mPreloadFrames = options->preloadFrames;
    return; 
  }   
  
  //  =============================================================
  virtual ~dmxRenderer() {
    ECHO_FUNCTION(5);
    ShutDownSlaves();
    return;
  }

  //  =============================================================
  virtual void InitCache(int readerThreads, int maxCachedImages){
    for (uint32_t i = 0; i < mDmxScreenInfos.size(); i++) {
      if (mDmxWindowInfos[i].window) {
        mActiveSlaves[mDmxWindowInfos[i].screen]->
          SendCacheInit(readerThreads, maxCachedImages);        
      }
    }
    return; 
  }

    
  // ======================================================================
  virtual void BeginRendererInit(void) {
    return; 
  }

  // ======================================================================
  virtual void FinishRendererInit(void);

  // DMX SPECIFIC STUFF from Canvas: 
  /* thump-thump */
  virtual void DMXSendHeartbeat(void); 
  virtual void DMXSpeedTest(void);
  virtual void DMXCheckNetwork(void);

  void ShutDownSlaves(void); 

  void Resize(int newWidth, int newHeight, int cameFromX);
  void Move(int newX, int newY, int cameFromX);

  void DrawString(int row, int col , const char *str); 
  
  virtual void RenderActual(Rectangle ROI);

  virtual void SwapBuffers(void);
 
  virtual void SetFrameList(FrameListPtr frameList, int readerThreads, 
                              int maxCachedImages );
  void Preload(uint32_t frameNumber,
               const Rectangle *imageRegion, uint32_t levelOfDetail);

  /*! 
    This must be called frequently to catch incoming network messages
  */ 
  void CheckNetwork(void);
  
  /* Test out a new way to do DMX:
     this makes the slaves just start playing and ignore Render commands 
  */ 
  void SpeedTest(void); 
  
  int IsDMXDisplay(Display *dpy);

  // FROM SLAVESERVER: 
  void LaunchSlave(QString hostname); 
  bool slavesReady(void) { return mSlavesReady; }
  
  public slots:
  void SlaveConnected(); 
  void UnexpectedDisconnect(DMXSlave *); 
  void SlaveError(DMXSlave *, QString host, QString msg, bool abort); 
  
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


  void CreateBackendRenderers(void);
  void UpdateBackendRenderers(void);

  void GetBackendInfo(void);
#ifdef FAKE_DMX
  void FakeBackendInfo(void);
#endif

  void ClearScreenInfos(void);
  void ClipImageRegion(int destX, int destY,  Rectangle &imageRegion,
                       const XRectangle *vis, float zoom,
                       int *destXout, int *destYout);
  
 public:
  QTcpServer mSlaveServer;
  int mPort; 
  bool mAllowIdleSlaves; 
  vector<DMXSlave *>mActiveSlaves,  // stored in DMX screen order 
    mIdleSlaves; // stored first-come, first serve.  I expect actually only one of these, and right now only at ORNL
  uint32_t mNumActiveSlaves; 
  bool mSlavesReady; 
 
  int mHaveDMX;
  QString  mBackendRendererName, mSlaveLaunchMethod , mMpiScript, 
    mMpiScriptArgs, mExecutable, mMessageLevelName;
    int mPreloadFrames;
  // FROM RENDERINFO: 
  
  vector<DMXScreenInfo> mDmxScreenInfos;  /* [numScreens] */
  vector<QHostAddress > mDmxHostAddresses; // Qt goodness for convenience
  vector<DMXWindowInfo> mDmxWindowInfos;  /* up to numScreens */
  int mNumValidWindowInfos; 
  
  // vector<string> mFiles;
  // int mSentSwapBuffers[MAX_SCREENS]; /* is there an outstanding SwapBuffers? */
  
  
} ; // end dmxRenderer

#endif
#endif
