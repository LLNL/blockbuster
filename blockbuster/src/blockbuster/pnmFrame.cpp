/* This file is derived from "pnmrw.h".  The following changes have been made:
 * - Error messages issued via fprintf() have been changed to use MESSAGE().
 * - pnm_init2() has been removed.  This routine initialized the program name
 *   in order to print error messages correctly; it is no longer needed.
 */

#include "pnmFrame.h"
#include "errmsg.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "errmsg.h"
#include "util.h"
#include <X11/Xlib.h>

/* pnmrw.h - header file for PBM/PGM/PPM read/write library
**
** Copyright (C) 1988,1989,1991 by Jef Poskanzer <jef@acme.com>.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

#ifndef _PNMRW_H_
#define _PNMRW_H_


/* Types. */

typedef unsigned char bit;
#define PBM_WHITE 0
#define PBM_BLACK 1
#define PBM_FORMAT_TYPE(f) ((f) == PBM_FORMAT || (f) == RPBM_FORMAT ? PBM_TYPE : -1)

typedef unsigned char gray;
#define PGM_MAXMAXVAL 255
#define PGM_FORMAT_TYPE(f) ((f) == PGM_FORMAT || (f) == RPGM_FORMAT ? PGM_TYPE : PBM_FORMAT_TYPE(f))

typedef gray pixval;
#define PPM_MAXMAXVAL PGM_MAXMAXVAL
typedef struct
{
  pixval r, g, b;
} pixel;
#define PPM_GETR(p) ((p).r)
#define PPM_GETG(p) ((p).g)
#define PPM_GETB(p) ((p).b)
#define PPM_ASSIGN(p,red,grn,blu) do { (p).r = (red); (p).g = (grn); (p).b = (blu); } while ( 0 )
#define PPM_EQUAL(p,q) ( (p).r == (q).r && (p).g == (q).g && (p).b == (q).b )
#define PPM_FORMAT_TYPE(f) ((f) == PPM_FORMAT || (f) == RPPM_FORMAT ? PPM_TYPE : PGM_FORMAT_TYPE(f))

typedef pixel xel;
typedef pixval xelval;
#define PNM_MAXMAXVAL PPM_MAXMAXVAL
#define PNM_GET1(x) PPM_GETB(x)
#define PNM_ASSIGN1(x,v) PPM_ASSIGN(x,0,0,v)
#define PNM_EQUAL(x,y) PPM_EQUAL(x,y)
#define PNM_FORMAT_TYPE(f) PPM_FORMAT_TYPE(f)


/* Magic constants. */

#define PBM_MAGIC1 'P'
#define PBM_MAGIC2 '1'
#define RPBM_MAGIC2 '4'
#define PBM_FORMAT (PBM_MAGIC1 * 256 + PBM_MAGIC2)
#define RPBM_FORMAT (PBM_MAGIC1 * 256 + RPBM_MAGIC2)
#define PBM_TYPE PBM_FORMAT

#define PGM_MAGIC1 'P'
#define PGM_MAGIC2 '2'
#define RPGM_MAGIC2 '5'
#define PGM_FORMAT (PGM_MAGIC1 * 256 + PGM_MAGIC2)
#define RPGM_FORMAT (PGM_MAGIC1 * 256 + RPGM_MAGIC2)
#define PGM_TYPE PGM_FORMAT

#define PPM_MAGIC1 'P'
#define PPM_MAGIC2 '3'
#define RPPM_MAGIC2 '6'
#define PPM_FORMAT (PPM_MAGIC1 * 256 + PPM_MAGIC2)
#define RPPM_FORMAT (PPM_MAGIC1 * 256 + RPPM_MAGIC2)
#define PPM_TYPE PPM_FORMAT


/* Color scaling macro -- to make writing ppmtowhatever easier. */

#define PPM_DEPTH(newp,p,oldmaxval,newmaxval)                           \
  PPM_ASSIGN( (newp),                                                   \
              ( (int) PPM_GETR(p) * (newmaxval) + (oldmaxval) / 2 ) / (oldmaxval), \
              ( (int) PPM_GETG(p) * (newmaxval) + (oldmaxval) / 2 ) / (oldmaxval), \
              ( (int) PPM_GETB(p) * (newmaxval) + (oldmaxval) / 2 ) / (oldmaxval) )


/* Luminance macro. */

#define PPM_LUMIN(p) ( 0.299 * PPM_GETR(p) + 0.587 * PPM_GETG(p) + 0.114 * PPM_GETB(p) )


/* Declarations of pnmrw routines. */

void pnm_init2 ( char* pn );

char** pm_allocarray ( int cols, int rows, int size );
#define pnm_allocarray( cols, rows ) ((xel**) pm_allocarray( cols, rows, sizeof(xel) ))
char* pm_allocrow( int cols, int size );
#define pnm_allocrow( cols ) ((xel*) pm_allocrow( cols, sizeof(xel) ))
void pm_freearray ( char** its, int rows );
#define pnm_freearray( xels, rows ) pm_freearray( (char**) xels, rows )
void pm_freerow ( char* itrow );
#define pnm_freerow( xelrow ) pm_freerow( (char*) xelrow )

xel** pnm_readpnm( FILE* file, int* colsP, int* rowsP, xelval* maxvalP, int* formatP );
int pnm_readpnminit( FILE* file, int* colsP, int* rowsP, xelval* maxvalP, int* formatP );
int pnm_readpnmrow( FILE* file, xel* xelrow, int cols, xelval maxval, int format );

int pnm_writepnm( FILE* file, xel** xels, int cols, int rows, xelval maxval, int format, int forceplain );
int pnm_writepnminit( FILE* file, int cols, int rows, xelval maxval, int format, int forceplain );
int pnm_writepnmrow( FILE* file, xel* xelrow, int cols, xelval maxval, int format, int forceplain );

