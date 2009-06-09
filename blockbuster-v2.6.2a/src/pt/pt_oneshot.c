/*
** $RCSfile: pt_oneshot.c,v $
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
** $Id: pt_oneshot.c,v 1.1 2007/06/13 18:59:32 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/


#include "pt.h"


/*********************************************************************
 *
 * Purpose:
 *
 *********************************************************************/
int pt_oneshot_nwaiting(pt_oneshot_t *oneshot)
{
   return(oneshot->noneshot);
}

/*********************************************************************
 *
 * Purpose:
 *
 *********************************************************************/
void pt_oneshot_init(pt_oneshot_t *oneshot,int nthreads)
{
   pthread_mutexattr_t mattr;
   pthread_condattr_t cattr;

   oneshot->noneshot=0; oneshot->nthreads=nthreads;
   pthread_mutexattr_init(&mattr);
   pthread_mutex_init(  &oneshot->mutex, &mattr);
   pthread_mutex_init(  &oneshot->block, &mattr);
   pthread_condattr_init(&cattr);
   pthread_cond_init (&oneshot->condvar, &cattr);
   pthread_cond_init (   &oneshot->last, &cattr);
}

/*********************************************************************
 *
 * Purpose:
 *
 *********************************************************************/
void pt_oneshot_destroy(pt_oneshot_t *oneshot)
{
   oneshot->noneshot=oneshot->nthreads=0;
   pthread_mutex_destroy(  &oneshot->mutex);
   pthread_mutex_destroy(  &oneshot->block);
   pthread_cond_destroy (&oneshot->condvar);
   pthread_cond_destroy (   &oneshot->last);
}

/*********************************************************************
 *
 * Purpose:
 *
 *********************************************************************/
void pt_oneshot_wait(pt_oneshot_t *oneshot)
{
   pthread_mutex_lock(&oneshot->block);       /* lock the block -- new   */
                                           /*   threads sleep here    */
   pthread_mutex_lock(&oneshot->mutex);       /* lock the mutex          */
   ++(oneshot->noneshot);
   pthread_mutex_unlock(&oneshot->block);  /* no, unlock block and    */
   pthread_cond_wait(&oneshot->condvar,    /*   go to sleep           */
                     &oneshot->mutex);

   if (--(oneshot->noneshot)==0) {                 /* last one out?   */
      pthread_cond_broadcast(&oneshot->last);      /* yes, wake up last one   */
   }
   pthread_mutex_unlock(&oneshot->mutex);
}

/*********************************************************************
 *
 * Purpose:
 *
 *********************************************************************/
void pt_oneshot_broadcast(pt_oneshot_t *oneshot)
{
   pthread_mutex_lock(&oneshot->block);
   pthread_mutex_lock(&oneshot->mutex);
   pthread_cond_broadcast(&oneshot->condvar);
   pthread_cond_wait(&oneshot->last, &oneshot->mutex);/* go to sleep til they're */
                                                      /* all awake... then       */
   pthread_mutex_unlock(&oneshot->block);  /* release the block       */
   pthread_mutex_unlock(&oneshot->mutex);
}
