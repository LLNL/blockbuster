
#include "BufferedStreamingMovie.h"

using namespace boost; 

void RenderBuffer::SetState(const RenderState &state) {
    lock_guard<mutex> lock(mMutex); 
    mRenderState = state;  
    return; 
}
