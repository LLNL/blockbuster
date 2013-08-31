/*
** $RCSfile: simple.c,v $
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
** $Id: simple.c,v 1.1 2007/06/13 21:40:26 wealthychef Exp $
**
*/
/*
**
**  Abstract:  Simple interface to the PNG library...
**
**  Author:
**
*/


#include <stdlib.h>
#include "png.h"
#include "pngsimple.h"

#define PNG_BYTES_TO_CHECK 4
int check_if_png(char *file_name, int *iSize)
{
  png_byte 	buf[PNG_BYTES_TO_CHECK];
  FILE 	*fp;
  png_structp 	png_ptr;
  png_infop 	info_ptr;
  png_uint_32 	width, height;
  int 		bit_depth, color_type, interlace_type, ret;

  /* Open the prospective PNG file. */
  if ((fp = fopen(file_name, "rb")) == NULL) return 0;

  /* Read in some of the signature bytes */
  if (fread(buf, 1, PNG_BYTES_TO_CHECK, fp) != PNG_BYTES_TO_CHECK) return 0;

  ret = png_sig_cmp(buf, (png_size_t)0, PNG_BYTES_TO_CHECK);
  if (ret) {
   	fclose(fp);
	return(0);
  }

  /* get the file size (dx,dy,spp) */
  if (iSize) {
	
   	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
   	if (png_ptr == NULL) {
      fclose(fp);
      return (0);
   	}

   	/* Allocate/initialize the memory for image information. */
   	info_ptr = png_create_info_struct(png_ptr);
   	if (info_ptr == NULL) {
      fclose(fp);
      png_destroy_read_struct(&png_ptr, NULL, NULL); 
      return (0);
   	}

   	if (setjmp(png_jmpbuf(png_ptr))) {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      fclose(fp);
      return (0);
   	}

   	png_init_io(png_ptr, fp);

   	/* If we have already read some of the signature */
   	png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

	png_read_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, 
                 &color_type, &interlace_type, NULL, NULL);

	iSize[0] = width;
	iSize[1] = height;
	iSize[2] = png_get_channels(png_ptr, info_ptr);

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  }

  fclose(fp);

  return(1);
}

