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
#include "../common/timer.h"

smBase *sm;
uint32_t gVerbose; 

int range[2] = {0,0}; 
std::vector<double> bytesRead;
 int nthreads=1;
int lod = 1; 
int nloops=1;
int pos[2] = {0};
int dim[2] = {0};
int step[2] = {1};
float fltpos[2] = {0}, fltdim[2]={0}, fltstep[2]={0}; 

int bHaveRect = 0;
int pan = 0;
float dpos[2] = {0,0};
float ddim[2] = {0,0};
float dstep[2] = {0,0};

void dbprintf(int level, const char *fmt, ...) {  
  if (gVerbose<level) return; 
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr,fmt,ap);
  va_end(ap);
  return; 
}

void calcrectpan(int x,int y,int *p,int *d,int *s)
{

  // move the frame 
  p[0] = pos[0] + (int)(x*dpos[0]);
  p[1] = pos[1] + (int)(y*dpos[1]);
  d[0] = dim[0];
  d[1] = dim[1];
  s[0] = step[0];
  s[1] = step[1];
  
   // make it valid...

   // adjust scales...
   if (s[0] < 1) s[0] = 1;
   if (s[1] < 1) s[1] = 1;
   if (d[0] < 1) d[0] = 1;
   if (d[1] < 1) d[1] = 1;
   if (d[0] > sm->getWidth()) d[0] = sm->getWidth();
   if (d[1] > sm->getHeight()) d[1] = sm->getHeight();
   if (s[0]*d[0] > sm->getWidth()) s[0] = sm->getWidth()/d[0];
   if (s[1]*d[1] > sm->getHeight()) s[1] = sm->getHeight()/d[1];

   // make the pos valid...
   if (p[0] < 0) p[0] -= 1;
   if (p[1] < 0) p[1] -= 1;
   p[0] = p[0] % sm->getWidth();
   p[1] = p[1] % sm->getHeight();
   if ((p[0] + d[0]*s[0]) > sm->getWidth()) p[0] = sm->getWidth()-(d[0]*s[0]);
   if ((p[1] + d[1]*s[1]) > sm->getHeight()) p[1] = sm->getHeight()-(d[1]*s[1]);

   return;
}

void calcrect(int n,int *p,int *d,int *s)
{
   float f = (float)n;

   // move the frame 
   p[0] = pos[0] + (int)(f*dpos[0]);
   p[1] = pos[1] + (int)(f*dpos[1]);
   d[0] = dim[0] + (int)(f*ddim[0]);
   d[1] = dim[1] + (int)(f*ddim[1]);
   s[0] = step[0] + (int)(f*dstep[0]);
   s[1] = step[1] + (int)(f*dstep[1]);

   // make it valid...

   // adjust scales...
   if (s[0] < 1) s[0] = 1;
   if (s[1] < 1) s[1] = 1;
   if (d[0] < 1) d[0] = 1;
   if (d[1] < 1) d[1] = 1;
   if (d[0] > sm->getWidth()) d[0] = sm->getWidth();
   if (d[1] > sm->getHeight()) d[1] = sm->getHeight();
   if (s[0]*d[0] > sm->getWidth()) s[0] = sm->getWidth()/d[0];
   if (s[1]*d[1] > sm->getHeight()) s[1] = sm->getHeight()/d[1];

   // make the pos valid...
   if (p[0] < 0) p[0] -= 1;
   if (p[1] < 0) p[1] -= 1;
   p[0] = p[0] % sm->getWidth();
   p[1] = p[1] % sm->getHeight();
   if ((p[0] + d[0]*s[0]) > sm->getWidth()) p[0] = sm->getWidth()-(d[0]*s[0]);
   if ((p[1] + d[1]*s[1]) > sm->getHeight()) p[1] = sm->getHeight()-(d[1]*s[1]);

   return;
}