extern xelval pnm_pbmmaxval;
/* This is the maxval used when a PNM program reads a PBM file.  Normally
** it is 1; however, for some programs, a larger value gives better results
*/


/* File open/close that handles "-" as stdin and checks errors. */

FILE* pm_openr( const char* name );
FILE* pm_openw( char* name );
int pm_closer( FILE* f );
int pm_closew( FILE* f );


/* Colormap stuff. */

typedef struct colorhist_item* colorhist_vector;
struct colorhist_item
{
  pixel color;
  int value;
};

typedef struct colorhist_list_item* colorhist_list;
struct colorhist_list_item
{
  struct colorhist_item ch;
  colorhist_list next;
};

typedef colorhist_list* colorhash_table;

colorhist_vector ppm_computecolorhist( pixel** pixels, int cols, int rows, int maxcolors, int* colorsP );
/* Returns a colorhist *colorsP long (with space allocated for maxcolors. */

void ppm_addtocolorhist( colorhist_vector chv, int* colorsP, int maxcolors, pixel* colorP, int value, int position );

void ppm_freecolorhist( colorhist_vector chv );

colorhash_table ppm_computecolorhash( pixel** pixels, int cols, int rows, int maxcolors, int* colorsP );

int
ppm_lookupcolor( colorhash_table cht, pixel* colorP );

colorhist_vector ppm_colorhashtocolorhist( colorhash_table cht, int maxcolors );
colorhash_table ppm_colorhisttocolorhash( colorhist_vector chv, int colors );

int ppm_addtocolorhash( colorhash_table cht, pixel* colorP, int value );
/* Returns -1 on failure. */
colorhash_table ppm_alloccolorhash( void );

void ppm_freecolorhash( colorhash_table cht );


#endif /*_PNMRW_H_*/

/* libpnmrw.c - PBM/PGM/PPM read/write library
**
** Copyright (C) 1988,1989,1991,1992 by Jef Poskanzer <jef@acme.com>.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

#include <string.h>

/* Definitions. */

#define pbm_allocarray( cols, rows ) ((bit**) pm_allocarray( cols, rows, sizeof(bit) ))
#define pbm_allocrow( cols ) ((bit*) pm_allocrow( cols, sizeof(bit) ))
#define pbm_freearray( bits, rows ) pm_freearray( (char**) bits, rows )
#define pbm_freerow( bitrow ) pm_freerow( (char*) bitrow )
#define pgm_allocarray( cols, rows ) ((gray**) pm_allocarray( cols, rows, sizeof(gray) ))
#define pgm_allocrow( cols ) ((gray*) pm_allocrow( cols, sizeof(gray) ))
#define pgm_freearray( grays, rows ) pm_freearray( (char**) grays, rows )
#define pgm_freerow( grayrow ) pm_freerow( (char*) grayrow )
#define ppm_allocarray( cols, rows ) ((pixel**) pm_allocarray( cols, rows, sizeof(pixel) ))
#define ppm_allocrow( cols ) ((pixel*) pm_allocrow( cols, sizeof(pixel) ))
#define ppm_freearray( pixels, rows ) pm_freearray( (char**) pixels, rows )
#define ppm_freerow( pixelrow ) pm_freerow( (char*) pixelrow )


/* Variable-sized arrays. */

char*
pm_allocrow( int cols, int size )
{
  register char* itrow;

  itrow = (char*) malloc( cols * size );
  if ( itrow == (char*) 0 )
	{
      SYSERROR("out of memory allocating a row");
      return (char*) 0;
	}
  return itrow;
}

void
pm_freerow(char * itrow )
{
  free ( itrow );
}

char**
pm_allocarray( int cols, int rows, int size )
{
  char** its;
  int i;

  its = (char**) malloc( rows * sizeof(char*) );
  if ( its == (char**) 0 )
	{
      SYSERROR("out of memory allocating an array");
      return (char**) 0;
	}
  its[0] = (char*) malloc( rows * cols * size );
  if ( its[0] == (char*) 0 )
	{
      SYSERROR("out of memory allocating an array");
      free( (char*) its );
      return (char**) 0;
	}
  for ( i = 1; i < rows; ++i )
	its[i] = &(its[0][i * cols * size]);
  return its;
}

void pm_freearray ( char** its, int  )
{
  free( its[0] );
  free( its );
}

FILE* pm_openr( const char* name )
{
  FILE* f;

  if ( strcmp( name, "-" ) == 0 )
	f = stdin;
  else
	{
#ifdef WIN32
      f = fopen( name, "rb" );
#else
      f = fopen( name, "r" );
#endif 
      if ( f == NULL )
	    {
          return (FILE*) 0;
	    }
	}
  return f;
}

FILE* pm_openw( char* name )
{
  FILE* f;

#ifdef WIN32
  f = fopen( name, "wb" );
#else 
  f = fopen( name, "w" );
#endif 
  if ( f == NULL )
	{
      return (FILE*) 0;
	}
  return f;
}

int pm_closer( FILE* f )
{
  if ( ferror( f ) )
	{
      SYSERROR("a file read error occurred at some point");
      return -1;
	}
  if ( f != stdin )
	if ( fclose( f ) != 0 )
      {
	    return -1;
      }
  return 0;
}

int pm_closew( FILE* f )
{
  fflush( f );
  if ( ferror( f ) )
	{
      SYSERROR("a file write error occurred at some point");
      return -1;
	}
  if ( f != stdout )
	if ( fclose( f ) != 0 )
      {
	    return -1;
      }
  return 0;
}

