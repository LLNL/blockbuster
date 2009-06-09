/*
** $RCSfile: sndplay_p.h,v $
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
** $Id: sndplay_p.h,v 1.1 2007/06/13 18:59:35 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/

#ifndef __SNDPLAY_P_H__

/* 
 * sndplay_p.h
 *
 * sndplay private interfaces 
 */

#ifdef irix
#include <dmedia/audiofile.h>
#include <dmedia/audio.h>
#endif

#include <pthread.h>

typedef struct _thr_list {
        pthread_t 		thr;
	long int		slot;
	long int		state;
        struct _thr_list 	*next;
} thr_list;

#define N_SAMP_PER_BLOCK	4000

int sndplay_init_local(void);
void sndplay_shutdown_local(void);
int sndplay_read_local(char *filename,long int slot);
int sndplay_read_next_local(char *filename);
int sndplay_play_local(long int slot);
int sndplay_quiet_local(long int slot);

void *sndplay_proc(void *input);

#define __SNDPLAY_P_H__ 1
#endif
