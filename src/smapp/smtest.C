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

smBase *sm;
bool gVerbose; 
#define MAXIMUM_THREADS 128

int range[2] = {0,0}; 
std::vector<uint32_t> bytesRead;
 int nthreads=1; 
int nloops=1;
int pos[2] = {0,0};
int dim[2] = {0,0};
int step[2] = {1,1};
int bHaveRect = 0;
int pan = 0;
float dpos[2] = {0,0};
float ddim[2] = {0,0};
float dstep[2] = {0,0};

void dbprintf(const char *fmt, ...) {  
  if (!gVerbose) return; 
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

void *readThread(void *data)
{
   u_char *frame;
   uint64_t mynum = (uint64_t)data;
   int f,j;
   float *mm = (float*)malloc(2*sizeof(float));
   mm[0] = 1e+8;
   mm[1] = -1e+8;
   bytesRead[mynum] = 0; 
   frame = (u_char *)malloc(sizeof(u_char[4]) * sm->getWidth() *
                            sm->getHeight());
   int rowStride = 0;
   for(j=0;j<nloops;j++) {
      for (f=range[0]; f<=range[1]; f++) {
        if ((f % nthreads) == mynum) {
          double t0;
          if (!bHaveRect) {
            t0 = get_clock();
            bytesRead[mynum] += sm->getFrame(f, frame, mynum);
          } else {
            if(pan == 0) {
              int p[2],d[2],s[2];
              calcrect(f+j*sm->getNumFrames(),p,d,s);
              t0 = get_clock();
              //fprintf(stderr," d = [%d,%d], p = [%d,%d], s = [%d,%d]\n",d[0],d[1],p[0],p[1],s[0],s[1]);
              bytesRead[mynum] += sm->getFrameBlock(f, frame, mynum, rowStride, d, p, s);
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
                  fprintf(stderr," d = [%d,%d], p = [%d,%d], s = [%d,%d]\n",d[0],d[1],p[0],p[1],s[0],s[1]);
                  bytesRead[mynum] += sm->getFrameBlock(f, frame, mynum, rowStride, d, p, s);
                }
              }
            }
          }
          t0 = get_clock() - t0;
          if (t0 < mm[0]) mm[0] = t0;
          if (t0 > mm[1]) mm[1] = t0;
          
          dbprintf("Thread %d got frame %d\r", mynum, f);         
        
        }
       }
   }
   
   return(mm);
}

void usage(char *prg)
{
   fprintf(stderr, "(%s) usage: %s [options] smfilename\n",__DATE__,prg);
   fprintf(stderr,"Options:\n");
   fprintf(stderr, "\t -range first last:  first and last frames to run over.  (1-based indexes, inclusive interval)\n"); 
   fprintf(stderr,"\t-nt <num>   Select the number of threads/windows.  Default: 1\n");
   fprintf(stderr,"\t-rect <x y dx dy xinc yinc>  Rect/step to sample.  Default: whole\n");
   fprintf(stderr,"\t-drect <x y dx dy xinc yinc>  Float rect deltas.  Default: 0. 0. 0. 0. 0. 0.\n");
   fprintf(stderr,"\t-loops <n>  Number of loops to run. Default: 1\n");
   fprintf(stderr,"\t-pan Pan across using drect before advancing to next frame : rect must be provided as well\n");
   fprintf(stderr, "\t-v Be verbose\n"); 
   exit(1);
}

