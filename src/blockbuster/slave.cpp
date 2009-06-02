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
#include "sm.h"
#include "cache.h"
#include <time.h>
#include "util.h"
#include <fstream>
#include "xwindow.h"


using namespace std; 


//=========================================================
/*
  Initializing the Slave: 
  Start the slave socket and get a display.  It's necessary to do this early
  because the master sends the DISPLAY and we want it to be set before
  GTK snarfs it up. 
*/
Slave::Slave(ProgramOptions *options):
  mOptions(options), mSocketFD(0), mCanvas(NULL) {
  SuppressMessageDialogs(); 
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
  if (mCanvas) {
    delete mCanvas;
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

bool Slave::LoadFrames(const char *files)
{
  DEBUGMSG("LoadFrames"); 
    FrameList *frames;
    int numFiles = 0;
    const char *start, *lastChar;

    QStringList fileList; 
    lastChar = files + strlen(files);

    start = files;
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

    /* debug */
    if (0) {
        int i;
        for (i = 0; i < numFiles; i++) {
          printf("File %d: %s\n", i, fileList[i].toStdString().c_str());
        }
    }

    if (numFiles == 1 &&
        fileList[0] == splashScreenFrameList.getFrame(0)->filename) {
      frames = &splashScreenFrameList;
    }
    else {
      frames = new FrameList; 
      frames->LoadFrames(fileList);
    }
    mCanvas->SetFrameList(mCanvas, frames);
	if (!mCanvas->frameList) {
	  return false;
	}
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
/*
 * When the movie player is operating in DMX slave mode, we'll take
 * our commands from the master instance of the player, rather than
 * the keyboard/mouse.
 */
int Slave::Loop(void)
{
  DEBUGMSG("SlaveLoop (thread %p), mMasterSocket state %d", QThread::currentThread(), mMasterSocket.state()); 
  int argc = 0;
  double fps = 0, recentEndTime, recentStartTime, elapsedTime;
  long recentFrameCount = 0; 
  int32_t lastImageRendered = -1; 
  gCoreApp = new QApplication(argc, NULL); 
  UserInterface *userInterface = mOptions->userInterface;
  bool idle = false; 
  Rectangle currentRegion; 
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
        exit(1); 
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
          exit(0);           
        } // end "Exit"
        else if (!idle) {
          if (token == "Render")  {
            if (messageList.size() != 10) {
              SendError("Bad Render message: "+message); 
              continue; 
            }
            //Rectangle src;
            qint32 imageNum = messageList[1].toLong(); 
            currentRegion.x = messageList[2].toLong(); 
            currentRegion.y = messageList[3].toLong(); 
            currentRegion.width = messageList[4].toLong(); 
            currentRegion.height = messageList[5].toLong(); 
            destX = messageList[6].toLong();
            destY = messageList[7].toLong();
            lod = messageList[9].toLong();
            zoom = messageList[8].toFloat();
            
            if (mCanvas->frameList) {
              mCanvas->Render(mCanvas, imageNum, &currentRegion, 
                             destX, destY, zoom, lod);
              lastImageRendered = imageNum; 
            }
            else {
              SendError("No frames to render\n");
              lastImageRendered = -1; 
            }
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
            mCanvas->DrawString(mCanvas, row, col, message.toAscii());          
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
            DEBUGMSG("frame %d, mCanvas %d: MPI_Barrier", lastImageRendered, mCanvas); 
            /*!
              TOXIC:  disable MPI for right now during testing
            */ 
            //if (0 && useMPI) MPI_Barrier(workers); 
            if (mOptions->useMPI) MPI_Barrier(workers); 
#endif
            if (mCanvas && mCanvas->frameList) { 
              // only swap if there is a valid frame  
              DEBUGMSG("frame %d: mCanvas->SwapBuffers", lastImageRendered); 
              mCanvas->SwapBuffers(mCanvas);
            }
#ifdef USE_MPI
            DEBUGMSG("frame %d: Finished MPI_Barrier", lastImageRendered); 
#endif
            /* send ack */
            SendMessage(QString("SwapBuffers complete %1 %2").arg(messageList[1]).arg(messageList[2])); 
            if (preload && mCanvas && mCanvas->frameList) {
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
                mCanvas->Preload(mCanvas, frame, &currentRegion, lod);
              }
               
            } 
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
          /*  else if (token == "Preload") {
              if (messageList.size() != 7) {
              SendError("Bad Preload message: "+message); 
              continue; 
              }
              if (mCanvas->frameList) {			   
              //Rectangle region;
              qint32 frame;
              bool ok = false; 
              frame=messageList[1].toLong(&ok);
              if (ok) currentRegion.x=messageList[2].toLong(&ok) ;
              if (ok) currentRegion.y=messageList[3].toLong(&ok);
              if (ok) currentRegion.width=messageList[4].toLong(&ok);
              if (ok) currentRegion.height=messageList[5].toLong(&ok);
              if (ok) lod=messageList[6].toLong(&ok); 
              if (!ok) {
              SendError("Bad Preload argument in message: "+message); 
              continue; 
              }
              mCanvas->Preload(mCanvas, frame, &currentRegion, lod);
              }

              }// end "Preload"
          */ 
          else if (token == "CreateCanvas") {
            if (messageList.size() != 8) {
              SendError("Bad CreateCanvas message: "+message); 
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
              SendError("Bad CreateCanvas argument in message: "+message); 
              continue; 
            }
            /* Create our canvas.  This canvas will not have any reader
             * threads associated with it, nor any frames in its
             * imageCache.
             */
            ProgramOptions *createOptions= new ProgramOptions;
            createOptions->frameCacheSize = cacheFrames;
            createOptions->readerThreads = numThreads;
            createOptions->fontName = DEFAULT_X_FONT;
            createOptions->geometry.width = w;
            createOptions->geometry.height = h;
            createOptions->displayName = displayName;
            createOptions->suggestedTitle = "Blockbuster Slave";
            mCanvas = new Canvas(userInterface, mOptions->rendererIndex, 
                                createOptions, parentWin);
            GetXEvent(mCanvas, 0, &junkEvent); 
            //canvas->GetEvent(mCanvas, 0, &junkEvent); 
            delete createOptions; 
            
            if (mCanvas == NULL) {
              ERROR("Could not create a canvas");
              SendError( "Error Could not create a canvas");
              return -1;
            }
         }// end "CreateCanvas"
          else if (token == "DestroyCanvas") {
            // this never did do anything that I remember.  
          } // end "DestroyCanvas"
          else if (token == "MoveResizeCanvas") {
            if (messageList.size() != 5) {
              SendError("Bad MoveResizeCanvas message: "+message); 
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
            
            if (!mCanvas || !mCanvas->Resize) {
              SendError("ResizeCanvas requested, but canvas is not ready"); 
            } else {
              mCanvas->Resize(mCanvas, w, h, 0);
              mCanvas->Move(mCanvas, x, y, 0); // the zero means "force this" 
            }
          }// end "MoveResizeCanvas"
          else if (token == "SetFrameList") {
            /* This is the "file load" part of the code, rather misleading */
            /*we need to destroy image cache/reader threads etc before smBase destructor */
            DestroyImageCache(mCanvas);
            if (mCanvas->frameList) {
              mCanvas->frameList->DeleteFrames(); 
              delete mCanvas->frameList; 
            }
           message.remove(0, 13); //strip "SetFrameList " from front
           DEBUGMSG((QString("File list is: ")+message)); 
           if (!LoadFrames( message.toAscii())) {			
              SendError("No frames could be loaded."); 
            }
            playFirstFrame = 0; 
            playLastFrame = mCanvas->frameList->numStereoFrames()-1; 
          }// end "SetFrameList"
          /* else if (token == "PlayForward") {
             playStep = 1; 
             }
          */ 
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
        if (mCanvas && mCanvas->frameList) {
          mCanvas->Render(mCanvas, playFrame, &currentRegion, 
                         destX, destY, zoom, lod);
        }
        lastImageRendered = playFrame; 
        playFrame ++; 
        if (playFrame > playLastFrame) {
          playStep = 0; 
        }
#ifdef USE_MPI
        if (mOptions->useMPI) { 
          DEBUGMSG("frame %d, %d:  MPI_Barrier",lastImageRendered, mCanvas); 
          MPI_Barrier(workers); 
        }
#endif
        if (mCanvas && mCanvas->frameList) { 
          // only swap if there is a valid frame  
          DEBUGMSG( "frame %d: mCanvas->SwapBuffers\n", lastImageRendered); 
          mCanvas->SwapBuffers(mCanvas);
        }
#ifdef USE_MPI
        DEBUGMSG( "frame %d: Finished MPI_Barrier\n", lastImageRendered); 
#endif
      }
      
      /*!
        The below is purely debugging code.  It sets the slaves off and running, rendering as fast as they can and printing frame rates.  This is obviously good for getting baseline for performance, so don't delete this unless you are just sick of it and don't give a crap any longer. It ignores heartbeats and everything, so the master will assume the slaves have died and error out at some point. 
      */ 
      if (speedTest) {
        while (1) {
          
          /* render the next frame and advance the counter */
          if (mCanvas && mCanvas->frameList) {
            mCanvas->Render(mCanvas, playFrame, &currentRegion, 
                           destX, destY, zoom, lod);
          }
          lastImageRendered = playFrame; 
          playFrame ++; 
          if (playFrame > playLastFrame) {
            playFrame = 0; 
          }
#ifdef USE_MPI
          if (mOptions->useMPI) { 
            DEBUGMSG("speedTest: frame %d, %d:  MPI_Barrier",lastImageRendered, mCanvas); 
            MPI_Barrier(workers); 
          }
#endif
          if (mCanvas && mCanvas->frameList) { 
            // only swap if there is a valid frame  
            DEBUGMSG( "speedTest: frame %d: mCanvas->SwapBuffers\n", lastImageRendered); 
            mCanvas->SwapBuffers(mCanvas);
          }
#ifdef USE_MPI
          DEBUGMSG( "speedTest: frame %d: Finished MPI_Barrier\n", lastImageRendered); 
#endif
          /* send ack */
          // SendMessage(QString("SwapBuffers complete %1 %2").arg(messageList[1]).arg(messageList[2])); 
          /*! 
            Compute frames per second 
          */ 
          recentFrameCount++;
          recentEndTime = GetCurrentTime();
          elapsedTime = recentEndTime - recentStartTime;
          if (elapsedTime >= 1.0) {
            fps = (double) recentFrameCount / elapsedTime;
            DEBUGMSG( "speedTest: Frame Rate: %g\n", fps); 
            /* reset timing info so we compute FPS over last 2 seconds */
            recentStartTime = GetCurrentTime();
            recentFrameCount = 0;
          }
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
