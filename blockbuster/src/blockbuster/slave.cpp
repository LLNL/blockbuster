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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <QWidget>
#include <QStringList>
#include "errmsg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "slave.h"
#include "errmsg.h"
#include "splash.h"
#include "frames.h"
#include "smFrame.h"
#include <time.h>
#include "util.h"
#include <fstream>

using namespace std; 


//=========================================================
/*
  Initializing the Slave: 
  Start the slave socket and get a display.  It's necessary to do this early
  because the master sends the DISPLAY and we want it to be set before
  GTK snarfs it up. 
*/
Slave::Slave(ProgramOptions *options):
  mOptions(options), mSocketFD(0), mRenderer(NULL) {
  resetFPS(); 
  SuppressMessageDialogs(true); 
  DEBUGMSG("Slave() called"); 
  DEBUGMSG("useMPI is %d\n", mOptions->useMPI); 
  
  return; 
}


//=========================================================
/*!
  destructor
*/
Slave::~Slave() {
  /* Done with the canvas */
  if (mRenderer) {
    delete mRenderer;
  }
  return;
}

//=========================================================
/*!
  initialize the connection to the master blockbuster instance. 
*/
bool Slave::InitNetwork(void) {
  mMasterSocket.connectToHost(mOptions->masterHost, mOptions->masterPort); 
  if (!mMasterSocket.waitForConnected(5000)) {
    QString err = mMasterSocket.errorString(); 
    cerr << err.toStdString() << endl; 
    ERROR("Connection to master FAILED."); 
    return false; 
  } else {
    INFO("Connection to master succeeded."); 
    SendMessage(QString("Slave awake")); 
  }
    
  QObject::connect(&mMasterSocket, 
                   SIGNAL(stateChanged(QAbstractSocket::SocketState)), 
                   this, 
                   SLOT(SocketStateChanged(QAbstractSocket::SocketState ))); 
  QObject::connect(&mMasterSocket, 
                   SIGNAL(error(QAbstractSocket::SocketError)), 
                   this, 
                   SLOT(SocketError(QAbstractSocket::SocketError ))); 
  mSocketFD = mMasterSocket.socketDescriptor(); 
  mMasterStream = new QDataStream (&mMasterSocket);
    
 
  /*
    We require lower latency on every packet sent so enable TCP_NODELAY.
  */ 
  int option = 1; 
  if (setsockopt(mSocketFD, IPPROTO_TCP, TCP_NODELAY,
                 &option, sizeof(option)) < 0) {
    DEBUGMSG("TCP_NODELAY setsockopt error");
  }
  
  
  DEBUGMSG("Slave ready for messages, mMasterSocket in state %d", mMasterSocket.state());

  return true; 

}

//=========================================================
/*! 
  Explicitly break this out for clarity
*/
bool Slave::GetDisplayName(void) {
  DEBUGMSG("GetDisplayName()"); 
  // the next thing that happens is the master sends the display info
  QString message, token; 
  while (1) {
    
    gCoreApp->processEvents(QEventLoop::ExcludeUserInputEvents); 
    if (mMasterSocket.bytesAvailable()) {
      (*mMasterStream) >> message; 
    
      //if (GetNextMessage(message)) {
      QStringList messageList = message.split(" "); 
      token = messageList[0]; 
      DEBUGMSG((QString("Slave got message: \"") + message + "\"")); 
      if (token != "DISPLAY") {
        QString msg("Expected DISPLAY command but got something else!"); 
        ERROR(msg); 
        SendError(msg); 
        return false; 
      } 
      mOptions->displayName = messageList[1]; 
      DEBUGMSG(QString("set display to %1").arg(messageList[1])); 
      break; 
    } else {
      DEBUGMSG("Waiting for DISPLAY message from server"); 
      usleep (500*1000); 
    }
  }
  DEBUGMSG("END SlaveInitialize"); 
  return true; 
}

//=========================================================
/*!
  receiving a message -- placed here to enable tinkering
*/ 
bool Slave::GetMasterMessage(QString &outMessage) {
  if (mMasterSocket.state() != QAbstractSocket::ConnectedState) {
    ERROR("Error:  lost connection to the master server (state is %d).  Exiting.\n", mMasterSocket.state());
    outMessage = "disconnected"; 
    return true; 
  }
  if (mMasterSocket.bytesAvailable()) {      
    (*mMasterStream) >> outMessage;         
    return true; 
  }
  return false; 
}