double get_clock(void)
{
    struct timeval t;

    gettimeofday(&t,NULL);
    return((double)t.tv_sec + (double)t.tv_usec*1E-6);
}
struct ThreadData {
  ThreadData(): threadNum(0), numFramesRead(0), 
                currentFrame(0), finished(false) {}
  uint32_t threadNum;
  uint32_t numFramesRead, currentFrame; 
  bool finished; 
};

void *readThread(void *data)
{
   u_char *frame;
   ThreadData *threadData = (ThreadData*)data; 
   uint32_t mynum = threadData->threadNum;
   int32_t loopNum, f;
   float *mm = (float*)malloc(2*sizeof(float));
   mm[0] = 1e+8;
   mm[1] = -1e+8;
   bytesRead[mynum] = 0; 
   frame = (u_char *)malloc(sizeof(u_char[4]) * sm->getWidth() *
                            sm->getHeight());
   int rowStride = 0;
   for(loopNum=0;loopNum<nloops;loopNum++) {    
     for (f=range[0]; f<=range[1]; f++) {
       if ((f % nthreads) == mynum) {
         double t0;
         if (!bHaveRect) {
           t0 = get_clock();
           bytesRead[mynum] += sm->getFrame(f, frame, mynum, lod);
         } else {
           if(pan == 0) {
             int p[2],d[2],s[2];
             calcrect(f+loopNum*sm->getNumResolutions()*sm->getNumFrames(),
                      p,d,s);
             t0 = get_clock();
             //fprintf(stderr," d = [%d,%d], p = [%d,%d], s = [%d,%d]\n",d[0],d[1],p[0],p[1],s[0],s[1]);
             bytesRead[mynum] += sm->getFrameBlock(f, frame, mynum, rowStride, d, p, s, lod);
           }
           else {
             int xStep,yStep;
             int p[2],d[2],s[2];
             int stepsX = (int)floor( (double)sm->getWidth()/dpos[0]);
             int stepsY = (int)floor((double)sm->getHeight()/dpos[1]);
             if(stepsX < 1) stepsX = 1;
             if(stepsY < 1) stepsY = 1;
             fprintf(stderr,"Position Steps[%d,%d]\n",stepsX,stepsY);
             t0 = get_clock();
             for(yStep = 0; yStep < stepsY; yStep++) {
               for(xStep = 0; xStep < stepsX; xStep++) {
                 calcrectpan(xStep,yStep,p,d,s);
                 //fprintf(stderr," d = [%d,%d], p = [%d,%d], s = [%d,%d]\n",d[0],d[1],p[0],p[1],s[0],s[1]);
                 bytesRead[mynum] += sm->getFrameBlock(f, frame, mynum, rowStride, d, p, s, lod);
               }
             }
           }
        }
         t0 = get_clock() - t0;
         if (t0 < mm[0]) mm[0] = t0;
         if (t0 > mm[1]) mm[1] = t0;
         threadData->numFramesRead++; 
         threadData->currentFrame = f; 
         
         //dbprintf("Thread %d got frame %d\r", mynum, f);         
         
       }
     }
   }
   threadData->finished = true; 
   return(mm);
}

void usage(char *prg)
{
   fprintf(stderr, "(%s) usage: %s [options] smfilename\n",__DATE__,prg);
   fprintf(stderr,"Options:\n");
   fprintf(stderr, "\t -h or -help:  display this menu\n"); 
   fprintf(stderr,"\t-lod <num> Level of detail.  default: 0\n");
   
   fprintf(stderr, "\t -range first last:  first and last frames to run over.  (1-based indexes, inclusive interval)\n"); 
   fprintf(stderr,"\t-nt <num>   Select the number of threads/windows.  Default: 1\n");
   fprintf(stderr,"\t-rect <xpos ypos xsize ysize xinc yinc>  Rect/step to sample.  Rect values are float.  Either pixels or window fractions between 0.01 and 0.99 are acceptable.  Default: whole\n");
   fprintf(stderr,"\t-pan <xpos ypos xsize ysize xinc yinc> Pan across using giiven rect before advancing to next frame.  Rect values are floats.  Either pixels or window fractions between 0.01 and 0.99 are acceptable.  Default: no panning.\n");
   fprintf(stderr,"\t-loops <n>  Number of loops to run. Default: 1\n");
   fprintf(stderr, "\t-v n Set verbosity to level n (0-5)\n"); 
   return;
}

