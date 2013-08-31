/*
** $RCSfile: pngsimple.h,v $
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
** $Id: pngsimple.h,v 1.1 2007/06/13 21:40:26 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/

#ifndef SIMPLE_PNG_H
#define SIMPLE_PNG_H

#ifdef __cplusplus
extern "C" {
#endif 

  int check_if_png(char *file_name,int *iSize);
int read_png_image(char *file_name,int *iSize,unsigned char *buff, int verbose);
int write_png_file(char *name, unsigned char *img, int *iSize);

#ifdef __cplusplus
}
#endif

#endif


