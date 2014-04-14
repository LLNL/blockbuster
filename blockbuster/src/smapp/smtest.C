/*
** $RCSfile: smtest.C,v $
** $Name:  $
**
** ASCI Visualization Project 
**
** Lawrence Livermore National Laboratory
** Information Management and Graphics Group
** P.O. Box 808, Mail Stop L-561
** Livermore, CA 94551-0808
**
** For information about this project see:
** 	http://www.llnl.gov/sccd/lc/img/
**
**      or contact: asciviz@llnl.gov
**
** For copyright and disclaimer information see:
**      $(ASCIVIS_ROOT)/copyright_notice_1.txt
**
** 	or man llnl_copyright
**
** $Id: smtest.C,v 1.2 2008/03/21 18:10:23 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "sm/smBase.h"
#include <stdint.h>
#include <vector>
#include "../RC_cpp_lib/timer.h"
#include "../config/version.h"

struct UserPrefs {
  UserPrefs(): nthreads(1), lod(-1), nloops(1), 
               range(2,-1), pos(2,0), dim(2, 0), step(2, 1),
               fltpos(2, 0), fltdim(2, 0), fltstep(2, 0), 
               dpos(2, 0), ddim(2, 0), dstep(2, 0), 
               haveRect(false), pan(false), sm(NULL) {
    return; 
  }
  int nthreads;
  int lod; 
  int nloops;
  vector<int> range, pos, dim, step;
  vector<float>fltpos, fltdim, fltstep, 
    dpos, ddim, dstep; 
  bool haveRect, pan;
  smBase *sm;
}; 

struct ThreadData {
  ThreadData(): threadNum(0), numFramesRead(0),  bytesRead(0),
                currentFrame(0), done(false), err(false), 
                min(1e+8), max(-1e+8) {
    return; 
  }
  UserPrefs userPrefs; 
  int lod; 
  uint32_t threadNum;
  uint32_t numFramesRead, currentFrame; 
  uint64_t bytesRead; 
  vector<int> range;
  bool done, err; 
  float min, max; 
};


void calcrectpan(UserPrefs *prefs, int x,int y,int *p,int *d,int *s )
{
  uint32_t width = prefs->sm->getWidth(), 
    height = prefs->sm->getWidth(); 
  // move the frame 
  p[0] = prefs->pos[0] + (int)(x*prefs->dpos[0]);
  p[1] = prefs->pos[1] + (int)(y*prefs->dpos[1]);
  d[0] = prefs->dim[0];
  d[1] = prefs->dim[1];
  s[0] = prefs->step[0];
  s[1] = prefs->step[1];
  
  // make it valid...

  // adjust scales...
  if (s[0] < 1) s[0] = 1;
  if (s[1] < 1) s[1] = 1;
  if (d[0] < 1) d[0] = 1;
  if (d[1] < 1) d[1] = 1;
  if (d[0] > width) d[0] = width;
  if (d[1] > height) d[1] = height;
  if (s[0]*d[0] > width) s[0] = width/d[0];
  if (s[1]*d[1] > height) s[1] = height/d[1];

  // make the pos valid...
  if (p[0] < 0) p[0] -= 1;
  if (p[1] < 0) p[1] -= 1;
  p[0] = p[0] % width;
  p[1] = p[1] % height;
  if ((p[0] + d[0]*s[0]) > width) p[0] = width-(d[0]*s[0]);
  if ((p[1] + d[1]*s[1]) > height) p[1] = height-(d[1]*s[1]);

  return;
}

void calcrect(UserPrefs *prefs, int n,int *p,int *d,int *s)
{
  float f = (float)n;
  uint32_t width = prefs->sm->getWidth(), 
    height = prefs->sm->getWidth(); 

  // move the frame 
  p[0] = prefs->pos[0] + (int)(f*prefs->dpos[0]);
  p[1] = prefs->pos[1] + (int)(f*prefs->dpos[1]);
  d[0] = prefs->dim[0] + (int)(f*prefs->ddim[0]);
  d[1] = prefs->dim[1] + (int)(f*prefs->ddim[1]);
  s[0] = prefs->step[0] + (int)(f*prefs->dstep[0]);
  s[1] = prefs->step[1] + (int)(f*prefs->dstep[1]);


  // adjust scales...
  if (s[0] < 1) s[0] = 1;
  if (s[1] < 1) s[1] = 1;
  if (d[0] < 1) d[0] = 1;
  if (d[1] < 1) d[1] = 1;
  if (d[0] > width) d[0] = width;
  if (d[1] > height) d[1] = height;
  if (s[0]*d[0] > width) s[0] = width/d[0];
  if (s[1]*d[1] > height) s[1] = height/d[1];

  // make the pos valid...
  if (p[0] < 0) p[0] -= 1;
  if (p[1] < 0) p[1] -= 1;
  p[0] = p[0] % width;
  p[1] = p[1] % height;
  if ((p[0] + d[0]*s[0]) > width) p[0] = width-(d[0]*s[0]);
  if ((p[1] + d[1]*s[1]) > height) p[1] = height-(d[1]*s[1]);

  return;
}

double get_clock(void)
{
  struct timeval t;

  gettimeofday(&t,NULL);
  return((double)t.tv_sec + (double)t.tv_usec*1E-6);
}
void *readThread(void *data)
{
  u_char *frame;
  ThreadData *threadData = (ThreadData*)data; 
  uint32_t mynum = threadData->threadNum;
  int32_t loopNum, f;
  smBase *sm = threadData->userPrefs.sm;
  UserPrefs *prefs = &threadData->userPrefs; 
  
  frame = (u_char *)malloc(sizeof(u_char[4]) * sm->getWidth() *
                           sm->getHeight());
  int rowStride = 0;
  for(loopNum=0; loopNum<prefs->nloops; loopNum++) {    
    for (f=threadData->range[0]; f<=threadData->range[1]; f++) {
      if ((f % prefs->nthreads) == mynum) {
        double t0;
        if (!prefs->haveRect) {
          t0 = get_clock();
          uint32_t bytesread = sm->getFrame(f, frame, mynum, threadData->lod);
          if (bytesread == 0) {
              threadData->err = true;
              return NULL; 
            }
          threadData->bytesRead += bytesread;
        } else {
          if(prefs->pan == 0) {
            int p[2],d[2],s[2];
            calcrect(prefs, f+loopNum*sm->getNumResolutions()*sm->getNumFrames(),
                     p,d,s);
            t0 = get_clock();
            smdbprintf(0," d = [%d,%d], p = [%d,%d], s = [%d,%d]\n",d[0],d[1],p[0],p[1],s[0],s[1]);
            int32_t bytesread = sm->getFrameBlock(f, frame, mynum, rowStride, d, p, s, threadData->lod);
            if (bytesread == 0) {
              threadData->err = true;             
              return NULL; 
            }
            threadData->bytesRead += bytesread;
          }
          else {
            int p[2],d[2],s[2];
            int stepsX = (int)floor( (double)sm->getWidth()/prefs->dpos[0]);
            int stepsY = (int)floor((double)sm->getHeight()/prefs->dpos[1]);
            if(stepsX < 1) stepsX = 1;
            if(stepsY < 1) stepsY = 1;
            smdbprintf(0,"Position Steps[%d,%d]\n",stepsX,stepsY);
            t0 = get_clock();
            for(int yStep = 0; yStep < stepsY; yStep++) {
              for(int xStep = 0; xStep < stepsX; xStep++) {
                calcrectpan(prefs, xStep,yStep,p,d,s);
                smdbprintf(0," d = [%d,%d], p = [%d,%d], s = [%d,%d]\n",d[0],d[1],p[0],p[1],s[0],s[1]);
                int32_t bytesread = sm->getFrameBlock(f, frame, mynum, rowStride, d, p, s, threadData->lod);
                if (bytesread == 0) {
                  threadData->err = true;             
                  return NULL; 
                }
                threadData->bytesRead += bytesread; 
              }
            }
          }
        }
        t0 = get_clock() - t0;
        if (t0 < threadData->min) threadData->min = t0;
        if (t0 > threadData->max) threadData->max = t0;
        threadData->numFramesRead++; 
        threadData->currentFrame = f; 
         
        smdbprintf(0, "Thread %d got frame %d\r", mynum, f);         
         
      }
    }
  }
  threadData->done = true; 
  return NULL;
}

void usage(char *prg)
{
  smdbprintf(0, "%s (%s) usage: %s [options] smfilename\n",__DATE__,prg);
  smdbprintf(0,"Options:\n");
  smdbprintf(0, "\t -h or -help:  display this menu\n"); 
  smdbprintf(0,"\t-lod <num|'all'> Level of detail.  default: 'all'\n");
   
  smdbprintf(0, "\t -range first last:  first and last frames to run over.  (0-based indexes, inclusive interval)\n"); 
  smdbprintf(0,"\t-nt <num>   Select the number of threads/windows.  Default: 1\n");
  smdbprintf(0,"\t-rect <xpos ypos xsize ysize xinc yinc>  Rect/step to sample.  Rect values are float.  Either pixels or window fractions between 0.01 and 0.99 are acceptable.  Default: whole\n");
  smdbprintf(0,"\t-pan <xpos ypos xsize ysize xinc yinc> Pan across using giiven rect before advancing to next frame.  Rect values are floats.  Either pixels or window fractions between 0.01 and 0.99 are acceptable.  Default: no panning.\n");
  smdbprintf(0,"\t-loops <n>  Number of loops to run. Default: 1\n");
  smdbprintf(0, "\t-v n Set verbosity to level n (0-5)\n"); 
  return;
}

void ConvertFraction(float &fraction, float refValue) {
  if (0.0 < fraction && fraction <= 1.0001) fraction *= refValue; 
  return; 
}

void ConvertFractions(const char *name, uint32_t width, uint32_t height, vector<float>&pos, vector<float>&dim, vector<float>&step) {
  if (0.0 < pos[0]  && pos[0]  <= 1.001) 
    pos[0]  = pos[0]*width-1;

  if (0.0 < pos[1]  && pos[1]  <= 1.001) 
    pos[1]  = pos[1]*height-1; 

  if (0.0 < dim[0]  && dim[0]  <= 1.001) 
    dim[0]  = dim[0]*width-1;

  if (0.0 < dim[1]  && dim[1]  <= 1.001) 
    dim[1]  = dim[1]*height-1; 

  if (0.0 < step[0] && step[0] <  0.990) 
    step[0] = step[0]*width-1;

  if (0.0 < step[1] && step[1] <  0.990) 
    step[1] = step[1]*height-1;

  smdbprintf(0, "After converting from fraction, %s is {%f, %f, %f, %f, %f, %f}\n", name, pos[0], pos[1], dim[0], dim[1], step[0], step[1]); 
  return; 
} 

int main(int argc, char *argv[])
{
  
  UserPrefs prefs;   
  int numerrs = 0; 
  int arg, f;
  double t0, t1;
  vector<pthread_t> threads;
  float *retval;
      
  for (arg=1; arg<argc && argv[arg][0]=='-'; arg++) {
    if (strncmp(argv[arg], "-h", 2) == 0) {
      usage(argv[0]); 
      exit(0); 
    } else if (strcmp(argv[arg], "-lod") == 0) {
      if (arg+1 >= argc) usage(argv[0]);
      if (string(argv[arg+1]) == "all") {
        prefs.lod = -1;
      } else {
        prefs.lod = atoi(argv[arg+1]);
      }
      arg++;
    } else if (strcmp(argv[arg], "-nt") == 0) {
      if (arg+1 >= argc) usage(argv[0]);
      prefs.nthreads=atoi(argv[arg+1]);
      arg++;
    } else if (strcmp(argv[arg], "-loops") == 0) {
      if (arg+1 >= argc) usage(argv[0]);
      prefs.nloops=atoi(argv[arg+1]);
      arg++;
    } else if (strcmp(argv[arg], "-pan") == 0) {
      smdbprintf(0,"Panning selected\n");
      if (arg+6 >= argc) usage(argv[0]);
      prefs.dpos[0] = atof(argv[++arg]);
      prefs.dpos[1] = atof(argv[++arg]);
      prefs.ddim[0] = atof(argv[++arg]);
      prefs.ddim[1] = atof(argv[++arg]);
      prefs.dstep[0] = atof(argv[++arg]);
      prefs.dstep[1] = atof(argv[++arg]);
    } else if (strcmp(argv[arg],"-rect") == 0) {
      if (arg+6 >= argc) usage(argv[0]);
      prefs.fltpos[0] = atof(argv[++arg]);
      prefs.fltpos[1] = atof(argv[++arg]);
      prefs.fltdim[0] = atof(argv[++arg]);
      prefs.fltdim[1] = atof(argv[++arg]);
      prefs.fltstep[0] = atof(argv[++arg]);
      prefs.fltstep[1] = atof(argv[++arg]);
      prefs.haveRect = 1;
    } else if (strcmp(argv[arg], "-range") == 0) {
      if (arg+2 >= argc) usage(argv[0]);
      prefs.range[0] = atoi(argv[++arg]);
      prefs.range[1] = atoi(argv[++arg]);    
    } else if (strcmp(argv[arg],"-v") == 0) {
      sm_setVerbose(atoi(argv[++arg])); 
    }
    else {
      smdbprintf(0,"\n******************\nUnknown arg: %s\n******************\n",argv[arg]);
      usage(argv[0]); 
      exit(1);
    }
  }
  if (argc < 2) {
    smdbprintf(0, "Not enough arguments.  Require a filename.\n"); 
    usage(argv[0]); 
    exit(3); 
  }
  for (; arg < argc; arg++) {
    string moviename = argv[arg]; 
    smdbprintf(0, "\n\nTesting movie: %s\n", moviename.c_str());
    prefs.sm = smBase::openFile(argv[arg], O_RDONLY, prefs.nthreads);
     
    if (!prefs.sm) {
      smdbprintf(0, "Unable to open movie: %s\n",moviename.c_str());
      exit(4);
    }
    smdbprintf(3, "movie is %dx%d\n", prefs.sm->getWidth(), prefs.sm->getHeight()); 
    ConvertFractions("pan rect", prefs.sm->getWidth(), prefs.sm->getHeight(), prefs.dpos, prefs.ddim, prefs.dstep); 
     
    threads.resize(prefs.nthreads); 

    // Default values
    if (!prefs.haveRect) {
      prefs.pos[0] = 0;
      prefs.pos[1] = 0;
      prefs.dim[0] = prefs.sm->getWidth();
      prefs.dim[1] = prefs.sm->getHeight();
      prefs.step[0] = 1;
      prefs.step[1] = 1;
    } else {
      ConvertFractions("crop rect", prefs.sm->getWidth(), prefs.sm->getHeight(), prefs.fltpos,prefs.fltdim,prefs.fltstep); 
      prefs.pos[0] = (int)(prefs.fltpos[0]+0.5); 
      prefs.pos[1] = (int)(prefs.fltpos[1]+0.5); 
      prefs.dim[0] = (int)(prefs.fltdim[0]+0.5); 
      prefs.dim[1] = (int)(prefs.fltdim[1]+0.5); 
      prefs.step[0] = (int)(prefs.fltstep[0]+0.5); 
      prefs.step[1] = (int)(prefs.fltstep[1]+0.5); 
      smdbprintf(0, "After rounding, crop rectangle is {%d, %d, %d, %d, %d, %d}\n", prefs.pos[0], prefs.pos[1], prefs.dim[0], prefs.dim[1], prefs.step[0], prefs.step[1]); 
    }

    // Double check
    int errnum = 0;
    if ((prefs.pos[0]<0) || (prefs.pos[0]>=prefs.sm->getWidth())) errnum = 1;
    if ((prefs.pos[1]<0) || (prefs.pos[1]>=prefs.sm->getWidth())) errnum = 2;
    if ((prefs.step[0]<0) || (prefs.step[1]<0)) errnum = 3;
    if ((prefs.dim[0]<1) || (prefs.dim[0]+prefs.pos[0]*prefs.step[0]>prefs.sm->getWidth())) {
      smdbprintf(0, "LH = %d, RH = %d\n", prefs.dim[0]+prefs.pos[0]*prefs.step[0], prefs.sm->getWidth()); 
      errnum = 4;
    }
    if ((prefs.dim[1]<1) || (prefs.dim[1]+prefs.pos[1]*prefs.step[1]>prefs.sm->getHeight())) errnum = 5;
   
    if (errnum) {
      smdbprintf(0,"Error: Invalid frame rectangle : Case %d\n", errnum);
      exit(5);
    }

    int endLOD = prefs.lod, lod = prefs.lod; 
    if (prefs.lod == -1) {
      lod = 0; 
      endLOD = prefs.sm->getNumResolutions()-1; 
    }
    if (lod >prefs.sm->getNumResolutions()-1) { 
      smdbprintf(0, "Error:  lod %d specified, but movie has only %d\n", 
                 lod, prefs.sm->getNumResolutions()); 
      exit(6); 
    }
    
    vector<int> range = prefs.range; 
    if (range[0] == -1) {
      range[0] = 0; 
    } 
    if (range[1] == -1 || range[1] > prefs.sm->getNumFrames()-1) {
      range[1] = prefs.sm->getNumFrames()-1; 
    }    
    
    printf("Frame range: %d to %d\n", range[0], range[1]); 
    printf("threads: %d\n",prefs.nthreads);
    printf("frame size: %d,%d\n", prefs.sm->getWidth(), prefs.sm->getHeight());
    printf("rect: %d,%d @ %d,%d step %d,%d\n",
           prefs.dim[0],prefs.dim[1],prefs.pos[0],prefs.pos[1],
           prefs.step[0],prefs.step[1]);

    for (; lod <= endLOD; lod++) {
      
      float min = 1e+8, max = -1e+8;
      smdbprintf(0, "Starting LOD %d\n", lod); 
      // frame range is 1-based

      t0 = get_clock();


      // computer estimated movie size to give user something to do while we work
      float windowFraction = 
        ((float)prefs.dim[0]/prefs.sm->getWidth()) *
        ((float)prefs.dim[1]/prefs.sm->getHeight());    
      int totalFrames = (range[1] - range[0]+1) * prefs.nloops; 
      float totalMegabytes = 0; 
      uint32_t cur_size = 0; 
      for (f=range[0]; f<=range[1]; f++) {
        cur_size =      prefs.sm->getCompFrameSize(f, 0); 
        totalMegabytes += cur_size;
        smdbprintf(5, "size of frame %d is %d\n", f, cur_size); 

      }
      totalMegabytes /= (1000.0*1000.0); 
      float mbPerFrameEst = totalMegabytes/(range[1]-range[0]+1)*windowFraction; 
      totalMegabytes *= prefs.nloops; 
      smdbprintf(0, "Total size to read in %d frames, based on inputs: %f total MB, %f adjusted for given window fraction.  average MB/frame = %f\n", 
                 totalFrames, totalMegabytes, windowFraction*totalMegabytes, mbPerFrameEst); 
      vector<ThreadData> threadData(prefs.nthreads); 
      for(f=0; f<prefs.nthreads; f++) { 
        threadData[f].range = range; 
        threadData[f].threadNum = f; 
        threadData[f].lod = lod; 
        threadData[f].userPrefs = prefs;
        pthread_t *tp = &threads[f]; 
        void *vp = (void*)(&threadData[f]);
        pthread_create(tp, NULL, readThread, vp);
      }
      bool done = false, err = false; 
      double previousTime = timer::GetExactSeconds(), totalTime = 0; 
      double elapsed, fps, megabytes, totalMB = 0, nframes; 
      vector<uint32_t> previousFrames(prefs.nthreads+1, 0), 
        numFrames(prefs.nthreads+1, 0); 
      vector<uint64_t> previousBytes(prefs.nthreads+1, 0);
      while (!done && !err) {
        previousFrames[prefs.nthreads] = numFrames[prefs.nthreads]; 
        numFrames[prefs.nthreads] = 0; 
        elapsed = timer::GetExactSeconds() - previousTime; 
        totalTime += elapsed; 
        previousTime = timer::GetExactSeconds(); 
        totalMB = 0; 
        int currentFrame = 0; 
        for(f=0; f<prefs.nthreads; f++) {
          previousFrames[f] = numFrames[f]; 
          done = done || threadData[f].done; 
          err = err || threadData[f].err; 
          numFrames[f] = threadData[f].numFramesRead; 
          numFrames[prefs.nthreads] += numFrames[f]; 
          nframes = numFrames[f]-previousFrames[f];
          fps = nframes / elapsed; 
          megabytes = 
            (threadData[f].bytesRead - previousBytes[f])/(1000*1000); 
          totalMB += megabytes; 
          previousBytes[f] = threadData[f].bytesRead; 
          smdbprintf(0, "Thread %02d: lod %d, frame %05d, %03.2f frames in %f secs =  %05.3f fps, actual MB/sec %f\n", f, lod, threadData[f].currentFrame, nframes, elapsed, fps, megabytes/elapsed); 
          if (threadData[f].currentFrame > currentFrame) {
            currentFrame = threadData[f].currentFrame; 
          }
        }
        fps = (numFrames[prefs.nthreads]-previousFrames[prefs.nthreads])/elapsed; 
        float pct = (numFrames[prefs.nthreads])/totalFrames*100.0;
        smdbprintf(0, "ALL THREADS: t = %05.3f:  lod %d, frame %05d, %4.2f%% complete, fps = %5.3f, actual MB/sec %f\n", totalTime, lod, currentFrame, pct, fps, totalMB/elapsed); 
        usleep(500*1000); // half a second
      }

      for(f=0; f<prefs.nthreads; f++) {
        pthread_join(threads[f], (void **)&retval);
        if (threadData[f].min < min) min = threadData[f].min; 
        if (threadData[f].max > max) max = threadData[f].max; 
      }
      if (err) {
        printf("\n\n***********************************************************\n"); 
        printf("BAD MOVIE: Got error while reading LOD %d of movie %s.\n", lod, moviename.c_str()); 
        printf("\n***********************************************************\n\n"); 
        numerrs ++; 
        continue; 
      }
      t1 = get_clock();

      totalMegabytes = 0; 
      for (f = 0; f < prefs.nthreads; f++) totalMegabytes += threadData[f].bytesRead; 
      totalMegabytes /= (1000.0*1000.0);  

      float totalSecs = t1-t0; 
      printf("%d frames %g seconds, ", totalFrames, totalSecs);
      printf("%g frames/second\n", totalFrames/totalSecs);
      printf("%0.2f MB actually read, %.2g MB/sec\n", totalMegabytes, totalMegabytes/totalSecs);  
      printf("min %g max %g\n",min, max);
    }
  } // end loop over lod

  printf("Got %d total errors during movie testing.\n", numerrs); 
  
  exit(numerrs);
}
