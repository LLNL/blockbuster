/*
** $RCSfile: ifl2sm.C,v $
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
** $Id: ifl2sm.C,v 1.1 2007/06/13 18:59:34 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/



// Utility to combine image files into movie
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sm/sm.h>
#ifdef irix

#include <ifl/iflFile.h>
#include <pthread.h>
#include <ifl/iflTypeNames.h>
#include <ifl/iflDatabase.h>
#include "sm/smRLE.h"
#include "sm/smGZ.h"
#include "sm/smLZO.h"
#include "sm/smRaw.h"
#include "sm/smJPG.h"

extern "C" {
#include "pt/pt.h"
}

void readFile(char *);

#define RAW 0
#define RLE 1
#define GZ 2
#define LZO 3
#define JPG 4

int gbl_start=-1;
int gbl_end=0;
int numt=1;
int width,height,depth;
pt_gate_t *gate;
smBase *sm;
char suffix[10];
char base_name[255];
char sformat[10];
pthread_t *threadid;
iflConfig *config;

void *
worker(void *data)
{
   int tn = (int)data;
   iflFile* file;
   iflStatus sts;
   char filename[200];
   unsigned char *buffer,*cbuffer;
   int csize=width*height*depth;
   int i,size;

   buffer = (unsigned char *) calloc(csize,sizeof(unsigned char));
   cbuffer = (unsigned char *) calloc(csize*1.5,sizeof(unsigned char));

   for (i=gbl_start+tn;i<gbl_start+gbl_end;i+=numt) {
      //sprintf(filename,"%s%.4d%s",base_name,i,suffix);
      sprintf(filename,sformat,base_name,i,suffix);
      file = iflFile::open(filename, O_RDONLY, &sts);
      sts = file->getTile(0,0,0,width,height,1,buffer,config);
      if (sts != iflOKAY) {printf("Can't read image %d",i);exit(0);}

      sm->compFrame(buffer, NULL, size);
      if (size > csize) {
	    csize = size;
            free(cbuffer);
   	    cbuffer = (unsigned char *) calloc(csize,sizeof(unsigned char));
      }
      sm->compFrame(buffer, cbuffer, size);

      pt_gate_sync(&gate[tn]);
      sm->setCompFrame(i-gbl_start, cbuffer, size);
      pt_gate_sync(&gate[tn]);

      file->close();
   }

   return(NULL);
}

int main(int argc,char **argv)
   {
   int i=1;
   int smformat = RAW;
   char filein[255];
   if(argc == 1)
      {
      printf("Usage: %s [-nt n] [-rle] [-gz] [-lzo] [-jpg] -f name####.suffix\n",argv[0]);
      exit(0);
      }
   while(i< argc)
      {
      if(strcmp(argv[i],"-f") == 0)
         {
         ++i;
         strcpy(filein,argv[i]);
         ++i;
         }
      else if(strcmp(argv[i],"-rle") == 0)
         {
         ++i;
         smformat = RLE;
         }
      else if(strcmp(argv[i],"-jpg") == 0)
         {
         ++i;
         smformat = JPG;
         }
      else if(strcmp(argv[i],"-gz") == 0)
         {
         ++i;
         smformat = GZ;
         }
      else if(strcmp(argv[i],"-lzo") == 0)
         {
         ++i;
         smformat = LZO;
         }
      else if(strcmp(argv[i],"-r") == 0)
         {
         ++i;
         strcpy(filein,argv[i]);
	 readFile(filein);
         ++i;
         }
      else if(strcmp(argv[i],"-nt") == 0)
         {
         ++i;
         numt = atoi(argv[i]);
         ++i;
         }
      else
	 {
	 printf("Bad option: %s\n",argv[i]);
         printf("Usage: %s [-nt n] [-rle] [-gz] [-lzo] [-jpg] -f name####.suffix\n",argv[0]);
	 exit(0);
	 }
      }

   // Assume file of form ...####....suffix
   char *first_num=strchr(filein,'#');
   char tail[100];
   sprintf(tail,"%s",first_num);
   int tail_len = strlen(tail);

   char *dot=strrchr(filein,'.');
   sprintf(suffix,"%s",dot);
   printf("File %s suffix %s len %d\n",filein,suffix,strlen(suffix));

   // numerical field width
   int num_width= tail_len - strlen(suffix);

   // base file name length
   int len = strlen(filein) - tail_len;
   strncpy(base_name,filein,len);
   base_name[len] = '\0';
   //printf("File %s Base name %s length %d\n",filein,base_name,len);

   // Format string for constructing file names
   sprintf(sformat,"%s%d%s","%s%.",num_width,"d%s");

   // Count the files
   int nfiles_max = pow(10.0,num_width)-1; 
   char filename[255];
   iflStatus sts;
   iflFile* file;
   iflSize dims;
   iflFormat *format;
   for(i=0;i<nfiles_max;++i)
      {
      //sprintf(filename,"%s%.4d%s",base_name,i,suffix);
      sprintf(filename,sformat,base_name,i,suffix);
      file = iflFile::open(filename, O_RDONLY, &sts);
      if (sts != iflOKAY) 
	 {
	 if(gbl_start >=0) break;
         }
      else
	 {
	 if(gbl_start <0) 
	    {
	    gbl_start=i;
            file->getDimensions(dims);
	    width = dims.x;
	    height = dims.y;
	    depth = dims.c;
	    format = file->getFormat();
	    }
	 else
	    {
            file->getDimensions(dims);
	    if(dims.x != width || dims.y != height)
	       {
	       printf("File %s image size is %d %d, expecting %d %d\n",
		  filename,dims.x,dims.y,width,height);
	       exit(0);
	       }
	    }
	 ++gbl_end;
         file->close();
	 }
      }
   // gbl_start+gbl_end-1 is the number of the last frame
   printf("First file number %d last %d\n",gbl_start,gbl_start+gbl_end-1);
   printf("Image size %d %d %d\n",width,height,depth);
   printf("Input format type is %s\n",format->getName());

   // Total number of frames
   int nframes = gbl_end;

   // All supported types
   iflDatabase *db;
   int index=0;
   printf("Supported file types:\n");
   while((db=iflDatabase::findNext(index)) !=NULL) printf("%s\n",db->getName());

   // Create combined file
   iflFormat *cformat = iflFormat::findByFormatName("TIFF");  
//   printf("Output file format name: %s\n",cformat->getName());
   char filecat[255];
   sprintf(filecat,"%s%s",base_name,".sm");

   iflSize dimc(width,height,nframes,depth);
   iflCompression comp=iflLZW;
      //iflZIP;
      //iflJPEG;
      //iflSGIRLE;
      //iflLZW;
      //iflPACKBITS;
      //iflNoCompression;
   iflFileConfig fc(
      &dimc,       // image dimensions
      iflUChar,    // data type
      iflInterleaved,  // ordering 
      iflColorModel(0), // color model iflRGB
      //iflOrientation(0),  // orientation iflLowerLeftOrigin
      iflLowerLeftOrigin,
      comp);

   // Data will be converted to this format
   config = new iflConfig(iflUChar,
      iflInterleaved,
      0,NULL,0,
      iflLowerLeftOrigin);

   // Movie file format
#if 0
   int supported=cformat->sizeIsSupported(width,height,nframes,depth,
      iflOrientation(0));
   printf("Format %s %s size of %d %d %d %d\n",cformat->getName(),
      supported ? "supports" : "doesn't support", width,height,nframes,depth);
   supported=cformat->compressionIsSupported(comp);
   printf("Format %s %s %s compression\n",cformat->getName(),
      supported ? "supports" : "doesn't support",iflCompressionName(comp));
   iflCompression pref_comp = cformat->getPreferredCompression();
   printf("Preferred compression is %s\n",iflCompressionName(pref_comp));
   iflFile* filec = iflFile::create(filecat, 
	NULL,    // source (iflFile*)
	&fc,     // config (iflFileConfig*)
	//NULL,    // format (iflFormat*)
	cformat,
	&sts);
   if (sts != iflOKAY) {printf("Can't create file: %s error %d\n",
      filecat,sts);
      char error[80];
      printf("%s\n",iflStatusToString(sts,error,80));exit(0);}

   printf("Created file %s\n",filecat);

   // Read data and write to combined file
   unsigned char *buffer;
   int size=width*height*depth;
   buffer = (unsigned char *) calloc(size,sizeof(unsigned char));
   for(i=gbl_start;i<gbl_start+gbl_end;++i)
      {
      //sprintf(filename,"%s%.4d%s",base_name,i,suffix);
      sprintf(filename,sformat,base_name,i,suffix);
      file = iflFile::open(filename, O_RDONLY, &sts);
      sts = file->getTile(0,0,0,width,height,1,buffer);
      if (sts != iflOKAY) {printf("Can't read image %d",i);exit(0);}
      file->close();

      sts = filec->setTile(0,0,i-gbl_start,width,height,1,buffer);
      if (sts != iflOKAY) {printf("Can't write data\n");exit(0);}
      //sts = filec->appendImg(file,&fc);
      //if (sts != iflOKAY) {printf("Can't append image %d\n",i);exit(0);}
      }

 
   filec->close();
#endif

   if (smformat == RLE) {
      sm= smRLE::newFile(filecat, width, height, nframes);
   } else if (smformat == GZ) {
      sm= smGZ::newFile(filecat, width, height, nframes);
   } else if (smformat == LZO) {
      sm= smLZO::newFile(filecat, width, height, nframes);
   } else if (smformat == JPG) {
      sm= smJPG::newFile(filecat, width, height, nframes);
   } else {
      sm= smRaw::newFile(filecat, width, height, nframes);
   }

   if (!sm) {
	smdbprintf(0,"Unable to create new file: %s\n",filecat);
	exit(1);
   }

   // Read data and write to combined file
   if (numt == 1) {
      unsigned char *buffer;
      //unsigned char *obuffer;
      int size=width*height*depth;
      buffer = (unsigned char *) calloc(size,sizeof(unsigned char));
      //obuffer = (unsigned char *) calloc(size,sizeof(unsigned char));
      for(i=gbl_start;i<gbl_start+gbl_end;++i)
         {
         //sprintf(filename,"%s%.4d%s",base_name,i,suffix);
         sprintf(filename,sformat,base_name,i,suffix);
         file = iflFile::open(filename, O_RDONLY, &sts);
         sts = file->getTile(0,0,0,width,height,1,buffer,config);
         if (sts != iflOKAY) {printf("Can't read image %d",i);exit(0);}
	 /*
         if (file->getOrientation() == iflUpperLeftOrigin) {
            for (int y=0; y<height; y++)
               memcpy(&obuffer[y*width*3], &buffer[(height-y-1)*width*3],
                      sizeof(char[3])*width);
            sm->setFrame(i-gbl_start, obuffer);
         }
         else
	 */
            sm->setFrame(i-gbl_start, buffer);
         file->close();
         }
   }
   else {
      gate = (pt_gate_t *)malloc(sizeof(pt_gate_t) * numt);
      threadid = (pthread_t *)malloc(sizeof(pthread_t) * numt);
      for (i=0; i<numt; i++) {
         pt_gate_init(&gate[i], 2);
         pthread_create(&threadid[i], NULL, worker, (void *)i);
      }
      for(i=gbl_start;i<gbl_start+gbl_end;++i) {
         pt_gate_sync(&gate[(i-gbl_start)%numt]);
         pt_gate_sync(&gate[(i-gbl_start)%numt]);
      }
   }

   sm->closeFile();
   }

void readFile(char *filename)
   {

   iflStatus sts;
   iflFile* file;
   iflSize dims;
   int width,height,times,depth;
   iflFormat *format;

   file = iflFile::open(filename, O_RDONLY, &sts);
   file->getDimensions(dims);
   width = dims.x;
   height = dims.y;
   times = dims.z;
   depth = dims.c;
   format = file->getFormat();
   printf("Image size %d %d %d %d\n",width,height,times,depth);
   printf("Format is %s\n",format->getName());

   exit(0);
   }

#else

int main(int argc, char **argv)
{
	smdbprintf(0,"%s : unsupported on this platform.\n",argv[0]);
	exit(1);
}
#endif