void ConvertFraction(float &fraction, float refValue) {
  if (0.0 < fraction && fraction <= 1.0001) fraction *= refValue; 
  return; 
}

void ConvertFractions(char *name, float ffpos[2], float ffdim[2], float ffstep[2]) {
  if (0.0 < ffpos[0]  && ffpos[0]  <= 1.001) 
    ffpos[0]  = ffpos[0]*sm->getWidth()-1;

  if (0.0 < ffpos[1]  && ffpos[1]  <= 1.001) 
    ffpos[1]  = ffpos[1]*sm->getHeight()-1; 

  if (0.0 < ffdim[0]  && ffdim[0]  <= 1.001) 
    ffdim[0]  = ffdim[0]*sm->getWidth()-1;

  if (0.0 < ffdim[1]  && ffdim[1]  <= 1.001) 
    ffdim[1]  = ffdim[1]*sm->getHeight()-1; 

  if (0.0 < ffstep[0] && ffstep[0] <  0.990) 
    ffstep[0] = ffstep[0]*sm->getWidth()-1;

  if (0.0 < ffstep[1] && ffstep[1] <  0.990) 
    ffstep[1] = ffstep[1]*sm->getHeight()-1;

  fprintf(stderr, "After converting from fraction, %s is {%f, %f, %f, %f, %f, %f}\n", name, ffpos[0], ffpos[1], ffdim[0], ffdim[1], ffstep[0], ffstep[1]); 
  return; 
} 

void RoundRectToInt(void) {
  pos[0] = (int)(fltpos[0]+0.5); 
  pos[1] = (int)(fltpos[1]+0.5); 
  dim[0] = (int)(fltdim[0]+0.5); 
  dim[1] = (int)(fltdim[1]+0.5); 
  step[0] = (int)(fltstep[0]+0.5); 
  step[1] = (int)(fltstep[1]+0.5); 
  fprintf(stderr, "After rounding, crop rectangle is {%d, %d, %d, %d, %d, %d}\n", pos[0], pos[1], dim[0], dim[1], step[0], step[1]); 
  return; 
}

