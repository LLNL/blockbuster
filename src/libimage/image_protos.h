/*
** $RCSfile: image_protos.h,v $
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
** $Id: image_protos.h,v 1.1 2007/06/13 18:59:30 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/

int ifilbuf(IMAGE *image);
int iflsbuf(IMAGE *image,unsigned int c);
int iclose(IMAGE *image);
int iflush(IMAGE *image);
void isetname(IMAGE *image,char *name);
void isetcolormap(IMAGE *image,int colormap);

void cvtimage(int *buffer);
void cvtlongs(int *buffer,int n);
void cvtshorts(unsigned short *buffer,int n);
unsigned int reverse(unsigned int lwrd);
unsigned short *ibufalloc(IMAGE *image);

IMAGE *imgopen(int f,char *file,char *mode,unsigned int type,unsigned int dim,
	unsigned int xsize,unsigned int ysize,unsigned int zsize);
IMAGE *fiopen(int f,char *mode,unsigned int type,unsigned int dim,
	unsigned int xsize,unsigned int ysize,unsigned int zsize);
IMAGE *iopen(char *file,char *mode,unsigned int type,unsigned int dim,
	unsigned int xsize,unsigned int ysize,unsigned int zsize);

/*	
int getpix(IMAGE *image);
int putpix(IMAGE *image,unsigned int pix);
*/

unsigned int img_optseek(IMAGE *image,unsigned int offset);
int img_read(IMAGE *image,char *buffer,int count);
int img_write(IMAGE *image,char *buffer,int count);
int img_badrow(IMAGE *image,int y,int z);
int img_seek(IMAGE *image,unsigned int y,unsigned int z);

void img_rle_expand(unsigned short *rlebuf,int ibpp,
	unsigned short *expbuf,int obpp);
int img_rle_compact(unsigned short *expbuf,int ibpp,
	unsigned short *rlebuf,int obpp,int cnt);
void img_setrowsize(IMAGE *image,int cnt,int y,int z);
int img_getrowsize(IMAGE *image);

int putrow(IMAGE *image,unsigned short *buffer,unsigned int y,unsigned int z);
int getrow(IMAGE *image,unsigned short *buffer,unsigned int y,unsigned int z);

void i_seterror(void (*func)(char *));
void i_errhdlr(char *err);
