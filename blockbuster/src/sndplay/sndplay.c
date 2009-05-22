/*
** $RCSfile: sndplay.c,v $
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
** $Id: sndplay.c,v 1.1 2007/06/13 18:59:35 wealthychef Exp $
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
 * sndplay.c
 *
 *	Basic async audio I/O and play control.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef WIN32
#include <unistd.h>
#else
#include <io.h>
#define popen _popen
#define pclose _pclose
#endif

#define FREE(a)		free((a))
#define MALLOC(a)	malloc((a))

#include "sndplay.h"
#include "sndplay_p.h"

/* globals */
static FILE    *pfp = NULL;
static int     bUseLocal = 0;

static sndbuff		*buffers[MAX_SNDS] = {0};
static thr_list	*threads = NULL;

int sndplay_init(void)
{
	char		*rmtcmd;

	if (rmtcmd = getenv("SND_RMT_CMD")) {
		pfp = popen(rmtcmd,"w");
		if (!pfp) return(SND_ERR);
		bUseLocal = 0;
	} else {
		bUseLocal = 1;
		if (sndplay_init_local()) return(SND_ERR);
	}

	return(SND_NO_ERR);
}

int sndplay_init_local(void)
{
	long int	i;

	for(i=0;i<MAX_SNDS;i++) buffers[i] = NULL;
	threads = NULL;

	return(SND_NO_ERR);
}

void sndplay_shutdown(void)
{
	if (!bUseLocal) {
		if (pfp) pclose(pfp);
		pfp = NULL;
	} else {
		sndplay_shutdown_local();
	}

	return;
}

void sndplay_shutdown_local(void)
{
	long int        i;

	sndplay_quiet_local(-1);

	for(i=0;i<MAX_SNDS;i++) {
		if (buffers[i]) sndplay_free(buffers[i]);
		buffers[i] = NULL;
	}

	return;
}

int sndplay_quiet(long int slot)
{
	if (slot > MAX_SNDS) return(SND_ERR);
	if (slot < -1) return(SND_ERR);

	if (bUseLocal) {
		return(sndplay_quiet_local(slot));
	} else {
		if (!pfp) return(SND_ERR);
		fprintf(pfp,"q %ld\n",slot);
	}

	return(SND_NO_ERR);
}

int sndplay_quiet_local(long int slot)
{
	thr_list	*list = threads;
	thr_list	*next,*prev;

	prev = NULL;

	/* walk the thread list looking for things to kill */
	while(list) {
		long int	term = 0;

		next = list->next;

		/* kill it if the slot number matches or is -1 */
		if ((slot == -1) || (list->slot == slot)) term = 1;

		/* passively reap dead threads */
		if (pthread_cancel(list->thr)) term = 1;

		/* kill and remove the thread */
		if (term) {
			/* tag the current state to quit */
			list->state = 1; 
			/* and reel it in... */
			pthread_join(list->thr,NULL);
			if (!prev) {
				threads = next;
			} else {
				prev->next = next;
			}
			FREE(list);
		} else {
			prev = list;
		}

		list = next;
	}

	return(SND_NO_ERR);
}

int sndplay_read(char *filename,long int slot)
{
	if (slot > MAX_SNDS) return(SND_ERR);
	if (slot < 0) return(SND_ERR);

	if (bUseLocal) {
		return(sndplay_read_local(filename,slot));
	} else {
		if (!pfp) return(SND_ERR);
		fprintf(pfp,"r %ld %s\n",slot,filename);
	}

	return(SND_NO_ERR);
}

int sndplay_read_local(char *filename,long int slot)
{
	if (buffers[slot]) {
		sndplay_quiet_local(slot);
		sndplay_free(buffers[slot]);
		buffers[slot] = NULL;
	}

	buffers[slot] = sndplay_load(filename);

	if (!buffers[slot]) return(SND_ERR);
	
	return(SND_NO_ERR);
}

int sndplay_read_next(char *filename)
{
	if (bUseLocal) {
		return(sndplay_read_next_local(filename));
	} else {
		return(SND_ERR);  /* not available remotely */
	}
}

int sndplay_read_next_local(char *filename)
{
	long int	slot;

	if (!filename) return(SND_ERR);

	for(slot=0;slot<MAX_SNDS;slot++) {
		if (buffers[slot]) {
			if (strcmp(filename,buffers[slot]->filename) == 0) {
				return(slot);
			}
		}
	}

	for(slot=0;slot<MAX_SNDS;slot++) if (!buffers[slot]) break;
	if (slot >= MAX_SNDS) return(SND_ERR);

	buffers[slot] = sndplay_load(filename);

	if (!buffers[slot]) return(SND_ERR);
	
	return(slot);
}

int sndplay_play(long int slot)
{
	if (slot > MAX_SNDS) return(SND_ERR);
	if (slot < 0) return(SND_ERR);

	if (bUseLocal) {
		return(sndplay_play_local(slot));
	} else {
		if (!pfp) return(SND_ERR);
		fprintf(pfp,"p %ld\n",slot);
	}

	return(SND_NO_ERR);
}