int main(int argc, char *argv[])
{
   int i, f;
   double t0, t1;
   vector<pthread_t> threads;
   float *retval;
   float mm[2] = {1e+8,-1e+8};
   
   smBase::init();

#ifdef irix
   if (pthread_setconcurrency(24) != 0)
     fprintf(stderr, "pthread_setconcurrency failed\n");
   printf("concurrency set to %d\n", pthread_getconcurrency());
#endif
   
   for (i=1; i<argc && argv[i][0]=='-'; i++) {
     if (strncmp(argv[i], "-h", 2) == 0) {
       usage(argv[0]); 
       exit(0); 
     } else if (strcmp(argv[i], "-lod") == 0) {
       if (i+1 >= argc) usage(argv[0]);
       lod=atoi(argv[i+1]);
       i++;
     } else if (strcmp(argv[i], "-nt") == 0) {
       if (i+1 >= argc) usage(argv[0]);
       nthreads=atoi(argv[i+1]);
       i++;
     } else if (strcmp(argv[i], "-loops") == 0) {
       if (i+1 >= argc) usage(argv[0]);
       nloops=atoi(argv[i+1]);
       i++;
     } else if (strcmp(argv[i], "-pan") == 0) {
       fprintf(stderr,"Panning selected\n");
       if (i+6 >= argc) usage(argv[0]);
       dpos[0] = atof(argv[++i]);
       dpos[1] = atof(argv[++i]);
       ddim[0] = atof(argv[++i]);
       ddim[1] = atof(argv[++i]);
       dstep[0] = atof(argv[++i]);
       dstep[1] = atof(argv[++i]);
     } else if (strcmp(argv[i],"-rect") == 0) {
       if (i+6 >= argc) usage(argv[0]);
       fltpos[0] = atof(argv[++i]);
       fltpos[1] = atof(argv[++i]);
       fltdim[0] = atof(argv[++i]);
       fltdim[1] = atof(argv[++i]);
       fltstep[0] = atof(argv[++i]);
       fltstep[1] = atof(argv[++i]);
       bHaveRect = 1;
    } else if (strcmp(argv[i], "-range") == 0) {
       if (i+2 >= argc) usage(argv[0]);
       range[0] = atoi(argv[++i]);
       range[1] = atoi(argv[++i]);    
     } else if (strcmp(argv[i],"-v") == 0) {
       gVerbose = atoi(argv[++i]); 
       sm_setVerbose(gVerbose); 
     }
     else {
       fprintf(stderr,"\n******************\nUnknown arg: %s\n******************\n",argv[i]);
       usage(argv[0]); 
       exit(1);
     }
   }
   if (argc < 2) {
     fprintf(stderr, "Not enough arguments.  Require a filename.\n"); 
     usage(argv[0]); 
     exit(3); 
   }
   if (i != argc-1) {
     fprintf(stderr, "Error parsing arguments.\n"); 
     usage(argv[0]);
     exit(2); 
   }
   sm = smBase::openFile(argv[i], nthreads);
   threads.resize(nthreads); 
   bytesRead.resize(nthreads); 
    
   if (!sm) {
      fprintf(stderr, "Unable to open movie: %s\n",argv[i]);
      exit(4);
   }
   dbprintf(3, "movie is %dx%d\n", sm->getWidth(), sm->getHeight()); 
   ConvertFractions("pan rect", dpos, ddim, dstep); 
   
   // Default values
   if (!bHaveRect) {
      pos[0] = 0;
      pos[1] = 0;
      dim[0] = sm->getWidth();
      dim[1] = sm->getHeight();
      step[0] = 1;
      step[1] = 1;
   } else {
     ConvertFractions("crop rect", fltpos,fltdim,fltstep); 
     RoundRectToInt(); 
   }

   // Double check
   i = 0;
   if ((pos[0]<0) || (pos[0]>=sm->getWidth())) i = 1;
   if ((pos[1]<0) || (pos[1]>=sm->getWidth())) i = 2;
   if ((step[0]<0) || (step[1]<0)) i = 3;
   if ((dim[0]<1) || (dim[0]+pos[0]*step[0]>sm->getWidth())) {
     fprintf(stderr, "LH = %d, RH = %d\n", dim[0]+pos[0]*step[0], sm->getWidth()); 
     i = 4;
   }
   if ((dim[1]<1) || (dim[1]+pos[1]*step[1]>sm->getHeight())) i = 5;
   
   if (i) {
      fprintf(stderr,"Error: Invalid frame rectangle : Case %d\n",i);
      exit(5);
   }

   if (lod > sm->getNumResolutions()) {
     fprintf(stderr, "Error:  lod %d specified, but movie has only %d\n", 
             lod, sm->getNumResolutions()); 
     exit(6); 
   }
   // frame range is 1-based
   if (!range[0]) {
     range[0] = 1; 
   } 
   if (!range[1] || range[1] > sm->getNumFrames()) {
     range[1] = sm->getNumFrames(); 
   }    
   printf("threads: %d\n",nthreads);
   printf("frame size: %d,%d\n", sm->getWidth(), sm->getHeight());
   printf("rect: %d,%d @ %d,%d step %d,%d\n",dim[0],dim[1],pos[0],pos[1],step[0],step[1]);
   printf("Frame range: %d to %d\n", range[0], range[1]); 
   t0 = get_clock();

   // convert to 0-based frames now: 
   range[0] -= 1; 
   range[1] -= 1; 


   // computer estimated movie size to give user something to do while we work
   float windowFraction = ((float)dim[0]/sm->getWidth())*((float)dim[1]/sm->getHeight());    
   int totalFrames = (range[1]-range[0]+1)*nloops; 
   float totalMegabytes = 0; 
   uint32_t cur_size = 0; 
   for (f=range[0]; f<=range[1]; f++) {
     cur_size =      sm->getCompFrameSize(f, 0); 
     totalMegabytes += cur_size;
     dbprintf(5, "size of frame %d is %d\n", f, cur_size); 

   }
   totalMegabytes /= (1000.0*1000.0); 
   float mbPerFrameEst = totalMegabytes/(range[1]-range[0]+1)*windowFraction; 
   totalMegabytes *= nloops; 
   fprintf(stderr, "Total size to read in %d frames, based on inputs: %f total MB, %f adjusted for given window fraction.  average MB/frame = %f\n", 
           totalFrames, totalMegabytes, windowFraction*totalMegabytes, mbPerFrameEst); 
   vector<ThreadData> threadData(f); 
   for(f=0; f<nthreads; f++) { 
     threadData[f].threadNum = f; 
     pthread_t *tp = &threads[f]; 
     void *vp = (void*)(&threadData[f]);
     pthread_create(tp, NULL, readThread, vp);
   }
   bool done = false; 
   double previousTime = GetExactSecondsDouble(), totalTime = 0; 
   double elapsed, fps, megabytes, totalMB = 0, nframes; 
   vector<double> previousFrames(nthreads+1, 0), numFrames(nthreads+1, 0), 
     previousBytes(nthreads+1, 0); 
   while (!done) {
     previousFrames[nthreads] = numFrames[nthreads]; 
     numFrames[nthreads] = 0; 
     elapsed = GetExactSecondsDouble() - previousTime; 
     totalTime += elapsed; 
     previousTime = GetExactSecondsDouble(); 
     fprintf(stderr, "t = %05.3f: ", totalTime); 
     totalMB = 0; 
     for(f=0; f<nthreads; f++) {
       previousFrames[f] = numFrames[f]; 
       done = done || threadData[f].finished; 
       numFrames[f] = threadData[f].numFramesRead; 
       numFrames[nthreads] += numFrames[f]; 
       nframes = numFrames[f]-previousFrames[f];
       fps = nframes / elapsed; 
       megabytes = (bytesRead[f] - previousBytes[f])/(1000*1000); 
       totalMB += megabytes; 
       previousBytes[f] = bytesRead[f]; 
       fprintf(stderr, "Thread %02d: frame %05d, %03.2f frames in %f secs =  %05.3f fps, actual MB/sec %f\n", f, threadData[f].currentFrame, nframes, elapsed, fps, megabytes/elapsed); 
     }
     fps = (numFrames[nthreads]-previousFrames[nthreads])/elapsed; 
     float pct = (numFrames[nthreads])/totalFrames*100.0;
     fprintf(stderr, "ALL THREADS: %4.2f%% complete, fps = %5.3f, actual MB/sec %f\n", pct, fps, totalMB/elapsed); 
     usleep(500*1000); // half a second
   }

   for(f=0; f<nthreads; f++) {
	pthread_join(threads[f], (void **)&retval);
        if (retval[0] < mm[0]) mm[0] = retval[0];
        if (retval[1] > mm[1]) mm[1] = retval[1];
	free(retval);
   }

   t1 = get_clock();

   totalMegabytes = 0; 
   for (f = 0; f < nthreads; f++) totalMegabytes += bytesRead[f]; 
   totalMegabytes /= (1000.0*1000.0);  

   float totalSecs = t1-t0; 
   printf("%d frames %g seconds, ", totalFrames, totalSecs);
   printf("%g frames/second\n", totalFrames/totalSecs);
   printf("%0.2f MB actually read, %.2g MB/sec\n", totalMegabytes, totalMegabytes/totalSecs);  
   printf("min %g max %g\n",mm[0],mm[1]);

   exit(0);
}