static int
pbm_getc( FILE* file )
{
  register int ich;

  ich = getc( file );
  if ( ich == EOF )
	{
      SYSERROR("EOF / read error");
      return EOF;
	}
    
  if ( ich == '#' )
	{
      do
	    {
          ich = getc( file );
          if ( ich == EOF )
            {
              SYSERROR("EOF / read error");
              return EOF;
            }
	    }
      while ( ich != '\n' && ich != '\r' );
	}

  return ich;
}

static bit
pbm_getbit(FILE*  file )
{
  register int ich;

  do
	{
      ich = pbm_getc( file );
      if ( ich == EOF )
	    return -1;
	}
  while ( ich == ' ' || ich == '\t' || ich == '\n' || ich == '\r' );

  if ( ich != '0' && ich != '1' )
	{
      SYSERROR("junk in file where bits should be");
      return -1;
	}

  return ( ich == '1' ) ? 1 : 0;
}

static int
pbm_readmagicnumber( FILE* file )
{
  int ich1, ich2;

  ich1 = getc( file );
  if ( ich1 == EOF )
	{
      SYSERROR("EOF / read error reading magic number");
      return -1;
	}
  ich2 = getc( file );
  if ( ich2 == EOF )
	{
      SYSERROR("EOF / read error reading magic number");
      return -1;
	}
  return ich1 * 256 + ich2;
}

static int
pbm_getint(FILE*  file )
{
  register int ich;
  register int i;

  do
	{
      ich = pbm_getc( file );
      if ( ich == EOF )
	    return -1;
	}
  while ( ich == ' ' || ich == '\t' || ich == '\n' || ich == '\r' );

  if ( ich < '0' || ich > '9' )
	{
      SYSERROR("junk in file where an integer should be");
      return -1;
	}

  i = 0;
  do
	{
      i = i * 10 + ich - '0';
      ich = pbm_getc( file );
      if ( ich == EOF )
	    return -1;
    }
  while ( ich >= '0' && ich <= '9' );

  return i;
}

static int
pbm_readpbminitrest(FILE*  file, int* colsP, int* rowsP )
{
  /* Read size. */
  *colsP = pbm_getint( file );
  *rowsP = pbm_getint( file );
  if ( *colsP == -1 || *rowsP == -1 )
	return -1;
  return 0;
}

static int
pbm_getrawbyte(FILE*  file )
{
  register int iby;

  iby = getc( file );
  if ( iby == EOF )
	{
      SYSERROR("EOF / read error");
      return -1;
	}
  return iby;
}

static int
pbm_readpbmrow(FILE*  file, bit* bitrow,int  cols,int  format )
{
  register int col, bitshift, b;
  register int item = 0;
  register bit* bP;

  switch ( format )
	{
	case PBM_FORMAT:
      for ( col = 0, bP = bitrow; col < cols; ++col, ++bP )
	    {
          b = pbm_getbit( file );
          if ( b == -1 )
            return -1;
          *bP = b;
	    }
      break;

	case RPBM_FORMAT:
      bitshift = -1;
      for ( col = 0, bP = bitrow; col < cols; ++col, ++bP )
	    {
          if ( bitshift == -1 )
            {
              item = pbm_getrawbyte( file );
              if ( item == -1 )
                return -1;
              bitshift = 7;
            }
          *bP = ( item >> bitshift ) & 1;
          --bitshift;
	    }
      break;

	default:
      SYSERROR("can't happen");
      return -1;
	}
  return 0;
}

static void
pbm_writepbminit( FILE*file,int  cols, int rows, int forceplain )
{
  if ( ! forceplain )
	(void) fprintf(
                   file, "%c%c\n%d %d\n", PBM_MAGIC1, RPBM_MAGIC2, cols, rows );
  else
	(void) fprintf(
                   file, "%c%c\n%d %d\n", PBM_MAGIC1, PBM_MAGIC2, cols, rows );
}

static void
pbm_writepbmrowraw( FILE* file, bit* bitrow, int cols )
{
  register int col, bitshift;
  register unsigned char item;
  register bit* bP;

  bitshift = 7;
  item = 0;
  for ( col = 0, bP = bitrow; col < cols; ++col, ++bP )
	{
      if ( *bP )
	    item += 1 << bitshift;
      --bitshift;
      if ( bitshift == -1 )
	    {
          (void) putc( item, file );
          bitshift = 7;
          item = 0;
	    }
	}
  if ( bitshift != 7 )
	(void) putc( item, file );
}

static void
pbm_writepbmrowplain( FILE* file,bit*  bitrow, int cols )
{
  register int col, charcount;
  register bit* bP;

  charcount = 0;
  for ( col = 0, bP = bitrow; col < cols; ++col, ++bP )
	{
      if ( charcount >= 70 )
	    {
          (void) putc( '\n', file );
          charcount = 0;
	    }
      putc( *bP ? '1' : '0', file );
      ++charcount;
	}
  (void) putc( '\n', file );
}

static void
pbm_writepbmrow( FILE* file, bit*  bitrow, int cols, int forceplain )
{
  if ( ! forceplain )
	pbm_writepbmrowraw( file, bitrow, cols );
  else
	pbm_writepbmrowplain( file, bitrow, cols );
}

static int
pgm_readpgminitrest( FILE* file, int* colsP, int* rowsP, gray*  maxvalP )
{
  int maxval;

  /* Read size. */
  *colsP = pbm_getint( file );
  *rowsP = pbm_getint( file );
  if ( *colsP == -1 || *rowsP == -1 )
	return -1;

  /* Read maxval. */
  maxval = pbm_getint( file );
  if ( maxval == -1 )
	return -1;
  if ( maxval > PGM_MAXMAXVAL )
	{
      SYSERROR("maxval is too large");
      return -1;
	}
  *maxvalP = maxval;
  return 0;
}

