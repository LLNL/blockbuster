/*
** $RCSfile: smJPG.C,v $
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
** $Id: smJPG.C,v 1.6 2009/05/19 02:52:19 wealthychef Exp $
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
// smJPG.C
//
//
//
//

#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "smJPG.h"

#include "setjmp.h"



typedef struct {               // derived from default jdatadst.c data manager
	struct jpeg_destination_mgr pub; // public fields
	char *buf;             // output buffer
	int  size;
	int  final;
} jpgdst_struct;

typedef struct {               // decode error object
	struct jpeg_error_mgr pub; // "public" fields
	jmp_buf setjmp_buffer; // for return to caller
} jpgerr_struct;

typedef struct {               // derived from default jdatasrc.c data manager
        struct jpeg_source_mgr pub; // public fields
        char *buf;             // input buffer
        int size;              // size
	char eoibuf[2];        // fake EOI chars for truncated files
} jpgsrc_struct;

/* callback protos */
static void jpg_term_source(j_decompress_ptr cinfo);
static void jpg_skip_input_data(j_decompress_ptr cinfo,long num_bytes);
static boolean jpg_fill_input_buffer(j_decompress_ptr cinfo);
static void jpg_init_source(j_decompress_ptr cinfo);
static void jpg_error_exit(j_common_ptr cinfo);
static void jpg_init_destination(j_compress_ptr cinfo);
static boolean jpg_empty_output_buffer(j_compress_ptr cinfo);
static void jpg_term_destination(j_compress_ptr cinfo);



smJPG::smJPG(int mode, const char *_fname, int _nwin)
  :smBase(mode, _fname, _nwin)
{
  mTypeID = 4;
  setQuality(75);
  if (getenv("JPG_COMPRESSION_QUALITY")) {
    int tmp;
    sscanf(getenv("JPG_COMPRESSION_QUALITY"),"%d",&tmp);
    setQuality(tmp);
  }
}

smJPG::~smJPG()
{
}

void smJPG::setQuality(int qual)
{
   jpg_quality = qual;
   if (jpg_quality > 100) jpg_quality = 100;
   if (jpg_quality < 0) jpg_quality = 0;
}

bool smJPG::decompBlock(u_char *cdata,u_char *image,int size,int *dim)
{
   jpgerr_struct jerr_st;
   struct jpeg_decompress_struct cinfo;
   jpgsrc_struct jsrc_st;

   jpeg_create_decompress(&cinfo);
   cinfo.src=(struct jpeg_source_mgr *)(&jsrc_st);

   // setup source manager
   jsrc_st.pub.init_source=jpg_init_source;
   jsrc_st.pub.fill_input_buffer=jpg_fill_input_buffer;
   jsrc_st.pub.skip_input_data=jpg_skip_input_data;
   jsrc_st.pub.resync_to_restart=jpeg_resync_to_restart; // use default
   jsrc_st.pub.term_source=jpg_term_source;
   jsrc_st.buf=(char *)cdata; jsrc_st.size=size;
   jsrc_st.pub.bytes_in_buffer=jsrc_st.size;
   jsrc_st.pub.next_input_byte=(JOCTET *)jsrc_st.buf;
   jsrc_st.eoibuf[0]=(JOCTET)0xFF;
   jsrc_st.eoibuf[1]=(JOCTET)JPEG_EOI;
 
   // setup error handling
   cinfo.err=jpeg_std_error(&(jerr_st.pub));
   jerr_st.pub.error_exit=jpg_error_exit;
   if (setjmp(jerr_st.setjmp_buffer)) {
   	jpeg_destroy_decompress(&cinfo);
	return false;
   }

   // grab header and startup decompress
   jpeg_read_header(&cinfo,TRUE);
   jpeg_start_decompress(&cinfo);
   
    JSAMPLE	*row = (JSAMPLE *)image;
    while(cinfo.output_scanline<cinfo.output_height) {
        jpeg_read_scanlines(&cinfo,&row,1);
        row += dim[0]*3;
    }
   
   // close down decompress
   jpeg_finish_decompress(&cinfo);
   jpeg_destroy_decompress(&cinfo);

   return true;
}

/*smJPG *smJPG::newFile(const char *_fname, 
                      u_int _width, u_int _height, 
                      u_int _nframes, u_int *_tile, u_int _nres, 
                      int numthreads)
{
   smJPG *r = new smJPG(NULL);

   if (r->smBase::newFile(_fname, _width, _height, _nframes, _tile, _nres, numthreads)) {
	delete r;
	r = NULL;
   }
 
   return(r);
   }*/

