/*
** $RCSfile: simple_jpeg.C,v $
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
** $Id: simple_jpeg.C,v 1.1 2007/06/13 18:59:34 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/
#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include "jpeglib.h"
#include "jerror.h"
}
#include <setjmp.h>

#include "simple_jpeg.h"

typedef struct {               // decode error object
        struct jpeg_error_mgr pub; // "public" fields
        jmp_buf setjmp_buffer; // for return to caller
} jpgerr_struct;

static void jpg_error_exit(j_common_ptr cinfo)
{
    jpgerr_struct *jerr;
                                                                                
    jerr=(jpgerr_struct *)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(jerr->setjmp_buffer,1);
}

int write_jpeg_image(char *file_name,unsigned char *buff,int *iSize, 
	int quality)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE	*fp;
	JSAMPROW row_pointer[1];

	// setup error handling
	cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);

	// output file
	if ((fp = fopen(file_name, "wb")) == NULL) {
  		jpeg_destroy_compress(&cinfo);
		return(-1);
	}
	jpeg_stdio_dest(&cinfo, fp);

        cinfo.image_width = iSize[0];
	cinfo.image_height = iSize[1];
	cinfo.input_components = iSize[2];        
	cinfo.in_color_space = JCS_RGB;       /* colorspace of input image */
	if (iSize[2] == 1) cinfo.in_color_space = JCS_GRAYSCALE;
	jpeg_set_defaults(&cinfo);
 
	jpeg_set_quality(&cinfo, quality, TRUE);

        jpeg_start_compress(&cinfo, TRUE);

	int row_stride = iSize[0]*iSize[2];
        while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = &buff[
			(iSize[1]-cinfo.next_scanline-1)*row_stride];
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);

	fclose(fp);

  	jpeg_destroy_compress(&cinfo);

	return(0);
}
int read_jpeg_image(char *file_name,int *iSize,unsigned char *buff)
{
   	jpgerr_struct jerr_st;
   	struct jpeg_decompress_struct cinfo;
	FILE *fp;
	if ((fp = fopen(file_name, "rb")) == NULL) return 0;
                                                                                
	// setup error handling
	cinfo.err=jpeg_std_error(&(jerr_st.pub));
	jerr_st.pub.error_exit=jpg_error_exit;
	if (setjmp(jerr_st.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		fclose(fp);
        	return(0);
   	}

	// start the read operation 
   	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, fp);
                                                                                
	// grab header and startup decompress
	jpeg_read_header(&cinfo,TRUE);
	jpeg_start_decompress(&cinfo);
                                                                                
	if (iSize) {
		iSize[0] = cinfo.output_width;
		iSize[1] = cinfo.output_height;
		iSize[2] = cinfo.output_components;
	}

	// if being used as a probe...
	if (!buff) {
		jpeg_destroy_decompress(&cinfo);
		fclose(fp);
		return(1);
	}


	while(cinfo.output_scanline<cinfo.output_height) {
		JSAMPLE     *row = (JSAMPLE *)buff+
			((iSize[1]-cinfo.output_scanline-1)*iSize[0]*iSize[2]);
		jpeg_read_scanlines(&cinfo,&row,1);
	}

	// close down decompress
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	fclose(fp);

	return(1);
}
int check_if_jpeg(char *file_name,int *iSize)
{
	return(read_jpeg_image(file_name,iSize,NULL));
}