static int
pgm_readpgmrow( FILE* file, gray* grayrow, uint32_t cols, gray , int format )
{
  register uint32_t col;
  int32_t val;
  register gray* gP;

  switch ( format )
	{
	case PGM_FORMAT:
      for ( col = 0, gP = grayrow; col < cols; ++col, ++gP )
	    {
          val = pbm_getint( file );
          if ( val == -1 )
            return -1;
          *gP = val;
	    }
      break;
	
	case RPGM_FORMAT:
      if ( fread( grayrow, 1, cols, file ) != cols )
	    {
          SYSERROR("EOF / read error");
          return -1;
	    }
      break;

	default:
      SYSERROR("can't happen");
      return -1;
	}
  return 0;
}

static void
pgm_writepgminit( FILE* file, int cols, int rows, gray maxval, int forceplain )
{
  if (/* maxval <= 255 && */! forceplain )
	fprintf(
            file, "%c%c\n%d %d\n%d\n", PGM_MAGIC1, RPGM_MAGIC2,
            cols, rows, maxval );
  else
	fprintf(
            file, "%c%c\n%d %d\n%d\n", PGM_MAGIC1, PGM_MAGIC2,
            cols, rows, maxval );
}

static void
putus( unsigned short n,FILE*  file )
{
  if ( n >= 10 )
	putus( n / 10, file );
  putc( n % 10 + '0', file );
}

static int
pgm_writepgmrowraw( FILE* file, gray* grayrow, uint32_t cols, gray  )
{
  if ( fwrite( grayrow, 1, cols, file ) != cols )
	{
      SYSERROR("write error");
      return -1;
	}
  return 0;
}

static int
pgm_writepgmrowplain(  FILE* file, gray* grayrow, int cols, gray )
{
  register int col, charcount;
  register gray* gP;

  charcount = 0;
  for ( col = 0, gP = grayrow; col < cols; ++col, ++gP )
	{
      if ( charcount >= 65 )
	    {
          (void) putc( '\n', file );
          charcount = 0;
	    }
      else if ( charcount > 0 )
	    {
          (void) putc( ' ', file );
          ++charcount;
	    }
      putus( (unsigned short) *gP, file );
      charcount += 3;
	}
  if ( charcount > 0 )
	(void) putc( '\n', file );
  return 0;
}

static int
pgm_writepgmrow( FILE* file, gray* grayrow, int cols, gray maxval, int forceplain )
{
  if (/* maxval <= 255 &&*/ ! forceplain )// maxval is always < 256
	return pgm_writepgmrowraw( file, grayrow, cols, maxval );
  else
	return pgm_writepgmrowplain( file, grayrow, cols, maxval );
}

static int
ppm_readppminitrest( FILE* file, int* colsP, int* rowsP, pixval *maxvalP )
{
  int maxval;

  /* Read size. */
  *colsP = pbm_getint( file );
  *rowsP = pbm_getint( file );
  if ( *colsP == -1 || *rowsP == -1 )
	return -1;

  /* Read maxval. */
  maxval = pbm_getint( file );
  if ( maxval == -1 )
	return -1;
  if ( maxval > PPM_MAXMAXVAL )
	{
      SYSERROR("maxval is too large");
      return -1;
	}
  *maxvalP = maxval;
  return 0;
}

static int
ppm_readppmrow( FILE* file, pixel* pixelrow, uint32_t cols, pixval , int format )
{
  register uint32_t col;
  register pixel* pP;
  register int r, g, b;
  gray* grayrow;
  register gray* gP;

  switch ( format )
	{
	case PPM_FORMAT:
      for ( col = 0, pP = pixelrow; col < cols; ++col, ++pP )
	    {
          r = pbm_getint( file );
          g = pbm_getint( file );
          b = pbm_getint( file );
          if ( r == -1 || g == -1 || b == -1 )
            return -1;
          PPM_ASSIGN( *pP, r, g, b );
	    }
      break;

	case RPPM_FORMAT:
      grayrow = pgm_allocrow( 3 * cols );
      if ( grayrow == (gray*) 0 )
	    return -1;
      if ( fread( grayrow, 1, 3 * cols, file ) != 3 * cols )
	    {
          SYSERROR("EOF / read error");
          return -1;
	    }
      for ( col = 0, gP = grayrow, pP = pixelrow; col < cols; ++col, ++pP )
	    {
          r = *gP++;
          g = *gP++;
          b = *gP++;
          PPM_ASSIGN( *pP, r, g, b );
	    }
      pgm_freerow( grayrow );
      break;

	default:
      SYSERROR("can't happen");
      return -1;
	}
  return 0;
}

static void
ppm_writeppminit( FILE* file, int cols, int rows, pixval maxval, int forceplain )
{
  if ( /*maxval <= 255 &&*/ ! forceplain )
	fprintf(
            file, "%c%c\n%d %d\n%d\n", PPM_MAGIC1, RPPM_MAGIC2,
            cols, rows, maxval );
  else
	fprintf(
            file, "%c%c\n%d %d\n%d\n", PPM_MAGIC1, PPM_MAGIC2,
            cols, rows, maxval );
}

static int
ppm_writeppmrowraw(   FILE* file,
                      pixel* pixelrow,
                      uint32_t cols,
                      pixval )
{
  register uint32_t col;
  register pixel* pP;
  gray* grayrow;
  register gray* gP;

  grayrow = pgm_allocrow( 3 * cols );
  if ( grayrow == (gray*) 0 )
	return -1;
  for ( col = 0, pP = pixelrow, gP = grayrow; col < cols; ++col, ++pP )
	{
      *gP++ = PPM_GETR( *pP );
      *gP++ = PPM_GETG( *pP );
      *gP++ = PPM_GETB( *pP );
    }
  if ( fwrite( grayrow, 1, 3 * cols, file ) != 3 * cols )
	{
      SYSERROR("write error");
      return -1;
	}
  pgm_freerow( grayrow );
  return 0;
}

