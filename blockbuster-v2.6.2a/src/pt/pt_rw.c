/********************************************************
 * An example source module to accompany...
 *
 * "Using POSIX Threads: Programming with Pthreads"
 *     by Brad nichols, Dick Buttlar, Jackie Farrell
 *     O'Reilly & Associates, Inc.
 *
 ********************************************************
 * 
 * Library of functions implementing reader/writer locks
 */
#include <pthread.h>
#include "pt.h"

void pt_rw_init(pt_rw_t *rdwrp)
{
  rdwrp->readers_reading = 0;
  rdwrp->readers_waiting = 0;
  rdwrp->writer_writing = 0;
  rdwrp->writers_waiting = 0;
  rdwrp->shutdown = 0;
  pthread_mutex_init(&(rdwrp->mutex), NULL);
  pthread_cond_init(&(rdwrp->lock_free), NULL);
  return;
}

void pt_rw_destroy(pt_rw_t *rdwrp)
{
  pthread_mutex_lock(&(rdwrp->mutex));
  rdwrp->shutdown = 1;
  while(rdwrp->writer_writing || rdwrp->readers_reading ||
	rdwrp->writers_waiting || rdwrp->readers_waiting) {
    pthread_cond_wait(&(rdwrp->lock_free), &(rdwrp->mutex));
  }
  pthread_mutex_unlock(&(rdwrp->mutex));

  pthread_mutex_destroy(&(rdwrp->mutex));
  pthread_cond_destroy(&(rdwrp->lock_free));
  return;
}

int pt_rw_read_lock(pt_rw_t *rdwrp)
{
  pthread_mutex_lock(&(rdwrp->mutex));
  if (rdwrp->shutdown) {
    pthread_mutex_unlock(&(rdwrp->mutex));
    return -1;
  }
  rdwrp->readers_waiting++;
  while(rdwrp->writer_writing || rdwrp->writers_waiting) {
    pthread_cond_wait(&(rdwrp->lock_free), &(rdwrp->mutex));
    if (rdwrp->shutdown) {
      rdwrp->readers_waiting--;
      pthread_mutex_unlock(&(rdwrp->mutex));
      return -1;
    }
  }
  rdwrp->readers_waiting--;
  rdwrp->readers_reading++;
  pthread_mutex_unlock(&(rdwrp->mutex));
  return 0;
}

int pt_rw_read_unlock(pt_rw_t *rdwrp)
{
  pthread_mutex_lock(&(rdwrp->mutex));
  if (rdwrp->readers_reading == 0) {
    pthread_mutex_unlock(&(rdwrp->mutex));
    return -1;
  }
  rdwrp->readers_reading--;
  if (rdwrp->readers_reading == 0) {
    /* wake everyone, waiting writers have priority */
    pthread_cond_broadcast(&(rdwrp->lock_free));
  }
  pthread_mutex_unlock(&(rdwrp->mutex));
  return 0;
}

int pt_rw_write_lock(pt_rw_t *rdwrp)
{
  pthread_mutex_lock(&(rdwrp->mutex));
  if (rdwrp->shutdown) {
    pthread_mutex_unlock(&(rdwrp->mutex));
    return -1;
  }
  rdwrp->writers_waiting++;
  while(rdwrp->writer_writing || rdwrp->readers_reading) {
    pthread_cond_wait(&(rdwrp->lock_free), &(rdwrp->mutex));
    if (rdwrp->shutdown) {
      rdwrp->writers_waiting--;
      pthread_mutex_unlock(&(rdwrp->mutex));
      return -1;
    }
  }
  rdwrp->writers_waiting--;
  rdwrp->writer_writing = 1;
  pthread_mutex_unlock(&(rdwrp->mutex));
  return 0;
}

int pt_rw_write_unlock(pt_rw_t *rdwrp)
{
  pthread_mutex_lock(&(rdwrp->mutex));
  if (rdwrp->writer_writing == 0) {
    pthread_mutex_unlock(&(rdwrp->mutex));
    return -1;
  }
  rdwrp->writer_writing = 0;
  pthread_cond_broadcast(&(rdwrp->lock_free));
  pthread_mutex_unlock(&(rdwrp->mutex));
  return 0;
}
