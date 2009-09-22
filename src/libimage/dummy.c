#ifndef __flclient_h_
#define __flclient_h_

#include <stdio.h>
#include <stdlib.h>

#ifndef __gl_h_
#include <GL/gl.h>
#endif /* __gl_h_ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* This file should be included in those OpenGL programs which need Font  */
/* Library (FL) functions from the file /usr/lib/libFL.so.                */

/* The fontNamePreference parameter in a call to flCreateContext can have  */
/* one of the following values:                                            */
#define FL_FONTNAME     0
#define FL_FILENAME     1
#define FL_XFONTNAME    2

/* The parameter hint in a call to flSetHint can have one of the following */
/* values:                                                                 */
#define FL_HINT_AABITMAPFONTS   1      /* bound to font                    */
#define FL_HINT_CHARSPACING     2      /* bound to font                    */
#define FL_HINT_FONTTYPE        3      /* bound to font                    */
#define FL_HINT_MAXAASIZE       4      /* bound to font                    */
#define FL_HINT_MINOUTLINESIZE  5      /* bound to font                    */
#define FL_HINT_ROUNDADVANCE    6      /* bound to font                    */
#define FL_HINT_SCALETHRESH     7      /* bound to font                    */
#define FL_HINT_TOLERANCE       8      /* bound to font                    */

#define FL_FONTTYPE_ALL         0      /* use all types of fonts (default) */
#define FL_FONTTYPE_BITMAP      1      /* use only bitmap fonts            */
#define FL_FONTTYPE_OUTLINE     2      /* use only outline fonts           */

#define FL_ASCII           0
#define FL_ADOBE           1
#define FL_JISC6226        2
#define FL_CYRILLIC        3
#define FL_HANGUL          4
#define FL_DEVENAGERI      5
#define FL_ISO88591        6     /* ISO 8859-1 */
#define FL_DECTECH         7     /* DEC DECTECH */
#define FL_JISX020819760   8     /* JISX0208.1976-0 */
#define FL_JISX020119760   9     /* JISX0201.1976-0 */
#define FL_SUNOLCURSOR1   10     /* SUN OPEN LOOK CURSOR 1 */
#define FL_SUNOLGLYPH1    11     /* SUN OPEN LOOK GLYPH 1 */
#define FL_SPECIFIC       12     /* FONT SPECIFIC */
#define FL_JISX020819830  13     /* JISX0208.1983-0 */
#define FL_KSC560119870   14     /* KSC5601.1987-0 */
#define FL_GB231219800    15     /* GB2312.1980-0 */
#define FL_78EUCH         16     /* CID 78-EUC-H */
#define FL_78H            17     /* CID 78-H */
#define FL_78RKSJH        18     /* CID 78-RKSJ-H */
#define FL_83PVRKSJH      19     /* CID 83pv-RKSJ-H */
#define FL_90MSRKSJH      20     /* CID 90ms-RKSJ-H */
#define FL_90PVRKSJH      21     /* CID 90pv-RKSJ-H */
#define FL_ADDH           22     /* CID Add-H */
#define FL_ADDRKSJH       23     /* CID Add-RKSJ-H */
#define FL_ADOBEJAPAN10   24     /* CID Adobe-Japan1-0 */
#define FL_ADOBEJAPAN11   25     /* CID Adobe-Japan1-1 */
#define FL_ADOBEJAPAN12   26     /* CID Adobe-Japan1-2 */
#define FL_EUCH           27     /* CID EUC-H */
#define FL_EXTH           28     /* CID Ext-H */
#define FL_EXTRKSJH       29     /* CID Ext-RKSJ-H */
#define FL_HIRAGANA       30     /* CID Hiragana */
#define FL_KATAKANA       31     /* CID Katakana */
#define FL_NWPH           32     /* CID NWP-H */
#define FL_RKSJH          33     /* CID RKSJ-H */
#define FL_ROMAN          34     /* CID Roman */
#define FL_WPSYMBOL       35     /* CID WP-Symbol */
#define FL_ADOBEJAPAN20   36     /* CID Adobe-Japan2-0 */
#define FL_HOJOH          37     /* CID Hojo-H */
#define FL_ISO88592       38     /* ISO 8859-2 */
#define FL_ISO88593       39     /* ISO 8859-3 */
#define FL_ISO88594       40     /* ISO 8859-4 */
#define FL_ISO88595       41     /* ISO 8859-5 */
#define FL_ISO88596       42     /* ISO 8859-6 */
#define FL_ISO88597       43     /* ISO 8859-7 */
#define FL_ISO88598       44     /* ISO 8859-8 */
#define FL_ISO88599       45     /* ISO 8859-9 */
#define FL_ISO885910      46     /* ISO 8859-10 */
#define FL_BIG5           47
#define FL_CNS1164319861  48     /* CNS11643.1986-1 */
#define FL_CNS1164319862  49     /* CNS11643.1986-2 */
#define FL_EUCCN          50 
#define FL_EUCJP          51 
#define FL_EUCKR          52
#define FL_EUCTW          53
#define FL_SJIS           54