int main(int argc, char *argv[])
{
   int i, f;
   double t0, t1;
   pthread_t th[MAXIMUM_THREADS];
   float *retval;
   float mm[2] = {1e+8,-1e+8};

   smBase::init();

#ifdef irix
   if (pthread_setconcurrency(24) != 0)
     fprintf(stderr, "pthread_setconcurrency failed\n");
   printf("concurrency set to %d\n", pthread_getconcurrency());
#endif
   
   for (i=1; i<argc && argv[i][0]=='-'; i++) {
     if (strcmp(argv[i], "-nt") == 0) {
       if (i+1 >= argc) usage(argv[0]);
       nthreads=atoi(argv[i+1]);
       if (nthreads > MAXIMUM_THREADS) nthreads = MAXIMUM_THREADS;
       i++;
       bytesRead.resize(nthreads); 
     } else if (strcmp(argv[i], "-loops") == 0) {
       if (i+1 >= argc) usage(argv[0]);
       nloops=atoi(argv[i+1]);
       i++;
     } else if (strcmp(argv[i], "-drect") == 0) {
       if (i+6 >= argc) usage(argv[0]);
       dpos[0] = atof(argv[++i]);
       dpos[1] = atof(argv[++i]);
       ddim[0] = atof(argv[++i]);
       ddim[1] = atof(argv[++i]);
       dstep[0] = atof(argv[++i]);
       dstep[1] = atof(argv[++i]);
     } else if (strcmp(argv[i], "-rect") == 0) {
       if (i+6 >= argc) usage(argv[0]);
       pos[0] = atoi(argv[++i]);
       pos[1] = atoi(argv[++i]);
       dim[0] = atoi(argv[++i]);
       dim[1] = atoi(argv[++i]);
       step[0] = atoi(argv[++i]);
       step[1] = atoi(argv[++i]);
       bHaveRect = 1;
     } else if (strcmp(argv[i],"-pan") == 0) {
       fprintf(stderr,"Panning selected\n");
       pan = 1;
    } else if (strcmp(argv[i], "-range") == 0) {
       if (i+2 >= argc) usage(argv[0]);
       range[0] = atoi(argv[++i]);
       range[1] = atoi(argv[++i]);       
     } else if (strcmp(argv[i],"-v") == 0) {
       gVerbose = 1; 
     }
     else {
       fprintf(stderr,"Unknown arg: %s\n",argv[i]);
       exit(1);
     }
   }
   if (i != argc-1) usage(argv[0]);
   
   sm = smBase::openFile(argv[i], nthreads);

   if (!sm) {
      fprintf(stderr, "Unable to open movie: %s\n",argv[i]);
      exit(1);
   }

   // Default values
   if (!bHaveRect) {
      pos[0] = 0;
      pos[1] = 0;
      dim[0] = sm->getWidth();
      dim[1] = sm->getHeight();
      step[0] = 1;
      step[1] = 1;
   }

   // Double check
   i = 0;
   if ((pos[0]<0) || (pos[0]>=sm->getWidth())) i = 1;
   if ((pos[1]<0) || (pos[1]>=sm->getWidth())) i = 2;
   if ((step[0]<0) || (step[1]<0)) i = 3;
   if ((dim[0]<1) || (dim[0]+pos[0]*step[0]>sm->getWidth())) i = 4;
   if ((dim[1]<1) || (dim[1]+pos[1]*step[1]>sm->getHeight())) i = 5;
    
   if (i) {
      fprintf(stderr,"Error: Invalid frame rectangle : Case %d\n",i);
      exit(1);
   }

   // frame range is 1-based
   if (!range[0]) {
     range[0] = 1; 
   } 
   if (!range[1]) {
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
   int numframes = (range[1]-range[0]+1)*nloops; 
   float totalMegabytes = 0; 
   for (f=range[0]; f<=range[1]; f++) {
     totalMegabytes += sm->getCompFrameSize(f, 0); 
   }
   totalMegabytes *= nloops; 
   totalMegabytes /= (1000.0*1000.0); 
   fprintf(stderr, "Total size to read in %d frames, based on inputs: %f total MB, %f adjusted for given window fraction\n", 
           numframes, totalMegabytes, windowFraction*totalMegabytes); 

   for(f=0; f<nthreads; f++) {
      pthread_create(&th[f], NULL, readThread, (void*)f);
   }

   for(f=0; f<nthreads; f++) {
	pthread_join(th[f], (void **)&retval);
        if (retval[0] < mm[0]) mm[0] = retval[0];
        if (retval[1] > mm[1]) mm[1] = retval[1];
	free(retval);
   }

   t1 = get_clock();

   totalMegabytes = 0; 
   for (f = 0; f < nthreads; f++) totalMegabytes += bytesRead[f]; 
   totalMegabytes /= (1000.0*1000.0);  

   float totalSecs = t1-t0; 
   printf("%d frames %g seconds, ", numframes, totalSecs);
   printf("%g frames/second\n", (float)numframes/totalSecs);
   printf("%0.2f MB actually read, %.2g MB/sec\n", totalMegabytes, totalMegabytes/totalSecs);  
   printf("min %g max %g\n",mm[0],mm[1]);

   exit(0);
}
