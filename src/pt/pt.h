/*
** $RCSfile: pt.h,v $
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
** $Id: pt.h,v 1.1 2007/06/13 18:59:32 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/


#ifndef PT_H
#define PT_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _pt_gate_t {
   int ngate;
   int nthreads;
   pthread_mutex_t mutex;
   pthread_cond_t condvar;
   pthread_cond_t last;
   int state;
} pt_gate_t;


typedef struct _pt_oneshot_t {
   int noneshot;
   int nthreads;
   pthread_mutex_t mutex;
   pthread_mutex_t block;
   pthread_cond_t condvar;
   pthread_cond_t last;
} pt_oneshot_t;


typedef struct _pt_semaphore_t {
   int count;
   int nthreads;
   pthread_mutex_t mutex;
   pthread_mutex_t block;
   pthread_cond_t condvar;
   pthread_cond_t last;
} pt_semaphore_t;

void pt_gate_init(pt_gate_t *gate,int nthreads);
void pt_gate_destroy(pt_gate_t *gate);
void pt_gate_sync(pt_gate_t *gate);

void pt_oneshot_init(pt_oneshot_t *oneshot, int nthreads);
void pt_oneshot_destroy(pt_oneshot_t *oneshot);
void pt_oneshot_wait(pt_oneshot_t *oneshot);
void pt_oneshot_broadcast(pt_oneshot_t *oneshot);
int  pt_oneshot_nwaiting(pt_oneshot_t *oneshot);

/* from "Using POSIX Threads: Programming with Pthreads"    */
/* API modified for private namespace and several mods made */
typedef struct pt_pool_work {
	void               (*routine)(void *);
	void                *arg;
	struct pt_pool_work   *next;
} pt_pool_work_t;

typedef struct pt_pool {
	/* pool characteristics */
	int                 num_threads;
        int                 max_queue_size;
        int                 do_not_block_when_full;
        /* pool state */
	pthread_t           *threads;
        int                 cur_queue_size;
	pt_pool_work_t        *queue_head;
	pt_pool_work_t        *queue_tail;
	pt_pool_work_t        *free_queue;
	int                 queue_closed;
        int                 shutdown;
	/* pool synchronization */
        pthread_mutex_t     queue_lock;
        pthread_cond_t      queue_not_empty;
        pthread_cond_t      queue_not_full;
	pthread_cond_t      queue_empty;
} *pt_pool_t;

void pt_pool_init( pt_pool_t tpool, int num_threads, int max_queue_size, 
			int do_not_block_when_full);
int pt_pool_add_work( pt_pool_t tpool,void (*routine)(void *),void *arg);
void pt_pool_destroy( pt_pool_t tpool, int finish);
  int pt_pool_threadnum(void);


typedef struct rdwr_var {
  int readers_reading;
  int readers_waiting;
  int writer_writing;
  int writers_waiting;
  int shutdown;
  pthread_mutex_t mutex;
  pthread_cond_t lock_free;
} pt_rw_t;

void pt_rw_init(pt_rw_t *rdwrp);
int pt_rw_read_lock(pt_rw_t *rdwrp);
int pt_rw_read_unlock(pt_rw_t *rdwrp);
int pt_rw_write_lock(pt_rw_t *rdwrp);
int pt_rw_write_unlock(pt_rw_t *rdwrp);
void pt_rw_destroy(pt_rw_t *rdwrp);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