static int
ppm_writeppmrowplain(     FILE* file,
                          pixel* pixelrow,
                          int cols,
                          pixval )    {
  register int col, charcount;
  register pixel* pP;
  register pixval val;

  charcount = 0;
  for ( col = 0, pP = pixelrow; col < cols; ++col, ++pP )
	{
      if ( charcount >= 65 )
	    {
          (void) putc( '\n', file );
          charcount = 0;
	    }
      else if ( charcount > 0 )
	    {
          (void) putc( ' ', file );
          (void) putc( ' ', file );
          charcount += 2;
	    }
      val = PPM_GETR( *pP );
      putus( val, file );
      (void) putc( ' ', file );
      val = PPM_GETG( *pP );
      putus( val, file );
      (void) putc( ' ', file );
      val = PPM_GETB( *pP );
      putus( val, file );
      charcount += 11;
	}
  if ( charcount > 0 )
	(void) putc( '\n', file );
  return 0;
}

static int
ppm_writeppmrow( FILE* file, pixel* pixelrow, int cols, pixval maxval, int forceplain )
{
  if ( /*maxval <= 255 && */! forceplain )
	return ppm_writeppmrowraw( file, pixelrow, cols, maxval );
  else
	return ppm_writeppmrowplain( file, pixelrow, cols, maxval );
}

xelval pnm_pbmmaxval = 1;

int pnm_readpnminit( FILE* file, int* colsP, int* rowsP, xelval* maxvalP, int* formatP )
{
  gray gmaxval;

  /* Check magic number. */
  *formatP = pbm_readmagicnumber( file );
  if ( *formatP == -1 )
	return -1;
  switch ( PNM_FORMAT_TYPE(*formatP) )
	{
	case PPM_TYPE:
      if ( ppm_readppminitrest( file, colsP, rowsP, (pixval*) maxvalP ) < 0 )
	    return -1;
      break;

	case PGM_TYPE:
      if ( pgm_readpgminitrest( file, colsP, rowsP, &gmaxval ) < 0 )
	    return -1;
      *maxvalP = (xelval) gmaxval;
      break;

	case PBM_TYPE:
      if ( pbm_readpbminitrest( file, colsP, rowsP ) < 0 )
	    return -1;
      *maxvalP = pnm_pbmmaxval;
      break;

	default:
      DEBUGMSG("bad magic number - not a ppm, pgm, or pbm file");
      return -1;
	}
  return 0;
}

int
pnm_readpnmrow( FILE* file, xel* xelrow, int cols, xelval maxval, int format )
{
  register int col;
  register xel* xP;
  gray* grayrow;
  register gray* gP;
  bit* bitrow;
  register bit* bP;

  switch ( PNM_FORMAT_TYPE(format) )
	{
	case PPM_TYPE:
      if ( ppm_readppmrow( file, (pixel*) xelrow, cols, (pixval) maxval, format ) < 0 )
	    return -1;
      break;

	case PGM_TYPE:
      grayrow = pgm_allocrow( cols );
      if ( grayrow == (gray*) 0 )
	    return -1;
      if ( pgm_readpgmrow( file, grayrow, cols, (gray) maxval, format ) < 0 )
	    return -1;
      for ( col = 0, xP = xelrow, gP = grayrow; col < cols; ++col, ++xP, ++gP )
	    PNM_ASSIGN1( *xP, *gP );
      pgm_freerow( grayrow );
      break;

	case PBM_TYPE:
      bitrow = pbm_allocrow( cols );
      if ( bitrow == (bit*) 0 )
	    return -1;
      if ( pbm_readpbmrow( file, bitrow, cols, format ) < 0 )
	    {
          pbm_freerow( bitrow );
          return -1;
	    }
      for ( col = 0, xP = xelrow, bP = bitrow; col < cols; ++col, ++xP, ++bP )
	    PNM_ASSIGN1( *xP, *bP == PBM_BLACK ? 0: pnm_pbmmaxval );
      pbm_freerow( bitrow );
      break;

	default:
      SYSERROR("can't happen");
      return -1;
	}
  return 0;
}

xel**
pnm_readpnm(  FILE* file,
              int* colsP,
              int* rowsP,
              int* formatP,
              xelval* maxvalP)
{
  xel** xels;
  int row;

  if ( pnm_readpnminit( file, colsP, rowsP, maxvalP, formatP ) < 0 )
	return (xel**) 0;

  xels = pnm_allocarray( *colsP, *rowsP );
  if ( xels == (xel**) 0 )
	return (xel**) 0;

  for ( row = 0; row < *rowsP; ++row )
	if ( pnm_readpnmrow( file, xels[row], *colsP, *maxvalP, *formatP ) < 0 )
      {
	    pnm_freearray( xels, *rowsP );
	    return (xel**) 0;
      }

  return xels;
}

#if __STDC__
int
pnm_writepnminit( FILE* file, int cols, int rows, xelval maxval, int format, int forceplain )
#else /*__STDC__*/
  int
pnm_writepnminit( file, cols, rows, maxval, format, forceplain )
  FILE* file;
int cols, rows, format;
xelval maxval;
int forceplain;
#endif /*__STDC__*/
{
  switch ( PNM_FORMAT_TYPE(format) )
	{
	case PPM_TYPE:
      ppm_writeppminit( file, cols, rows, (pixval) maxval, forceplain );
      break;

	case PGM_TYPE:
      pgm_writepgminit( file, cols, rows, (gray) maxval, forceplain );
      break;

	case PBM_TYPE:
      pbm_writepbminit( file, cols, rows, forceplain );
      break;

	default:
      SYSERROR("can't happen");
      return -1;
	}
  return 0;
}

