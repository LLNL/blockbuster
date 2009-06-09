/*
** $RCSfile: pt_gate.c,v $
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
** $Id: pt_gate.c,v 1.1 2007/06/13 18:59:32 wealthychef Exp $
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
#include "pt.h"

#define STATE_LOAD 0
#define STATE_DUMP 1

/*********************************************************************
 *
 * Purpose:
 *
 *********************************************************************/
void pt_gate_init(pt_gate_t *gate,int nthreads)
{
   gate->ngate=0; gate->nthreads=nthreads;
   gate->state=STATE_LOAD;
   pthread_mutex_init(  &gate->mutex, NULL);
   pthread_cond_init (&gate->condvar, NULL);
   pthread_cond_init (   &gate->last, NULL);
}

/*********************************************************************
 *
 * Purpose:
 *
 *********************************************************************/
void pt_gate_destroy(pt_gate_t *gate)
{
   gate->ngate=gate->nthreads=0;
   pthread_mutex_destroy(  &gate->mutex);
   pthread_cond_destroy (&gate->condvar);
   pthread_cond_destroy (   &gate->last);
}

/*********************************************************************
 *
 * Purpose:
 *
 *********************************************************************/
void pt_gate_sync(pt_gate_t *gate)
{
   if (gate->nthreads<2) return;           /* trivial case            */

   pthread_mutex_lock(&gate->mutex);

   if (gate->state == STATE_DUMP) {
      while(gate->state == STATE_DUMP) {
	pthread_cond_wait(&gate->condvar, &gate->mutex);
      }
   }
   gate->ngate += 1;
#ifdef DEBUG
   printf("%d : outof condvar %d of %d\n",pthread_self(),gate->ngate,gate->nthreads);
   fflush(stdout);
#endif
   if (gate->ngate == gate->nthreads) {
#ifdef DEBUG
	printf("%d : %d %d state to DUMP\n",pthread_self(),gate->ngate,gate->nthreads);
   fflush(stdout);
#endif
	gate->state = STATE_DUMP;
        pthread_cond_broadcast(&gate->last);
   }
   if (gate->state == STATE_LOAD) {
     while(gate->state == STATE_LOAD) {
	pthread_cond_wait(&gate->last, &gate->mutex);
     }
   }
   gate->ngate -= 1;
#ifdef DEBUG
   printf("%d : outof last %d of %d\n",pthread_self(),gate->ngate,gate->nthreads);
   fflush(stdout);
#endif
   if (gate->ngate == 0) {
	gate->state = STATE_LOAD;
#ifdef DEBUG
printf("%d : %d %d state to LOAD\n",pthread_self(),gate->ngate,gate->nthreads);
   fflush(stdout);
#endif
	pthread_cond_broadcast(&gate->condvar);
   }
   pthread_mutex_unlock(&gate->mutex);
   return;
}