int sndplay_play_local(long int slot)
{
	thr_list	*play;

	if (!buffers[slot]) return(SND_ERR);

	play = (thr_list *)MALLOC(sizeof(thr_list));
	if (!play) return(SND_ERR);

	play->slot = slot;
	play->state = 0;

	/* fire up a thread */
	if (pthread_create(&(play->thr),NULL,sndplay_proc,play)) {
		FREE(play);
		return(SND_ERR);
	}

	/* add it to the threads list */
	play->next = threads;
	threads = play;

	return(SND_NO_ERR);
}

void *sndplay_proc(void *input)
{
#ifdef irix
#ifndef irix64
	thr_list	*play = (thr_list *)input;
	sndbuff		*snd;
	ALconfig        audio_port_config;
	ALport          audio_port;
	char		tstr[256];
	long int	i;
	char 		*buffer;
	ALpv		params[2];

	/* grab a sound handle */
	if (!buffers[play->slot]) return(NULL);
	snd = buffers[play->slot];

	/* configure an audio port */
	audio_port_config = alNewConfig();
	if (!audio_port_config) {
		fprintf(stderr,"Unable to obtain a configuration\n");
		return(NULL);
	}
	if (alSetWidth( audio_port_config,snd->width)) {
		fprintf(stderr,"Unable to set width %d\n",snd->width);
		return(NULL);
	}
	if (alSetChannels( audio_port_config,snd->nchannels)) {
		fprintf(stderr,"Unable to set # of chan %ld\n",snd->nchannels);
		return(NULL);
	}
	if (alSetQueueSize( audio_port_config, N_SAMP_PER_BLOCK)) {
		fprintf(stderr,"Unable to set queuesize %d\n",N_SAMP_PER_BLOCK);
		return(NULL);
	}

	/* create an audio port */
	sprintf(tstr,"%ld",input);
	audio_port = alOpenPort( tstr, "w", audio_port_config );
	alFreeConfig( audio_port_config );
	if (!audio_port) {
		fprintf(stderr,"Unable to create port\n");
		return(NULL);
	}

	/* set its sampling rate */
	params[0].param = AL_MASTER_CLOCK;
	params[0].value.i = AL_CRYSTAL_MCLK_TYPE;
	params[1].param = AL_RATE;
	params[1].value.ll = alDoubleToFixed(snd->rate);
	i = alSetParams( alGetResource(audio_port), params, 2);
	if (i != 2) {
		fprintf(stderr,"Unable to set rate %f (%ld)\n",snd->rate,i);
	}

	/* send the sample */
	buffer = (char *)snd->data;
	i = snd->frames;
	while(i > N_SAMP_PER_BLOCK) {
		alWriteFrames(audio_port,buffer,N_SAMP_PER_BLOCK);
		buffer += (snd->framesize*N_SAMP_PER_BLOCK);
		i -= N_SAMP_PER_BLOCK;
		/* if we have been requested to stop, do so... */
		if (play->state) {
			alClosePort(audio_port);
			return(NULL);
		}
	}
	alWriteFrames(audio_port,buffer,i);

	while(alGetFilled(audio_port) && (!play->state)) usleep(1000);

	alClosePort(audio_port);
#endif
#endif

	return(NULL);
}

sndbuff *sndplay_load(char *filename)
{
#ifdef irix
#ifndef irix64
	AFfilehandle	file;
	AFfileoffset	size;
	sndbuff		*buff;

	if (!filename) return(NULL);

	buff = (sndbuff *)MALLOC(sizeof(sndbuff));
	if (!buff) return(NULL);

	file = afOpenFile(filename,"r",NULL);
	if (!file) {
		FREE(buff);
		return(NULL);
	}

	buff->nchannels = afGetChannels(file,AF_DEFAULT_TRACK);
	buff->rate = afGetRate(file,AF_DEFAULT_TRACK);
	afGetSampleFormat(file,AF_DEFAULT_TRACK,&(buff->format),
		&(buff->width));
	buff->frames = afGetFrameCount(file,AF_DEFAULT_TRACK);

	size = afGetTrackBytes(file,AF_DEFAULT_TRACK);
	buff->data = (void *)MALLOC(size);
	if (!buff->data) {
		FREE(buff);
		afCloseFile(file);
		return(NULL);
	}

	buff->framesize = (buff->width*buff->nchannels)/8;

	switch(buff->width) {
		case 8:
			buff->width = AL_SAMPLE_8;
			break;
		case 16:
			buff->width = AL_SAMPLE_16;
			break;
		case 24:
			buff->width = AL_SAMPLE_24;
			break;
		default:
			buff->width = AL_SAMPLE_16;
			break;
	}
	switch(buff->nchannels) {
		case 1:
			buff->nchannels = AL_MONO;
			break;
		case 2:
			buff->nchannels = AL_STEREO;
			break;
		default:
			buff->nchannels = AL_MONO;
			break;
	}
	

	if (afReadFrames(file,AF_DEFAULT_TRACK,buff->data,buff->frames) == -1) {
		FREE(buff->data);
		FREE(buff);
		afCloseFile(file);
		return(NULL);
	}
	afCloseFile(file);

	buff->filename = strdup(filename);

	return(buff);
#else
	return(NULL);
#endif
#endif
}

void sndplay_free(sndbuff *snd)
{
	if (snd) {
		if (snd->data) FREE(snd->data);
		if (snd->filename) FREE(snd->filename);
		FREE(snd);
	}
	return;
}
