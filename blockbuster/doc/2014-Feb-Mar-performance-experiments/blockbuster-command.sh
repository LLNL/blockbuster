#!/usr/bin/env bash
# small tiles:
blockbuster -dev -cachedebug -framerate 100 -repeat 2 -v 5 -w -traceEvents -play -display :0 -noscreensaver -fullscreen -log experiment.log /usr/dvstmp/Movies/2008-05-09-Kerman-Earthquake-notiles.sm >output.txt 2>&1

No debug info, just frame rates:
blockbuster -dev -cachedebug -framerate 100 -repeat 2  -play -display :0 -noscreensaver -fullscreen /usr/dvstmp/Movies/2008-05-09-Kerman-Earthquake-notiles.sm

no threads: 
blockbuster -dev -cachedebug -framerate 100 -repeat 2 -v 6 -w -traceEvents -play -display :0 -noscreensaver -fullscreen -log experiment.log -threads 0 /usr/dvstmp/Movies/2008-05-09-Kerman-Earthquake.sm >output.txt 2>&1

# textures
blockbuster -dev -renderer gltexture -cachedebug -framerate 100 -repeat 2 -v 5 -w -traceEvents -play -display :0 -noscreensaver -fullscreen -log experiment.log /usr/dvstmp/Movies/2008-05-09-Kerman-Earthquake.sm >output.txt 2>&1


Looks like glDrawPixels is 15ms: 

BLOCKBUSTER (rzthriller9): INFO: glRenderer::RenderActual begin, frame 567, 2867 x 2047  at 0, 0  zoom=0.987793  lod=0 [glRenderer.cpp:RenderActual() line 126, thread 30, time=7900.173]
BLOCKBUSTER (rzthriller9): INFO: Pull the image from our cache  [glRenderer.cpp:RenderActual() line 179, thread 30, time=7900.173]
BLOCKBUSTER (rzthriller9): DEBUG: ImageCache::GetImage frame 567 [cache.cpp:GetImage() line 641, thread 30, time=7900.173]
BLOCKBUSTER (rzthriller9): DEBUG: Preloaded 40 frames from 567 to 606 stepping by 1 (max is 40) [cache.cpp:GetImage() line 661, thread 30, time=7900.174]
BLOCKBUSTER (rzthriller9): DEBUG: Returning found image 567 [cache.cpp:GetImage() line 714, thread 30, time=7900.174]
BLOCKBUSTER (rzthriller9): INFO: Got image [glRenderer.cpp:RenderActual() line 181, thread 30, time=7900.174]
BLOCKBUSTER (rzthriller9): INFO: done with glViewport [glRenderer.cpp:RenderActual() line 196, thread 30, time=7900.174]
BLOCKBUSTER (rzthriller9): INFO: Done with glClearColor and glClear.  Frame 567 row order is 2 [glRenderer.cpp:RenderActual() line 207, thread 30, time=7900.174]
BLOCKBUSTER (rzthriller9): INFO: Done with glPixelZoom. Region 0 0 2867 2047 : LodScale 1 : Zoom 0.987793 [glRenderer.cpp:RenderActual() line 230, thread 30, time=7900.174]
BLOCKBUSTER (rzthriller9): INFO: glPixelStorei(GL_UNPACK_ROW_LENGTH, 2867) [glRenderer.cpp:RenderActual() line 240, thread 30, time=7900.174]
BLOCKBUSTER (rzthriller9): INFO: glPixelStorei(GL_UNPACK_SKIP_ROWS,  1) [glRenderer.cpp:RenderActual() line 241, thread 30, time=7900.174]
BLOCKBUSTER (rzthriller9): INFO: glPixelStorei(GL_UNPACK_SKIP_PIXELS,  0) [glRenderer.cpp:RenderActual() line 242, thread 30, time=7900.174]
BLOCKBUSTER (rzthriller9): INFO: glPixelStorei(GL_UNPACK_ALIGNMENT,  1) [glRenderer.cpp:RenderActual() line 243, thread 30, time=7900.174]
BLOCKBUSTER (rzthriller9): INFO: Buffer for frame 567 is 2867w x 2048h, region is 2867w x 2047h, destX = 1143, destY = 0 [glRenderer.cpp:RenderActual() line 245, thread 30, time=7900.174]
BLOCKBUSTER (rzthriller9): INFO: Done with glDrawPixels [glRenderer.cpp:RenderActual() line 256, thread 30, time=7900.188]
BLOCKBUSTER (rzthriller9): INFO: Done with glBitmap [glRenderer.cpp:RenderActual() line 261, thread 30, time=7900.188]
BLOCKBUSTER (rzthriller9): INFO: glRenderer::RenderActual end [glRenderer.cpp:RenderActual() line 269, thread 30, time=7900.188]
BLOCKBUSTER (rzthriller9): DEBUG: Renderer::ReportFrameChange 567 [Renderer.cpp:ReportFrameChange() line 658, thread 30, time=7900.196]
BLOCKBUSTER (rzthriller9): DEBUG: frameNumber changed  to 568 after switch [movie.cpp:DisplayLoop() line 860, thread 30, time=7900.196]
<t=7900.197> movie.cpp:DisplayLoop, 987():  Sending snapshot mSnapshotType=MOVIE_NONE mFilename=/usr/dvstmp/Movies/2008-05-09-Kerman-Earthquake.sm mFrameRate=42.516117 mTargetFPS=100.000000 mZoom=0.987793 mLOD=0 mStereo=0 mPlayStep=1 mStartFrame=0 mEndFrame=1949 mNumFrames=1950 mFrameNumber=568 mLoop=1 mPingPong=0 mFullScreen=1 mZoomToFill=1 mNoScreensaver=1 mScreenHeight=2023 mScreenWidth=5120 mScreenXpos=0 mScreenYpos=0 mImageHeight=2048 mImageWidth=2867 mImageXpos=0 mImageYpos=0