#if __STDC__
int
pnm_writepnmrow( FILE* file, xel* xelrow, int cols, xelval maxval, int format, int forceplain )
#else /*__STDC__*/
  int
pnm_writepnmrow( file, xelrow, cols, maxval, format, forceplain )
  FILE* file;
xel* xelrow;
int cols, format;
xelval maxval;
int forceplain;
#endif /*__STDC__*/
{
  register int col;
  register xel* xP;
  gray* grayrow;
  register gray* gP;
  bit* bitrow;
  register bit* bP;

  switch ( PNM_FORMAT_TYPE(format) )
	{
	case PPM_TYPE:
      if ( ppm_writeppmrow( file, (pixel*) xelrow, cols, (pixval) maxval, forceplain ) < 0 )
	    return -1;
      break;

	case PGM_TYPE:
      grayrow = pgm_allocrow( cols );
      if ( grayrow == (gray*) 0 )
	    return -1;
      for ( col = 0, gP = grayrow, xP = xelrow; col < cols; ++col, ++gP, ++xP )
	    *gP = PNM_GET1( *xP );
      if ( pgm_writepgmrow( file, grayrow, cols, (gray) maxval, forceplain ) < 0 )
	    {
          pgm_freerow( grayrow );
          return -1;
	    }
      pgm_freerow( grayrow );
      break;

	case PBM_TYPE:
      bitrow = pbm_allocrow( cols );
      if ( bitrow == (bit*) 0 )
	    return -1;
      for ( col = 0, bP = bitrow, xP = xelrow; col < cols; ++col, ++bP, ++xP )
	    *bP = PNM_GET1( *xP ) == 0 ? PBM_BLACK : PBM_WHITE;
      pbm_writepbmrow( file, bitrow, cols, forceplain );
      pbm_freerow( bitrow );
      break;

	default:
      SYSERROR("can't happen");
      return -1;
	}
  return 0;
}

#if __STDC__
int
pnm_writepnm( FILE* file, xel** xels, int cols, int rows, xelval maxval, int format, int forceplain )
#else /*__STDC__*/
  int
pnm_writepnm( file, xels, cols, rows, maxval, format, forceplain )
  FILE* file;
xel** xels;
xelval maxval;
int cols, rows, format;
int forceplain;
#endif /*__STDC__*/
{
  int row;

  if ( pnm_writepnminit( file, cols, rows, maxval, format, forceplain ) < 0 )
	return -1;

  for ( row = 0; row < rows; ++row )
	if ( pnm_writepnmrow( file, xels[row], cols, maxval, format, forceplain ) < 0 )
      return -1;
  return 0;
}


/* Colormap stuff. */

#define HASH_SIZE 20023

#define ppm_hashpixel(p) ( ( ( (long) PPM_GETR(p) * 33023 + (long) PPM_GETG(p) * 30013 + (long) PPM_GETB(p) * 27011 ) & 0x7fffffff ) % HASH_SIZE )

colorhist_vector
ppm_computecolorhist(  pixel** pixels,
                       int cols, int rows, int maxcolors,
                       int* colorsP)
{
  colorhash_table cht;
  colorhist_vector chv;

  cht = ppm_computecolorhash( pixels, cols, rows, maxcolors, colorsP );
  if ( cht == (colorhash_table) 0 )
	return (colorhist_vector) 0;
  chv = ppm_colorhashtocolorhist( cht, maxcolors );
  ppm_freecolorhash( cht );
  return chv;
}

void
ppm_addtocolorhist(colorhist_vector chv,
                   pixel* colorP,
                   int* colorsP,
                   int maxcolors, int value, int position)
{
  int i, j;

  /* Search colorhist for the color. */
  for ( i = 0; i < *colorsP; ++i )
	if ( PPM_EQUAL( chv[i].color, *colorP ) )
      {
	    /* Found it - move to new slot. */
	    if ( position > i )
          {
            for ( j = i; j < position; ++j )
              chv[j] = chv[j + 1];
          }
	    else if ( position < i )
          {
            for ( j = i; j > position; --j )
              chv[j] = chv[j - 1];
          }
	    chv[position].color = *colorP;
	    chv[position].value = value;
	    return;
      }
  if ( *colorsP < maxcolors )
	{
      /* Didn't find it, but there's room to add it; so do so. */
      for ( i = *colorsP; i > position; --i )
	    chv[i] = chv[i - 1];
      chv[position].color = *colorP;
      chv[position].value = value;
      ++(*colorsP);
	}
}

colorhash_table ppm_computecolorhash( pixel** pixels, int cols, int rows, int maxcolors, int* colorsP )
{
  colorhash_table cht;
  register pixel* pP;
  colorhist_list chl;
  int col, row, hash;

  cht = ppm_alloccolorhash( );
  if ( cht == (colorhash_table) 0 )
	return (colorhash_table) 0;
  *colorsP = 0;

  /* Go through the entire image, building a hash table of colors. */
  for ( row = 0; row < rows; ++row )
	for ( col = 0, pP = pixels[row]; col < cols; ++col, ++pP )
      {
	    hash = ppm_hashpixel( *pP );
	    for ( chl = cht[hash]; chl != (colorhist_list) 0; chl = chl->next )
          if ( PPM_EQUAL( chl->ch.color, *pP ) )
		    break;
	    if ( chl != (colorhist_list) 0 )
          ++(chl->ch.value);
	    else
          {
            if ( ++(*colorsP) > maxcolors )
              {
                ppm_freecolorhash( cht );
                return (colorhash_table) 0;
              }
            chl = (colorhist_list) malloc( sizeof(struct colorhist_list_item) );
            if ( chl == 0 )
              {
                SYSERROR("out of memory computing hash table");
                ppm_freecolorhash( cht );
                return (colorhash_table) 0;
              }
            chl->ch.color = *pP;
            chl->ch.value = 1;
            chl->next = cht[hash];
            cht[hash] = chl;
          }
      }
    
  return cht;
}

