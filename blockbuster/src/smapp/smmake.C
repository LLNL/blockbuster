/*
** $RCSfile: smmake.C,v $
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
** $Id: smmake.C,v 1.1 2007/06/13 18:59:35 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/


//
// smmake.C
//
//
//
#include <sys/types.h>
#include "sm/smRaw.h"
#include "sm/smRLE.h"

u_char buf[40][40][3];
u_int tilesizes[2];

int blockdim[2] ;
int blockpos[2];

TileInfo *tileinfo;

int
main(int argc, char *argv[])
{
   smRLE *sm;
   smRaw *sm2;
   int t, x, y;

#if 0
   sm= new smRLE("test.smrle", 40, 40, 40);

   for (t=0; t<40; t++) {
      for (x=0; x<40; x++)
         for (y=0; y<40; y++)
            buf[x][y][0] =
            buf[x][y][1] =
            buf[x][y][2] = t+10;
      sm->compressAndWriteFrame(t, buf);
   }

   sm->closeFile();

#endif

   tilesizes[0] = 10;
   tilesizes[1] = 10;
   int frames = 1;
   sm2 = new smRaw("test.smraw", 40, 40, frames, &tilesizes[0], 1);
   u_char *bufp = &buf[0][0][0];
   for (t=0; t<frames; t++) {
     for (x=0; x<40; x++)
       for (y=0; y<40; y++)
         buf[x][y][0] =
           buf[x][y][1] =
           buf[x][y][2] = t+10;
     sm2->compressAndWriteFrame(t, bufp);
   }
#if 0
   // test various tile overlap configurations -- include degenerate cases
   // first single pixel block at 0,0
   int nx = sm2->getTileNx(0);
   int ny = sm2->getTileNy(0);
   int numTiles =  nx * ny ;
  
   tileinfo = (tileInfo *)calloc(numTiles,sizeof(tileInfo));
   blockdim[0] = 1; blockdim[1] =1; blockpos[0]=0; blockpos[1]=0;
   sm2->computeTileOverlap( &blockdim[0],&blockpos[0], 0, tileinfo);

   printf("Degenerate Case -- Single pixel Lower Left Corner\n");

   for(int j=0; j < ny; j++) {
     for(int i=0; i < nx; i++) {
       if(tileinfo[(j*nx)+i].overlaps) {
	 printf(" Tile<x,y>[%d,%d]\n",i,j);
       }
     } 
   }

   blockdim[0] = 1; blockdim[1] =1; blockpos[0]=10; blockpos[1]=10;
   sm2->computeTileOverlap( &blockdim[0],&blockpos[0], 0, tileinfo);

   printf("Degenerate Case -- Single pixel Lower at LL of Tile <1,1>\n");

   for(int j=0; j < ny; j++) {
     for(int i=0; i < nx; i++) {
       if(tileinfo[(j*nx)+i].overlaps) {
	 printf(" Tile<x,y>[%d,%d]\n",i,j);
       }
     } 
   }

   printf("Degenerate Case -- Only first row has pixels\n");
   blockdim[0] = 40; blockdim[1] =1; blockpos[0]=0; blockpos[1]=0;
   sm2->computeTileOverlap( &blockdim[0],&blockpos[0], 0, tileinfo);
   for(int j=0; j < ny; j++) {
     for(int i=0; i < nx; i++) {
       if(tileinfo[(j*nx)+i].overlaps) {
	 printf(" Tile<x,y>[%d,%d]\n",i,j);
       }
     } 
   }

   printf("Degenerate Case -- Only first col has pixels\n");
   blockdim[0] = 1; blockdim[1] =40; blockpos[0]=0; blockpos[1]=0;
   sm2->computeTileOverlap( &blockdim[0],&blockpos[0], 0, tileinfo);
   for(int j=0; j < ny; j++) {
     for(int i=0; i < nx; i++) {
       if(tileinfo[(j*nx)+i].overlaps) {
	 printf(" Tile<x,y>[%d,%d]\n",i,j);
       }
     } 
   }


   printf("Degenerate Case -- Only col 39 has pixels\n");
   blockdim[0] = 1; blockdim[1] =40; blockpos[0]=39; blockpos[1]=0;
   sm2->computeTileOverlap( &blockdim[0],&blockpos[0], 0, tileinfo);
   for(int j=0; j < ny; j++) {
     for(int i=0; i < nx; i++) {
       if(tileinfo[(j*nx)+i].overlaps) {
	 printf(" Tile<x,y>[%d,%d]\n",i,j);
       }
     } 
   }


   printf("Case -- overlaps Tiles<0,0> AND <1,0>\n");
   blockdim[0] = 15; blockdim[1] =5; blockpos[0]=2; blockpos[1]=2;
   memset(tileinfo,0,sizeof(tileInfo_t)*numTiles);
   sm2->computeTileOverlap( &blockdim[0],&blockpos[0], 0, tileinfo);
   for(int j=0; j < ny; j++) {
     for(int i=0; i < nx; i++) {
       if(tileinfo[(j*nx)+i].overlaps) {
	 printf(" Tile<x,y>[%d,%d]\n",i,j);
	 printf(" Block Offset X : %d\n",tileinfo[(j*nx)+i].blockOffsetX);
	 printf(" Block Offset Y : %d\n",tileinfo[(j*nx)+i].blockOffsetY);
	 printf(" Tile Offset X : %d\n",tileinfo[(j*nx)+i].tileOffsetX);
	 printf(" Tile Offset Y : %d\n",tileinfo[(j*nx)+i].tileOffsetY);
	 printf(" Tile Length X : %d\n",tileinfo[(j*nx)+i].tileLengthX);
	 printf(" Tile Length Y : %d\n",tileinfo[(j*nx)+i].tileLengthY);
       }
     } 
   }

   printf("Case -- overlaps all\n");
   blockdim[0] = 35; blockdim[1] =35; blockpos[0]=2; blockpos[1]=2;
   memset(tileinfo,0,sizeof(tileInfo_t)*numTiles);
   sm2->computeTileOverlap( &blockdim[0],&blockpos[0], 0, tileinfo);
   for(int j=0; j < ny; j++) {
     for(int i=0; i < nx; i++) {
       if(tileinfo[(j*nx)+i].overlaps) {
	 printf(" Tile<x,y>[%d,%d]\n",i,j);
	 printf(" Block Offset X : %d\n",tileinfo[(j*nx)+i].blockOffsetX);
	 printf(" Block Offset Y : %d\n",tileinfo[(j*nx)+i].blockOffsetY);
	 printf(" Tile Offset X : %d\n",tileinfo[(j*nx)+i].tileOffsetX);
	 printf(" Tile Offset Y : %d\n",tileinfo[(j*nx)+i].tileOffsetY);
	 printf(" Tile Length X : %d\n",tileinfo[(j*nx)+i].tileLengthX);
	 printf(" Tile Length Y : %d\n",tileinfo[(j*nx)+i].tileLengthY);
       }
     } 
   }
#endif
   sm2->closeFile();
}
