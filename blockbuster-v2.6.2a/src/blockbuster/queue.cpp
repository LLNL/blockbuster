/* Copyright (c) 2003 Tungsten Graphics, Inc.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files ("the
 * Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:  The above copyright notice, the Tungsten
 * Graphics splash screen, and this permission notice shall be included
 * in all copies or substantial portions of the Software.  THE SOFTWARE
 * IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "errmsg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cache.h"
/* This file contains queue utilities for the Image Cache.  These utilities
 * are not used except for the Image Cache, and are only used if the Image
 * Cache is multi-threaded.
 *
 * In the single-threaded case, no queues are used; requests are handled 
 * directly.
 *
 * In all cases, the caller is reponsible for ensuring that proper
 * locks are held before making these calls.  Either the caller must
 * ensure that no other thread could possibly be making the calls, or
 * that imageCacheLock is held.
 */

/* This is used to prepare a queue for operation. */
void InitializeQueue(ImageCacheQueue *queue)
{
    queue->head = queue->tail = NULL;
}

/* This adds the job to the rear of the queue.  It is used when 
 * preloading new images.  (It is never used when directly loading
 * a needed image that isn't in the queue; that work is done by the
 * main thread directly.)
 */
void AddJobToQueue(ImageCacheQueue *queue, ImageCacheJob *job)
{
#if 1
    const ImageCacheJob *j;
    for (j = queue->head; j; j = j->next) {
	bb_assert(j != job);
    }
#endif

    if (queue->head == NULL) {
	queue->head = job;
	queue->tail = job;
	job->prev = NULL;
	job->next = NULL;
    }
    else {
	job->prev = queue->tail;
	job->next = NULL;
	queue->tail->next = job;
	queue->tail = job;
    }
}

/* This takes a new job off a queue, and returns it.  It returns NULL
 * if the queue is empty.
 */
ImageCacheJob *GetJobFromQueue(ImageCacheQueue *queue)
{
    ImageCacheJob *job;
    if (queue->head == NULL) return NULL;
    else {
	job = queue->head;
	queue->head = job->next;
	if (queue->head == NULL) {
	    queue->tail = NULL;
	}
	else {
	    queue->head->prev = NULL;
	}

	job->next = NULL;
    }

    return job;
}

/* This removes a job from the queue, wherever in the queue it might be.
 * Bad things will happen if the job isn't actually in the queue.
 */
void RemoveJobFromQueue(ImageCacheQueue *queue, ImageCacheJob *job)
{
    if (job == queue->head) {
	queue->head = queue->head->next;
	if (queue->head == NULL) {
	    queue->tail = NULL;
	}
	else {
	    queue->head->prev = NULL;
	}
    }
    else if (job == queue->tail) {
	queue->tail = queue->tail->prev;
	if (queue->tail == NULL) {
	    queue->head = NULL;
	}
	else {
	    queue->tail->next = NULL;
	}
    }
    else {
	job->prev->next = job->next;
	job->next->prev = job->prev;
    }

    job->next = job->prev = NULL;
}

/* This moves a job from its position on the queue to the head of the
 * queue.  It is used if a work job suddenly becomes higher in priority
 * than the rest of the jobs in the queue (say, if the work job exists
 * but is needed immediately).
 */
void MoveJobToHeadOfQueue(ImageCacheQueue *queue, ImageCacheJob *job)
{
    RemoveJobFromQueue(queue, job);
    job->next = queue->head;
    if (queue->head) {
	queue->head->prev = job;
    }
    else {
	queue->tail = job;
    }
    queue->head = job;
}

/* This finds a particular job in the queue.  The job to be matched must
 * have the same frameNumber, and the job's region either must be a
 * superset of the desired region, or the desired region must be NULL,
 * showing that any matching frame is acceptable.  The job remains
 * in the queue; it can be removed later.
 */

ImageCacheJob *FindJobInQueue(ImageCacheQueue *queue,
	unsigned int frameNumber, const Rectangle *region,
        unsigned int levelOfDetail)
{
    ImageCacheJob *job;

    job = queue->head;
    while (job) {
	if (job->frameNumber == frameNumber &&
            job->levelOfDetail == levelOfDetail) {
	    if (region == NULL || RectContainsRect(&job->region, region)) {
		return job;
	    }
	}
	job = job->next;
    }

    return NULL;
}

/* This routine clears all the jobs from the queue */
void ClearQueue(ImageCacheQueue *queue)
{
    register ImageCacheJob *job;

    while (queue->head) {
	job = queue->head;
	queue->head = job->next;

	free(job);
    }

    queue->tail = NULL;
}