colorhash_table
ppm_alloccolorhash( )
{
  colorhash_table cht;
  int i;

  cht = (colorhash_table) malloc( HASH_SIZE * sizeof(colorhist_list) );
  if ( cht == 0 )
	{
      SYSERROR("out of memory allocating hash table");
      return (colorhash_table) 0;
	}

  for ( i = 0; i < HASH_SIZE; ++i )
	cht[i] = (colorhist_list) 0;

  return cht;
}

int ppm_addtocolorhash( colorhash_table cht, pixel* colorP, int value )
{
  register int hash;
  register colorhist_list chl;

  chl = (colorhist_list) malloc( sizeof(struct colorhist_list_item) );
  if ( chl == 0 )
	return -1;
  hash = ppm_hashpixel( *colorP );
  chl->ch.color = *colorP;
  chl->ch.value = value;
  chl->next = cht[hash];
  cht[hash] = chl;
  return 0;
}

colorhist_vector ppm_colorhashtocolorhist( colorhash_table cht, int maxcolors )    {
  colorhist_vector chv;
  colorhist_list chl;
  int i, j;

  /* Now collate the hash table into a simple colorhist array. */
  chv = (colorhist_vector) malloc( maxcolors * sizeof(struct colorhist_item) );
  /* (Leave room for expansion by caller.) */
  if ( chv == (colorhist_vector) 0 )
	{
      SYSERROR("out of memory generating histogram");
      return (colorhist_vector) 0;
	}

  /* Loop through the hash table. */
  j = 0;
  for ( i = 0; i < HASH_SIZE; ++i )
	for ( chl = cht[i]; chl != (colorhist_list) 0; chl = chl->next )
      {
	    /* Add the new entry. */
	    chv[j] = chl->ch;
	    ++j;
      }

  /* All done. */
  return chv;
}

colorhash_table ppm_colorhisttocolorhash( colorhist_vector chv, int colors )
{
  colorhash_table cht;
  int i, hash;
  pixel color;
  colorhist_list chl;

  cht = ppm_alloccolorhash( );
  if ( cht == (colorhash_table) 0 )
	return (colorhash_table) 0;

  for ( i = 0; i < colors; ++i )
	{
      color = chv[i].color;
      hash = ppm_hashpixel( color );
      for ( chl = cht[hash]; chl != (colorhist_list) 0; chl = chl->next )
	    if ( PPM_EQUAL( chl->ch.color, color ) )
          {
            SYSERROR("same color found twice - %d %d %d",
                     PPM_GETR(color), PPM_GETG(color), PPM_GETB(color) );
            ppm_freecolorhash( cht );
            return (colorhash_table) 0;
          }
      chl = (colorhist_list) malloc( sizeof(struct colorhist_list_item) );
      if ( chl == (colorhist_list) 0 )
	    {
          SYSERROR("out of memory");
          ppm_freecolorhash( cht );
          return (colorhash_table) 0;
	    }
      chl->ch.color = color;
      chl->ch.value = i;
      chl->next = cht[hash];
      cht[hash] = chl;
	}

  return cht;
}

int
ppm_lookupcolor( colorhash_table cht, pixel* colorP )
{
  int hash;
  colorhist_list chl;

  hash = ppm_hashpixel( *colorP );
  for ( chl = cht[hash]; chl != (colorhist_list) 0; chl = chl->next )
	if ( PPM_EQUAL( chl->ch.color, *colorP ) )
      return chl->ch.value;

  return -1;
}

void ppm_freecolorhist( colorhist_vector chv )
{
  free( (char*) chv );
}

void
ppm_freecolorhash(  colorhash_table cht)
{
  int i;
  colorhist_list chl, chlnext;

  for ( i = 0; i < HASH_SIZE; ++i )
	for ( chl = cht[i]; chl != (colorhist_list) 0; chl = chlnext )
      {
	    chlnext = chl->next;
	    free( (char*) chl );
      }
  free( (char*) cht );
}

/******************************************************************************************/
/* Start of Blockbuster integration code. */