/* font direction */
#define FL_FONT_LEFTTORIGHT         0
#define FL_FONT_RIGHTTOLEFT         1
#define FL_FONT_BOTTOMTOTOP         2
#define FL_FONT_TOPTOBOTTOM         3

typedef GLint FLfontNumber;

/* character outline data structures */
typedef struct {
    GLfloat        x;
    GLfloat        y;
} FLpt2;

typedef struct FLoutline {
    GLshort        outlinecount;
    GLshort        *vertexcount;
    FLpt2          **vertex;
    GLfloat        xadvance;
    GLfloat        yadvance;
} FLoutline;

/* FLcontext is a pointer to private (opaque) data */
typedef struct __FLcontextRec *FLcontext;

typedef struct FLbitmap {
    GLsizei width;
    GLsizei height;
    GLfloat xorig;
    GLfloat yorig;
    GLfloat xmove;
    GLfloat ymove;
    GLubyte *bitmap;
} FLbitmap;

typedef struct FLscalableBitmap { /* font metrics in thousandths of an em */ 
    GLsizei ymove;  /* in thousandths of an em */
    GLsizei xmove;  /* in thousandths of an em */
    GLsizei llx;    /* in thousandths of an em */
    GLsizei lly;    /* in thousandths of an em */
    GLsizei urx;    /* in thousandths of an em */
    GLsizei ury;    /* in thousandths of an em */
    GLsizei width;  /* bitmap width in pixels */
    GLsizei height; /* bitmap height in pixels */
    GLfloat xorig;  /* x coordinate for the bitmap origin in pixels */ 
    GLfloat yorig;  /* y coordinate for the bitmap origin in pixels */ 
    GLubyte *bitmap;
} FLscalableBitmap;

typedef struct FLFontProp {
    unsigned long name;
    unsigned long value;
} FLFontProp;

typedef struct {
    short       lbearing;       /* origin to left edge of raster */
    short       rbearing;       /* origin to right edge of raster */
    short       width;          /* advance to next char's origin */
    short       ascent;         /* baseline to top edge of raster */
    short       descent;        /* baseline to bottom edge of raster */
    unsigned short attributes;  /* per char flags (not predefined) */
} FLCharStruct;

typedef struct FLfontStruct {
    FLfontNumber       fn;      /* font number (handle) */
    unsigned    direction;      /* hint about direction the font is painted */
    unsigned    min_char_or_byte2;/* first character */
    unsigned    max_char_or_byte2;/* last character */
    unsigned    min_byte1;      /* first row that exists */
    unsigned    max_byte1;      /* last row that exists */
    int         all_chars_exist;/* flag if all characters have non-zero size*/
    unsigned    default_char;   /* char to print for undefined character */
    int         n_properties;   /* how many properties there are */
    FLFontProp  *properties;    /* pointer to array of additional properties*/
    FLCharStruct min_bounds;    /* minimum bounds over all existing char*/
    FLCharStruct max_bounds;    /* maximum bounds over all existing char*/
    FLCharStruct *per_char;     /* first_char to last_char information */
    int         ascent;         /* log. extent above baseline for spacing */
    int         descent;        /* log. descent below baseline for spacing */
} FLfontStruct;


/* functions provided (exported) by the OpenGL Font Library (FL) */
void flAAColor(
    GLuint                         colorFull, 
    GLuint                         colorHalf 
) {}

void flAACpack(
    GLuint                         colorFull, 
    GLuint                         colorHalf 
) {}

void flCpack(
    GLuint                         color 
) {}

FLcontext flCreateContext(
    const GLubyte *                fontPath ,
    GLint                          fontNamePreference ,
    const GLubyte *                fontNameRestriction ,
    GLfloat                        pointsPerUMx,
    GLfloat                        pointsPerUMy 
) {return NULL;}

FLfontNumber flCreateFont(
    const GLubyte *                 fontName ,
    GLfloat mat[2][2]                  , 
    GLint                           charNameCount ,
    GLubyte **                      charNameVector 
) {return 0;}

void flDestroyContext(
    FLcontext                       ctx 
) {}

