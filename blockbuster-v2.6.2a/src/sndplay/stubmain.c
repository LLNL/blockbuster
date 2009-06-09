/*
** $RCSfile: stubmain.c,v $
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
** $Id: stubmain.c,v 1.1 2007/06/13 18:59:35 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/

/*
 * stubmain.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define FREE(a)         free((a))
#define MALLOC(a)       malloc((a))
 
#include "sndplay.h"
#include "sndplay_p.h"

int main(int argc,char **argv)
{
        char            tstr[256];
        char            file[256];
        long int        pos,i;

	/* init the buffers */
	if (sndplay_init_local()) exit(1);

	/* handle commands */
        while(fgets(tstr,256,stdin)) {
                switch(tstr[0]) {
                        case 'r':
                                sscanf(tstr+1,"%d %s",&pos,file);
				sndplay_read_local(file,pos);
                                break;
                        case 'p':
                                sscanf(tstr+1,"%d",&pos);
				sndplay_play_local(pos);
                                break;
                        case 'q':
                                sscanf(tstr+1,"%d",&pos);
				sndplay_quiet_local(pos);
                                break;
                        default:
                                break;
                }
        }

	/* free up the loaded snd data */
	sndplay_shutdown_local();

        exit(0);
}