/* Load the desired subimage into a set of RGB bytes */
int PNMFrameInfo::LoadImage(ImagePtr image,
                        ImageFormat *requiredImageFormat, 
                        const Rectangle *, int levelOfDetail)
{
  FILE *f;
  register int i, j, k;
  unsigned char *p;
  xel* xrow;
  xel* xp;
  uint8_t	rp,gp,bp;
  int width, height, depth, fmt;
  xelval value;
  int extraBytesPerPixel = 0;
  int scanlineBytes, rowOrder, byteOrder;

  /* Calculate how much image data we need. */
  extraBytesPerPixel = requiredImageFormat->bytesPerPixel - 3;
  scanlineBytes = ROUND_UP_TO_MULTIPLE(
                                    requiredImageFormat->bytesPerPixel * mWidth,
                                    requiredImageFormat->scanlineByteMultiple
                                    );
  if (requiredImageFormat->rowOrder == ROW_ORDER_DONT_CARE)
    rowOrder = BOTTOM_TO_TOP; /* Bias toward OpenGL */
  else
    rowOrder = requiredImageFormat->rowOrder;
  byteOrder = requiredImageFormat->byteOrder;

  if (!image->allocate(mHeight * scanlineBytes)) {
    ERROR("could not allocate %dx%dx%d image data ",
          mWidth, mHeight, mDepth);
    return 0;
  }
  
  image->width = mWidth;
  image->height = mHeight;
  image->imageFormat.bytesPerPixel = requiredImageFormat->bytesPerPixel;
  image->imageFormat.scanlineByteMultiple = requiredImageFormat->scanlineByteMultiple;
  image->imageFormat.byteOrder = byteOrder;
  image->imageFormat.rowOrder = rowOrder;
  image->levelOfDetail = levelOfDetail;
  

  /* We're really dumb.  We only know how to return our
   * specific format (RGB, 3 bytes per pixel).  We'll rely on the
   * conversion module to handle anything different.
   */
  f = pm_openr(mFilename.c_str());
  if (f == NULL) {
	WARNING("cannot open file %s", mFilename.c_str());
	return 0;
  }
  if (pnm_readpnminit(f, &width,&height,&value,&fmt) == -1) {
	SYSERROR("%s is not a PNM file", mFilename.c_str());
	pm_closer(f);
	return 0;
  }
  switch(PNM_FORMAT_TYPE(fmt)) {
  case PBM_TYPE:
    WARNING("The file '%s' is a PNM bitmap file, and not supported.",
            mFilename.c_str());
    return 0;
    break;
  case PPM_TYPE:
    depth = 3;
    break;
  default:
    depth = 1;
    break;
  }
  if (mWidth != (uint32_t)width ||
      mHeight != (uint32_t)height ||
      mDepth != 8*(uint32_t)depth) {
	ERROR("PNM file %s is %dx%dx%d, expected %dx%dx%d",
          mFilename.c_str(),
          width, height, 8*depth,
          mWidth, mHeight, mDepth);
	pm_closer(f);
	return 0;
  }

#define NORM(x,mx) static_cast<uint8_t>(((float)(x)/(float)(mx))*255.0)

  xrow = pnm_allocrow( width );
  for(i=0;i<height;i++) {
	p = (unsigned char *) image->Data() + (rowOrder == TOP_TO_BOTTOM?i:height - i - 1)*scanlineBytes;
	pnm_readpnmrow( f, xrow, width, value, fmt );
	if (depth == 3) {
      xp = xrow;
      for(j=0;j<width;j++) {
		rp = NORM(PPM_GETR(*xp),value);
		gp = NORM(PPM_GETG(*xp),value);
		bp = NORM(PPM_GETB(*xp),value);
		xp++;
		if (byteOrder == MSB_FIRST) {
          *p++ = rp;
          *p++ = gp;
          *p++ = bp;
		}
		else {
          *p++ = bp;
          *p++ = gp;
          *p++ = rp;
		}
		for (k = 0; k < extraBytesPerPixel; k++) {
          *p++ = 0xff; /* add alpha, other bytes */
		}
      }
	} else {
      xp = xrow;
      for(j=0;j<width;j++) {
		rp = NORM(PNM_GET1(*xp),value);
		xp++;
		/* Byte order doesn't matter in grayscale */
		*p++ = rp;
		*p++ = rp;
		*p++ = rp;
		for (k = 0; k < extraBytesPerPixel; k++) {
          *p++ = 0xff; /* add alpha, other bytes */
		}
      }
	}
  }
  pnm_freerow(xrow);
  pm_closer(f);

  image->loadedRegion.x = 0;
  image->loadedRegion.y = 0;
  image->loadedRegion.height = height;
  image->loadedRegion.width = width;

  return 1; /* success */
}

//============================================================

PNMFrameInfo::PNMFrameInfo(string fname): FrameInfo(fname) {
  
  FILE *f;
  int 	pnmwidth, pnmheight, pnmfmt;
  xelval	lvalue;
  f = pm_openr(mFilename.c_str());
  if (!f) {
    WARNING("Cannot open the file '%s'", mFilename.c_str());
    return ;
  }
  if (pnm_readpnminit(f, &pnmwidth,&pnmheight,&lvalue,&pnmfmt) == -1) {
    DEBUGMSG("The file '%s' is not in PNM format.", mFilename.c_str());
    pm_closer(f);
    return ;
  }
  pm_closer(f);

  /* set up our params */
  switch(PNM_FORMAT_TYPE(pnmfmt)) {
  case PBM_TYPE:
    WARNING("The file '%s' is a PNM bitmap file, and not supported.",
            mFilename.c_str());
    return ;
    break;
  case PPM_TYPE:
    mDepth = 24;
    break;
  default:
    mDepth = 8;
    break;
  }
  mWidth = pnmwidth; 
  mHeight = pnmheight; 

  DEBUGMSG("The file '%s' is a valid PNM file.", mFilename.c_str());
  mValid = true; 
  return; 
}

//============================================================

FrameListPtr pnmGetFrameList(const char *filename)
{
  FrameListPtr frameList; 


  /* Prepare the FrameList and FrameInfo structures we are to
   * return to the user.  Since a PNG file stores a single 
   * frame, we need only one frameInfo, and the frameList
   * need be large enough only for 2 entries (the information
   * about the single frame, and the terminating NULL).
   */
  FrameInfoPtr frameInfo(new PNMFrameInfo(filename)); 
  if (!frameInfo) {
    ERROR("cannot allocate FrameInfo structure");
    return frameList;
  }
  if (!frameInfo->mValid) {
    return frameList; 
  }

  frameList.reset(new FrameList); 
  if (!frameList) {
    ERROR("cannot allocate FrameInfo list structure");
    return frameList;
  }

  /* Fill out the final return form, and call it a day */
  frameList->append(frameInfo);
  frameList->mTargetFPS = 0.0;

  frameList->mFormatName = "PNM"; 
  frameList->mFormatDescription = "Single-frame image in a PNM (PBM/PGM/PPM) file"; 
  return frameList;
}

