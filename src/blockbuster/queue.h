#ifndef BLOCKBUSTER_QUEUE_H
#define BLOCKBUSTER_QUEUE_H

  /* Additional Image Cache utility functions, from queue.c */
  extern void InitializeQueue(ImageCacheQueue *queue);
  extern void AddJobToQueue(ImageCacheQueue *queue, ImageCacheJob *job);
  extern ImageCacheJob *GetJobFromQueue(ImageCacheQueue *queue);
  extern void RemoveJobFromQueue(ImageCacheQueue *queue, ImageCacheJob *job);
  extern void MoveJobToHeadOfQueue(ImageCacheQueue *queue, ImageCacheJob *job);
  extern ImageCacheJob *FindJobInQueue(ImageCacheQueue *queue,
               unsigned int frameNumber, const Rectangle *region,
               unsigned int levelOfDetail);
  extern void ClearQueue(ImageCacheQueue *queue);

#endif