//=========================================================
/*! 
  Provide a standard message format:
*/ 
void Slave::SendMessage(QString msg) {
  QDataStream stream(&mMasterSocket); 
  DEBUGMSG((QString("Slave Sending message: ")+msg));
  stream << msg;
  mMasterSocket.waitForBytesWritten(-1);
  mMasterSocket.flush(); 
  DEBUGMSG("Message sent");
}

//=========================================================
/*! 
  Provide a standard error message format:
*/ 
void Slave::SendError(QString msg) {
  ERROR(msg); 
  SendMessage(QString("ERROR: ")+msg);  
  return; 
}

//=========================================================
bool Slave::LoadFrames(const char *files)
{
  
  DEBUGMSG("LoadFrames"); 
  FrameListPtr frames;
  int numFiles = 0;
  char *start, *lastChar;

  QStringList fileList; 
  start = strdup(files);
  lastChar = start + strlen(start);

  while (1) {
    char *end = strchr(start, ' ');
    if (!end)
      end = (char *) lastChar;
    if (end && end > start) {
      /* found a filename between <start> and <end> */
      char save = *end;
      *end = 0;
      /* save this filename */
      fileList.append(start); 
      numFiles++;
      /* restore character at <end> */
      *end = save;
      if (end == lastChar)
        break;
      start = end + 1;
    }
    else {
      break;
    }
  }
  free(start);
  /* debug */
  if (0) {
    int i;
    for (i = 0; i < numFiles; i++) {
      printf("File %d: %s\n", i, fileList[i].toStdString().c_str());
    }
  }

  if (numFiles == 1 &&
      fileList[0] == splashScreenFrameListPtr->getFrame(0)->mFilename.c_str()) {
    frames = splashScreenFrameListPtr;
  }
  else {
    frames.reset(new FrameList); 
    if (frames) {
      frames->LoadFrames(fileList);
    }
  }
  if (frames) {
    mRenderer->SetFrameList(frames);
  }
  resetFPS(); 
  return true;
}


//=========================================================
void Slave::SocketStateChanged(QAbstractSocket::SocketState state) {
  DEBUGMSG("Socket state changed to %d", state); 
  return; 
}

//=========================================================
void Slave::SocketError(QAbstractSocket::SocketError ) {
  DEBUGMSG(QString("Socket error: %1").arg(mMasterSocket.errorString())); 
  return; 
}

//=========================================================
void Slave::resetFPS(void) {
  recentFrameCount = 0; 
  recentStartTime = GetCurrentTime(); 
  return; 
}
//=========================================================
/*! 
  Compute frames per second 
*/ 
void Slave::updateAndReportFPS(void) {
  recentFrameCount++;
  double currentTime = GetCurrentTime();
  double elapsedTime = currentTime - recentStartTime;
  if (elapsedTime >= 1.0) {
    double fps = (double)recentFrameCount / elapsedTime;
    DEBUGMSG( "Frame Rate: %g FPS\n", fps); 
    /* reset timing info so we compute FPS over last 2 seconds */
    recentStartTime = currentTime;
    recentFrameCount = 0;
  }
  return; 
}
  
//=========================================================
/*
 * When the movie player is operating in DMX slave mode, we'll take
 * our commands from the master instance of the player, rather than
 * the keyboard/mouse.
 */
