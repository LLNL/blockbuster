/*
** $RCSfile: simple_jpeg.h,v $
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
** $Id: simple_jpeg.h,v 1.1 2007/06/13 18:59:34 wealthychef Exp $
**
*/
/*
**
**  Abstract:  Greatly simplified interface to basic JPEG file I/O.
**
**  Author: rjf
**
*/

#ifndef __SIMPLE_JPEG_H__
#define __SIMPLE_JPEG_H__

int write_jpeg_image(char *file_name,unsigned char *buff,int *iSize, 
	int quality);
int read_jpeg_image(char *file_name,int *iSize,unsigned char *buff);
int check_if_jpeg(char *file_name,int *iSize);

#endif
