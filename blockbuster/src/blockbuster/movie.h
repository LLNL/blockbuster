#ifndef BLOCKBUSTER_MOVIE_H
#define BLOCKBUSTER_MOVIE_H 1
#include "frames.h"

#define REPEAT_FOREVER (-1)
int DisplayLoop(/*FrameListPtr &allFrames,*/ struct ProgramOptions *options, vector<struct MovieEvent> script);

#endif 