int Slave::Loop(void)
{
  DEBUGMSG("SlaveLoop (thread %p), mMasterSocket state %d", QThread::currentThread(), mMasterSocket.state()); 
  int argc = 0;
  int32_t lastImageRendered = -1; 
  gCoreApp = new QApplication(argc, NULL); 
  bool idle = false; 
  RectanglePtr currentRegion; 
  qint32 destX = 0,  destY = 0,  lod = 0;
  float zoom;
  /* contact the master movie player */
  try {
    bool speedTest=false; 
    int32_t  playFrame = 0, playFirstFrame = 0, playLastFrame = 0; 
    int32_t playStep = 0;  // how much to advance the next frame by
    int32_t preload = 0; // number of frames to preload
	QString message, token;
    time_t lastheartbeat=time(NULL), now; 
    while (1) {
      now = time(NULL); 
      if (now - lastheartbeat > 300) {
        ERROR("It's been more than 5 minutes since the server checked in -- exiting."); 
        return 1; 
      }
      //      DEBUGMSG("About to process events. mMasterSocket state is %d", mMasterSocket.state()); 
      if (GetMasterMessage(message) ) {
        
        if (message == "disconnected") {
          ERROR("Error:  lost connection to the master server (state is %d).  Exiting.\n", mMasterSocket.state());
          break; 
        }
        
		QStringList messageList = message.split(" "); 
        token = messageList[0]; 
		QString dbgmsg = QString("Slave got message: \"") + message + "\""; 
        DEBUGMSG(dbgmsg); 
		if (token == "Heartbeat") {
		  //SendMessage("pong");
          lastheartbeat = time(NULL); 
        } // end "Heartbeat" 
        else if (token == "SlaveIdle")  {
          if (messageList.size() != 2) {
			SendError("Bad SlaveIdle message: "+message); 
			continue; 
		  }
          idle = (messageList[1] == "true"); 
          // if messageList[1] is "true" then we're idle.  If it's "false",
          // then we're a worker.
#ifdef USE_MPI
          MPI_Comm_split(MPI_COMM_WORLD, idle, 0, &workers);
#endif
        } // end "SlaveIdle"
        else if (token == "SpeedTest") {
          DEBUGMSG("slave got speedTest"); 
          speedTest = true;
          recentStartTime = GetCurrentTime();
          recentFrameCount = 0;
        }
        else if (token == "Exit") {
          ERROR("Slave received Exit signal from server.");
          return 0;           
        } // end "Exit"
        else if (!idle) {
          if (token == "Render")  {
            if (messageList.size() != 10) {
              SendError("Bad Render message: "+message); 
              continue; 
            }
            //Rectangle src;
            qint32 imageNum = messageList[1].toLong(); 
            qint32 region_x = messageList[2].toLong(); 
            qint32 region_y = messageList[3].toLong(); 
            qint32 region_width = messageList[4].toLong(); 
            qint32 region_height = messageList[5].toLong(); 
            if (!currentRegion || region_x != currentRegion->x ||
                region_y != currentRegion->y || 
                region_width != currentRegion->width ||
                region_height != currentRegion->height) {
              currentRegion.reset(new Rectangle(region_x, region_y, region_width, region_height)); 
            }
                
            destX = messageList[6].toLong();
            destY = messageList[7].toLong();
            lod = messageList[9].toLong();
            zoom = messageList[8].toFloat();
            if (mRenderer->mFrameList) {
              mRenderer->Render(imageNum, lastImageRendered,  
                                preload, playStep, 
                                playFirstFrame, playLastFrame, 
                                currentRegion, destX, destY, zoom, lod);
              
             
              lastImageRendered = imageNum; 
            }
            else {
              SendError("No frames to render\n");
              lastImageRendered = -1; 
            }
            DEBUGMSG(QString("Render %1 complete").arg(imageNum)); 
          }// end "Render"
          else if (token == "DrawString") {
            if (messageList.size() != 3) {
              SendError("Bad DrawString message: "+message); 
              continue; 
            }
            bool rowOK=false, colOK=false; 
            qint32 row = messageList[1].toLong(&rowOK), 
              col = messageList[2].toLong(&colOK);
            if (!rowOK or !colOK) {
              SendError(QString("Bad number in DrawString command")
                        + message); 
              continue; 
            }
            if (! mMasterSocket.bytesAvailable()) {
              INFO("Waiting for incoming data for DrawString"); 
              while (!mMasterSocket.bytesAvailable()) {
                usleep (1000); 
              }
            }
            (*mMasterStream) >> message; 
            DEBUGMSG((QString("string to draw is: ")+message)); 
            mRenderer->DrawString(row, col, message.toAscii());          
          }// "DrawString"
          else if (token == "SwapBuffers") {
            if (messageList.size() != 7) {
              SendError(QString("Bad SwapBuffers message: ")+message); 
              continue; 
            }
            bool ok = false; 
            qint32 currentFrame, swapID, 
              playDirection, preload, startFrame, endFrame; 
            int argnum = 1; 
            currentFrame = messageList[argnum++].toLong(&ok);
            if (ok) swapID = messageList[argnum++].toLong(&ok);
            if (ok) playDirection = messageList[argnum++].toLong(&ok); 
            if (ok) preload = messageList[argnum++].toLong(&ok); 
            if (ok) startFrame = messageList[argnum++].toLong(&ok); 
            if (ok) endFrame = messageList[argnum++].toLong(&ok); 
            if (!ok) {
              SendError(QString("Bad SetPlayDirection message: ")+message); 
              continue; 
            }
#ifdef USE_MPI
            DEBUGMSG("frame %d, mRenderer %d: MPI_Barrier", currentFrame, mRenderer); 
            /*!
              TOXIC:  disable MPI for right now during testing
            */ 
            //if (0 && useMPI) MPI_Barrier(workers); 
            if (mOptions->useMPI) MPI_Barrier(workers); 
#endif
            if (mRenderer && mRenderer->mFrameList) { 
              // only swap if there is a valid frame  
              DEBUGMSG("frame %d: mRenderer->SwapBuffers", currentFrame); 
              mRenderer->SwapBuffers();
              updateAndReportFPS(); 
            }
#ifdef USE_MPI
            DEBUGMSG("frame %d: Finished MPI_Barrier", currentFrame); 
#endif
            /* send ack */
            SendMessage(QString("SwapBuffers complete %1 %2").arg(messageList[1]).arg(messageList[2])); 
            /*  mRenderer->Preload(lastImageRendered, preload, playDirection, 
                startFrame, endFrame, &currentRegion, lod); 
            */
            /* if (preload && mRenderer && mRenderer->mFrameList) {
               int32_t i;
               for (i = 1; i <= preload; i++) {
               int offset = (playDirection == -1) ? -i : i;
               int frame = (lastImageRendered + offset);
               if (frame > endFrame) {
               frame = startFrame + (frame - endFrame);// preload for loops
               } 
               if (frame < startFrame) {
               frame = endFrame - (startFrame - frame); // for loops
               } 
               DEBUGMSG("Preload frame %d", frame); 
               mRenderer->Preload(frame, &currentRegion, lod);
               }
               
               }
            */ 
          } // end "SwapBuffers"
          else if (token == "Preload") {
            if (messageList.size() != 2) {
              SendError("Bad Preload message: "+message); 
              continue; 
            }
            bool ok = false; 
            preload = messageList[1].toLong(&ok); 
            if (!ok) {
              SendError(QString("Bad Preload argument ")+messageList[1]); 
              continue; 
            }
          }
          else if (token == "CreateRenderer") {
            if (messageList.size() != 8) {
              SendError("Bad CreateRenderer message: "+message); 
              continue; 
            }
            QString displayName = messageList[1];
            qint32 parentWin,  w, h, d, cacheFrames, numThreads;
            bool ok = false; 
            MovieEvent junkEvent; 
            parentWin = messageList[2].toLong(&ok); 
            if (ok) w = messageList[3].toLong(&ok); 
            if (ok) h = messageList[4].toLong(&ok);
            if (ok) d = messageList[5].toLong(&ok);
            if (ok) cacheFrames = messageList[6].toLong(&ok); 
            if (ok) numThreads = messageList[7].toLong(&ok);; 
            if (!ok) {
              SendError("Bad CreateRenderer argument in message: "+message); 
              continue; 
            }
            /* Create our canvas.  This canvas will not have any reader
             * threads associated with it, nor any frames in its
             * imageCache.
             */
            ProgramOptions *options=  GetGlobalOptions();
            options->mMaxCachedImages = cacheFrames;
            options->readerThreads = numThreads;
            options->fontName = DEFAULT_X_FONT;
            options->geometry.width = w;
            options->geometry.height = h;
            options->displayName = displayName;
            options->suggestedTitle = "Blockbuster Slave";
            mRenderer = Renderer::CreateRenderer(options, parentWin);
 
            mRenderer->GetXEvent(0, &junkEvent); 
             
            if (mRenderer == NULL) {
              ERROR("Could not create a canvas");
              SendError( "Error Could not create a canvas");
              return -1;
            }
          }// end "CreateRenderer"
          else if (token == "DestroyRenderer") {
            // this never did do anything that I remember.  
          } // end "DestroyRenderer"
          else if (token == "MoveResizeRenderer") {
            if (messageList.size() != 5) {
              SendError("Bad MoveResizeRenderer message: "+message); 
              continue; 
            }
            qint32 w, h, x, y;
            bool ok = false; 
            w = messageList[1].toLong(&ok);; 
            if (ok) h = messageList[2].toLong(&ok);; 
            if (ok) x = messageList[3].toLong(&ok);; 
            if (ok) y = messageList[4].toLong(&ok);; 
            if (!ok) {
              SendError("Bad MoveResize argument in message: "+message); 
              continue; 
            }
            
            if (!mRenderer) {
              SendError("ResizeRenderer requested, but canvas is not ready"); 
            } else {
              mRenderer->Resize(w, h, 0);
              mRenderer->Move(x, y, 0); // the zero means "force this" 
            }
          }// end "MoveResizeRenderer"
          else if (token == "SetFrameList") {
            /* This is the "file load" part of the code, rather misleading */
            /*we need to destroy image cache/reader threads etc before smBase destructor */
            mRenderer->DestroyImageCache();
            mRenderer->mFrameList.reset(); 

            message.remove(0, 13); //strip "SetFrameList " from front
            DEBUGMSG((QString("File list is: ")+message)); 
            if (!LoadFrames( message.toAscii()) || !mRenderer->mFrameList->numActualFrames()) {			
              SendError("No frames could be loaded."); 
            }
            playFirstFrame = 0; 
            playLastFrame = mRenderer->mFrameList->numStereoFrames()-1; 
            /*!
              Initialize frame rate computation
            */ 
            recentStartTime = GetCurrentTime(); 
            recentFrameCount = 0;
          }// end "SetFrameList"
          else {
            QString msg = QString("Bad message: ")+ message;
            SendError(msg);
            break;
          } // end (bad message)
        } // end if (!idle)
      }  /* end if bytesAvailable */
      else {
        /* no bytes available, be nice to the OS and sleep 2 milliseconds
           unless you are going 100 fps, this should not hurt frame rate...
        */
        gCoreApp->processEvents(QEventLoop::ExcludeUserInputEvents); 
        usleep(2*1000); 
      }

      /*!
        NEW CODE:  Behave like DisplayLoop, in that you do not need explicit master control of when to swap the next frame.  Hopefully, this eliminates delays inherent in that model.  
      */ 
      if (0 && playStep) {
        if (mRenderer && mRenderer->mFrameList) {
          mRenderer->Render(playFrame, lastImageRendered,  
                            preload, playStep, 
                            playFirstFrame, playLastFrame, currentRegion, 
                            destX, destY, zoom, lod);
        }
        lastImageRendered = playFrame; 
#ifdef USE_MPI
        if (mOptions->useMPI) { 
          DEBUGMSG("frame %d, %d:  MPI_Barrier", playFrame, mRenderer); 
          MPI_Barrier(workers); 
        }
#endif
        if (mRenderer && mRenderer->mFrameList) { 
          // only swap if there is a valid frame  
          DEBUGMSG( "frame %d: mRenderer->SwapBuffers\n", playFrame); 
          mRenderer->SwapBuffers();
        }
#ifdef USE_MPI
        DEBUGMSG( "frame %d: Finished MPI_Barrier\n", playFrame); 
#endif
        playFrame ++; 
        if (playFrame > playLastFrame) {
          playStep = 0; 
        }
      }
      
      /*!
        The below is purely debugging code.  It sets the slaves off and running, rendering as fast as they can and printing frame rates.  This is obviously good for getting baseline for performance, so don't delete this unless you are just sick of it and don't give a crap any longer. It ignores heartbeats and everything, so the master will assume the slaves have died and error out at some point. 
      */ 
      if (speedTest) {
        while (1) {
          
          /* render the next frame and advance the counter */
          if (mRenderer && mRenderer->mFrameList) {
            mRenderer->Render(playFrame, lastImageRendered,  
                              preload, playStep, 
                              playFirstFrame, playLastFrame, currentRegion, 
                              destX, destY, zoom, lod);
          }
          lastImageRendered = playFrame; 
          playFrame ++; 
          if (playFrame > playLastFrame) {
            playFrame = 0; 
          }
#ifdef USE_MPI
          if (mOptions->useMPI) { 
            DEBUGMSG("speedTest: frame %d, %d:  MPI_Barrier",playFrame, mRenderer); 
            MPI_Barrier(workers); 
          }
#endif
          if (mRenderer && mRenderer->mFrameList) { 
            // only swap if there is a valid frame  
            DEBUGMSG( "speedTest: frame %d: mRenderer->SwapBuffers\n",playFrame ); 
            mRenderer->SwapBuffers();
          }
#ifdef USE_MPI
          DEBUGMSG( "speedTest: frame %d: Finished MPI_Barrier\n",playFrame ); 
#endif
          /* send ack */
          // SendMessage(QString("SwapBuffers complete %1 %2").arg(messageList[1]).arg(messageList[2])); 
          updateAndReportFPS();         
        }
      } 
      /*! 
        End of debugging code for performance baseline
      */
    }/* end while loop */ 
  } catch (...) {
    SendError(QString("Exception in MessageLoop"));
  }
  
  SendMessage(QString("Slave exiting"));
  
  return 0;
}
