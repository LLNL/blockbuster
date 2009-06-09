/*
** $RCSfile: test.c,v $
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
** $Id: test.c,v 1.1 2007/06/13 18:59:35 wealthychef Exp $
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
#include <math.h>
#include <string.h>
#include <unistd.h>

#include "sndplay.h"

int main(int argc, char **argv)
{
	char	*files[] = {"pop.aiff","laser.aiff","fire.aiff",NULL};
	long int	i,j;
	long int	slots[10];

	if (sndplay_init()) {
		fprintf(stderr,"Unable to init snd system.\n");
		exit(1);
	}

	i = 1;
	while(files[i]) {
		if (sndplay_read(files[i],i-1)) {
			fprintf(stderr,"Unable to read the file:%s\n",files[i]);
		}
		i++;
	}

	for(j=0;j<i;j++) {
		sndplay_play(j);
		usleep(200000);
	}

	for(j=0;j<i+1;j++) {
		slots[j] = sndplay_read_next(files[j]);
		printf("slot[%ld]= %ld\n",j,slots[j]);
	}

	sleep(1);

	sndplay_shutdown();

	exit(0);
}