void flDestroyFont(
    FLfontNumber                    fn 
) {}

void flDrawCharacters(
    GLubyte *                       str 
) {}

void flDrawNCharacters(
    void *                          str , 
    GLint                           charCount ,
    GLint                           bytesPerCharacter 
) {}

void flEnumerateFonts(
    void (*fn)(GLubyte *)           
) {}

void flEnumerateSizes(
    GLubyte *                       typeface ,
    void (*fn)(GLfloat)             
) {}

void flFreeBitmap(
    FLbitmap *                      bitmapPtr 
) {}

void flFreeScalableBitmap(
    FLscalableBitmap *              bitmapPtr 
) {}

void flFreeFontInfo(
    FLfontStruct *                  fontStruct 
) {}

void flFreeFontNames(
    GLubyte **                      list 
) {}

void flFreeFontSizes(
    GLfloat *                       list 
) {}

void flFreeOutline(
    FLoutline *                      outline 
) {}

GLboolean flGetOutlineBBox(
    FLfontNumber                    fn ,
    GLuint                          c , 
    GLfloat *                       llx , 
    GLfloat *                       lly ,
    GLfloat *                       urx , 
    GLfloat *                       ury 
) {return 0;}

FLbitmap *flGetBitmap(
    FLfontNumber                    fn , 
    GLuint                          c 
) {return NULL;}

FLscalableBitmap *flGetScalableBitmap(
    FLfontNumber                    fn ,
    GLuint                          c 
) {return NULL;}

FLcontext flGetCurrentContext(
    void
) {return NULL;}

GLboolean flMakeCurrentContext(
    FLcontext                       ctx 
) {return 0;}

FLfontNumber flGetCurrentFont(
    void
) {return 0;}

FLfontStruct *flGetFontInfo(
    FLfontNumber                    fn 
) {return NULL;}

FLoutline *flGetOutline(
    FLfontNumber                    fn , 
    GLuint                          c 
) {return NULL;}

GLboolean  flGetStringWidth(
    FLfontNumber                    fn ,
    GLubyte *                       str , 
    GLfloat *                       dx  , 
    GLfloat *                       dy 
) {return 0; }

GLboolean flGetStringWidthN(
    FLfontNumber                    fn ,
    void *                          str , 
    GLint                           charCount ,
    GLint                           bytesPerCharacter , 
    GLfloat *                       dx , 
    GLfloat *                       dy 
) {return 0; }

GLubyte **flListFonts(
    GLint                           maxNames , 
    GLint *                         countReturn 
) {return NULL;}

GLfloat *flListSizes(
    const GLubyte *                 typeface , 
    GLint *countReturn
) {return NULL;}

GLboolean flMakeCurrentFont(
    FLfontNumber                    fn 
) {return 0; }

FLfontNumber flScaleRotateFont(
    const GLubyte *                 fontName ,
    GLfloat                         fontScale , 
    GLfloat                         angle 
) {return 0; }

void flSetHint(
    GLuint                          hint , 
    GLfloat                         hintValue 
) {}

void flGetFontBBox(
    FLfontNumber                    fn ,
    GLfloat *                       llx ,
    GLfloat *                       lly ,
    GLfloat *                       urx ,
    GLfloat *                       ury ,
    GLint *                         isfixed 
) {}

FLbitmap *flUniGetBitmap(
    GLubyte *                       fontList , 
    GLubyte *                       UCS2 
) {return NULL;}

FLoutline *flUniGetOutline(
    GLubyte *                       fontList , 
    GLubyte *                       UCS2 
) {return NULL;}

GLboolean flUniGetOutlineBBox(
    GLubyte *                       fontList , 
    GLubyte *                       UCS2 ,
    GLfloat *                       llx ,
    GLfloat *                       lly ,
    GLfloat *                       urx ,
    GLfloat *                       ury 
) {return 0;}

FLscalableBitmap *flUniGetScalableBitmap(
    GLubyte *                       fontList , 
    GLubyte *                       UCS2 
) {return NULL;}

/* Include dummy functions for iconv as well under OSX */
#ifdef osx
void libiconv() {};
void libiconv_open() {};

int iconv_close (int cd)
{
	return(0);
}
int iconv_open(const char *to,const char *from)
{
	return(1);
}
size_t iconv(int cd, const char **inbuf, size_t *inbytesleft,
                  char **outbuf, size_t *outbytesleft)
{
	while(*inbytesleft && *outbytesleft) {
		**outbuf = **inbuf;
		*outbuf++; *inbuf++;
		*inbytesleft--;
		*outbytesleft--;
        }
	return(0);
}

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __flclient_h_ */
