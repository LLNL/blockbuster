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
#ifdef USE_DMX
#include "dmxRenderer.h"

void dmxRenderer::HandleOptions(int &argc, char *argv[]){
  return; 
}

void dmxRenderer::Render(int frameNumber,
            const Rectangle *imageRegion,
                         int destX, int destY, float zoom, int lod) {
  return; 
}


void dmxRenderer::SetFrameList(FrameList *frameList) {
  return; 
}
void dmxRenderer::Preload(uint32_t frameNumber,
                          const Rectangle *imageRegion, uint32_t levelOfDetail) {
  return; 
}

#endif
