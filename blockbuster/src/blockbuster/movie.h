#ifndef BLOCKBUSTER_MOVIE_H
#define BLOCKBUSTER_MOVIE_H 1
#include "frames.h"

#define LOOP_FOREVER (-1)
int DisplayLoop(FrameListPtr &allFrames, ProgramOptions *options, vector<MovieEvent> events);


#endif 