void smJPG::compBlock(void *data, void *cdata, int &size,int *dim)
{
    if (!cdata) {
      size = (int)(dim[0]*dim[1]*3*1.5);
      return;
    }

    jpgdst_struct jdst_st;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    // setup compression objects, parameters
    {
       // standard compression setup
       cinfo.err=jpeg_std_error(&jerr);
       jpeg_create_compress(&cinfo);
       // special destination handlers
       jdst_st.pub.init_destination = jpg_init_destination;
       jdst_st.pub.empty_output_buffer = jpg_empty_output_buffer;
       jdst_st.pub.term_destination = jpg_term_destination;
       jdst_st.buf = (char*)cdata;
       jdst_st.size = (int)(dim[0]*dim[1]*3*1.1);
       jdst_st.final = 0;

       cinfo.dest=(struct jpeg_destination_mgr *)(&jdst_st);
       // compression parameters
       cinfo.image_width=dim[0];
       cinfo.image_height=dim[1];
       cinfo.input_components=3;
       cinfo.in_color_space=JCS_RGB;
       cinfo.data_precision=8;
       jpeg_set_defaults(&cinfo);
       jpeg_set_quality(&cinfo,jpg_quality,TRUE);
   }
   
   JSAMPLE	*row = (JSAMPLE *)data;
   jpeg_start_compress(&cinfo, TRUE);
   while(cinfo.next_scanline<cinfo.image_height) {
       jpeg_write_scanlines(&cinfo,&row,1);
       //row += getWidth()*3;  
       row += cinfo.image_width * 3;
   }
   jpeg_finish_compress(&cinfo);

   size = jdst_st.final;

   // free up temps
   jpeg_destroy_compress(&cinfo);

   return;
}

/* callbacks */
static void jpg_error_exit(j_common_ptr cinfo)
{
    jpgerr_struct *jerr;

    jerr=(jpgerr_struct *)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(jerr->setjmp_buffer,1);
}

static void jpg_init_source(j_decompress_ptr cinfo)
{
    jpgsrc_struct *jsrc;

    jsrc=(jpgsrc_struct *)cinfo->src;
    jsrc->pub.bytes_in_buffer=jsrc->size;
    jsrc->pub.next_input_byte=(JOCTET *)jsrc->buf;
}

static boolean jpg_fill_input_buffer(j_decompress_ptr cinfo)
{
    jpgsrc_struct *jsrc;

    jsrc=(jpgsrc_struct *)cinfo->src;

    // shouldn't get here--so use code similar to jdatasrc.c does on error
    WARNMS(cinfo,JWRN_JPEG_EOF);
    jsrc->pub.next_input_byte=(const JOCTET *)jsrc->eoibuf;
    jsrc->pub.bytes_in_buffer=2;

    return TRUE;
}

static void jpg_skip_input_data(j_decompress_ptr cinfo,long num_bytes)
{
    jpgsrc_struct *jsrc;

    jsrc=(jpgsrc_struct *)cinfo->src;
    //...more: should check for overflow
    jsrc->pub.next_input_byte+=(size_t)num_bytes;
    jsrc->pub.bytes_in_buffer-=(size_t)num_bytes;
}

static void jpg_term_source(j_decompress_ptr cinfo)
{
    // do nothing
}

static void jpg_init_destination(j_compress_ptr cinfo)
{
    jpgdst_struct *jdst;

    jdst=(jpgdst_struct *)cinfo->dest;
    jdst->pub.free_in_buffer=jdst->size;
    jdst->pub.next_output_byte=(JOCTET *)jdst->buf;
    jdst->final = 0;
}

static boolean jpg_empty_output_buffer(j_compress_ptr cinfo)
{
    jpgdst_struct *jdst;

    jdst=(jpgdst_struct *)cinfo->dest; 
    jdst->pub.free_in_buffer=jdst->size;
    jdst->pub.next_output_byte=(JOCTET *)jdst->buf;
  smdbprintf(0,"Warning: jpeg output buffer overflow...\n");
    jdst->final = -1;
    return TRUE;
}

static void jpg_term_destination(j_compress_ptr cinfo)
{
    jpgdst_struct *jdst;

    jdst=(jpgdst_struct *)cinfo->dest; 
    // record length...
    if (jdst->final == 0) {
    	jdst->final = jdst->size - jdst->pub.free_in_buffer;
    }
}

