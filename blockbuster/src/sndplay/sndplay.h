/*
** $RCSfile: sndplay.h,v $
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
** $Id: sndplay.h,v 1.1 2007/06/13 18:59:35 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/

#ifndef __SNDPLAY_H__

/* 
 * sndplay.h
 *
 * sndplay public interfaces 
 */
typedef struct {
	void 		*data;
	int		format;
	int		width;
	double		rate;
	long int	nchannels;
	long int	frames;
	long int	framesize;
	char		*filename;
} sndbuff;

/* main lib interface */
int sndplay_init(void);
void sndplay_shutdown(void);
int sndplay_quiet(long int slot);
int sndplay_play(long int slot);
int sndplay_read(char *filename, long int slot);
int sndplay_read_next(char *filename);

/* utilities */
sndbuff *sndplay_load(char *filename);
void sndplay_free(sndbuff *snd);

#define SND_NO_ERR	0
#define SND_ERR		-1

#define MAX_SNDS	512

#define __SNDPLAY_H__ 1
#endif