int read_png_image(char *file_name,int *iSize,unsigned char *buff, int verbose)
{
  png_byte 	buf[PNG_BYTES_TO_CHECK];
  FILE 	*fp;
  png_structp 	png_ptr;
  png_infop 	info_ptr;
  png_uint_32 	width, height;
  int 		bit_depth, color_type, interlace_type, ret, row;
  png_bytep *row_pointers;

  /* Open the prospective PNG file. */
  if ((fp = fopen(file_name, "rb")) == NULL) {
    if (verbose) {
      fprintf(stderr, "Cannot open file %s\n", file_name); 
    }
    return 0;
  }

  /* Read in some of the signature bytes */
  if (fread(buf, 1, PNG_BYTES_TO_CHECK, fp) != PNG_BYTES_TO_CHECK) {
    if (verbose) {
      fprintf(stderr, "Cannot read file %s\n", file_name); 
    }
    return 0;
  }

  /* Compare the first PNG_BYTES_TO_CHECK bytes of the signature.
     Return nonzero (true) if they match */
  ret = png_sig_cmp(buf, (png_size_t)0, PNG_BYTES_TO_CHECK);
  if (ret) {
    if (verbose) {
      fprintf(stderr, "File %s is not a PNG file\n", file_name); 
    }
   	fclose(fp);
	return(0);
  }

  /* get the file size (dx,dy,spp) */
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
  if (png_ptr == NULL) {
    if (verbose) {
      fprintf(stderr, "Could not create read struct for file %s (memory error?)\n", file_name); 
    }
    fclose(fp);
    return (0);
  }

  /* Allocate/initialize the memory for image information. */
  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    if (verbose) {
      fprintf(stderr, "Could not allocate/initialize the memory for image information for file %s\n", file_name); 
    }
    fclose(fp);
    png_destroy_read_struct(&png_ptr, NULL, NULL); 
    return (0);
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    if (verbose) {
      fprintf(stderr, "setjmp failed for PNG file %s\n", file_name); 
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return (0);
  }

  png_init_io(png_ptr, fp);

  /* If we have already read some of the signature */
  png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

  png_read_info(png_ptr, info_ptr);

  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, 
               &color_type, &interlace_type, NULL, NULL);

  if ((iSize[0] != width) || (iSize[1] != height)) {
    if (verbose) {
      fprintf(stderr, "PNG size (%d,%d) does not match expected size (%d, %d) for file %s\n", width, height, iSize[0], iSize[1], file_name);
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);
	return (0);
  }
  if ((iSize[2] > 0) && (iSize[2] != png_get_channels(png_ptr, info_ptr))) {
    if (verbose) {
      fprintf(stderr, "PNG depth (%d) does not match expected depth (%d) for file %s\n", png_get_channels(png_ptr, info_ptr), iSize[2], file_name);
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);
	return (0);
  }

  /* set up the read */
  png_set_strip_16(png_ptr);

  /* one sample/byte */
  png_set_packing(png_ptr);

  /* no alpha */
  if (color_type & PNG_COLOR_MASK_ALPHA) png_set_strip_alpha(png_ptr);

  /* palette -> RGB */
  if (color_type == PNG_COLOR_TYPE_PALETTE) 
	png_set_palette_to_rgb(png_ptr);

  /* expand to 8 bit values */
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) 
	png_set_gray_1_2_4_to_8(png_ptr);

  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  row_pointers = (png_bytep *)malloc(height*sizeof(png_bytep));
  for (row = 0; row < height; row++) {
    row_pointers[row]=(png_bytep)malloc(png_get_rowbytes(png_ptr,info_ptr));
  }

  /* read it */
  png_read_image(png_ptr, row_pointers);

  png_read_end(png_ptr, info_ptr);

  /* suck out the bytes */
  for (row = 0; row < height; row++) {
	memcpy(buff+(row*3*iSize[0]),row_pointers[height-row-1],3*iSize[0]);
  }

  /* done */
  for (row = 0; row < height; row++) free(row_pointers[row]); 
  free(row_pointers);

  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  fclose(fp);

  return(1);
}

int write_png_file(char *name, unsigned char *img, int *iSize)
{
  FILE *fp;
  png_structp png_ptr;
  png_infop info_ptr;
  png_uint_32 k;
  png_bytep *row_pointers;

  fp = fopen(name, "wb");
  if (!fp) return(-1);

  png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING,
                                     NULL,NULL,NULL);
  if (!png_ptr) {
    fclose(fp);
    return(-1);
  }

  /* Allocate/initialize the image information data.  */
  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    fclose(fp);
    png_destroy_write_struct(&png_ptr,  png_infopp_NULL);
    return (-1);
  }

   
  /* Set error handling.  if you aren't supplying your own
   * error handling functions in the png_create_write_struct() call.
   */
  if (setjmp(png_jmpbuf(png_ptr))) {
    fclose(fp);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return (-1);
  }

  /* do it */
  png_init_io(png_ptr, fp);

  /* Set the image information here.  Width and height are up to 2^31,
   * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
   * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
   * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
   * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
   * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
   * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. 
   */
  png_set_IHDR(png_ptr, info_ptr, iSize[0], iSize[1], 8, 
               PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, 
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  png_write_info(png_ptr, info_ptr);

  png_set_packing(png_ptr);

  row_pointers = (png_bytep *)malloc(iSize[1]*sizeof(png_bytep));
  for (k = 0; k < iSize[1]; k++) {
    row_pointers[iSize[1]-k-1] = img + k*iSize[0]*3;
  }

  png_write_image(png_ptr, row_pointers);

  png_write_end(png_ptr, info_ptr);

  free(row_pointers);

  png_destroy_write_struct(&png_ptr, &info_ptr);

  /* done */
  fclose(fp);

  return(0);
}
