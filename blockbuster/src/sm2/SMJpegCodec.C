#include "SMJpegCodec.h"
extern "C" {
#include "jpeglib.h"
#include "jerror.h"
} 

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


bool SMJpegCodec::Decompress(unsigned char *in, uint32_t inputSize, std::vector<unsigned char> &out, uint32_t blockDims[2]) 
// u_char *cdata,u_char *image,int size,int *dim)
{
   jpgerr_struct jerr_st;
   struct jpeg_decompress_struct cinfo;
   jpgsrc_struct jsrc_st;

   // setup error handling
   cinfo.err=jpeg_std_error(&(jerr_st.pub));
   jerr_st.pub.error_exit=jpg_error_exit;
   if (setjmp(jerr_st.setjmp_buffer)) {
   	jpeg_destroy_decompress(&cinfo);
	return false;
   }

   jpeg_create_decompress(&cinfo);
   cinfo.src=(struct jpeg_source_mgr *)(&jsrc_st);

   // setup source manager
   jsrc_st.pub.init_source=jpg_init_source;
   jsrc_st.pub.fill_input_buffer=jpg_fill_input_buffer;
   jsrc_st.pub.skip_input_data=jpg_skip_input_data;
   jsrc_st.pub.resync_to_restart=jpeg_resync_to_restart; // use default
   jsrc_st.pub.term_source=jpg_term_source;
   jsrc_st.buf=(char *)in; jsrc_st.size=blockDims[0]*blockDims[1];
   jsrc_st.pub.bytes_in_buffer=jsrc_st.size;
   jsrc_st.pub.next_input_byte=(JOCTET *)jsrc_st.buf;
   jsrc_st.eoibuf[0]=(JOCTET)0xFF;
   jsrc_st.eoibuf[1]=(JOCTET)JPEG_EOI;
 
   // grab header and startup decompress
   jpeg_read_header(&cinfo,TRUE);
   jpeg_start_decompress(&cinfo);
   
   JSAMPLE	*row = (JSAMPLE *)(&out[0]);
    while(cinfo.output_scanline<cinfo.output_height) {
        jpeg_read_scanlines(&cinfo,&row,1);
        row += blockDims[0]*3;
    }
   
   // close down decompress
   jpeg_finish_decompress(&cinfo);
   jpeg_destroy_decompress(&cinfo);

   return true;
}


bool SMJpegCodec::Compress(unsigned char *in, uint32_t inputSize, std::vector<unsigned char> &out, uint32_t blockDims[2])
//compBlock(void *data, void *cdata, int &size,int *dim)
{
   return false;
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

