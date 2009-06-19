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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "ui.h"
#include "errmsg.h"
#include "xwindow.h"

/* UserInterface utilities */

/*********************************************************************/
/* This used to be a big complicated section about matching user interfaces etc. but it's not needed.  There is only one user interface to match.  
 */
UserInterface *userInterfaces[] = {
  &x11UserInterface,
  NULL
};

MovieStatus MatchUserInterfaceToRenderer(
        QString userInterfaceName, QString rendererName, 
        UserInterface **matchedUserInterface, Renderer **matchedRenderer,
        int *matchedRendererIndex)
{
    register int i, j;
    int userInterfaceFound = 0;

    for (i = 0; userInterfaces[i] != NULL; i++) {
        if (
            userInterfaceName == "" || 
            userInterfaceName == userInterfaces[i]->name
			) {
            userInterfaceFound = 1;
            /* Matched the user interface; try to match the renderer */
            for (j = 0; userInterfaces[i]->supportedRenderers[j] != NULL; j++) {
                if (
                    rendererName == "" ||
                    rendererName == userInterfaces[i]->supportedRenderers[j]->renderer->name
                ) {
                    /* Match! */
                    if (matchedRendererIndex != NULL) {
                        *matchedRendererIndex = j;
                    }
                    if (matchedUserInterface != NULL) {
                        *matchedUserInterface = userInterfaces[i];
                    }
                    if (matchedRenderer != NULL) {
                        *matchedRenderer = userInterfaces[i]->supportedRenderers[j]->renderer;
                    }
                    return MovieSuccess;
                }
            }
        }
    }

    /* No match; determine the reason why */

    if (!userInterfaceFound) {
	  QString errmsg("No such user interface '%s'.");
	  ERROR(errmsg.arg(userInterfaceName));
    }
    else if (userInterfaceName == "") {
	  if (rendererName == "") {
		ERROR("No default user interface and renderer?!?");
	  }
	  else {
		QString errmsg("No user interface supports the '%s' renderer.");
		ERROR(errmsg.arg(rendererName));
	  }
    }
    else if (rendererName == "") {
	  QString errmsg("User interface '%s' doesn't support any renderers?!?");
	  ERROR(errmsg.arg(userInterfaceName));
    }
    else {
	  QString errmsg("User interface '%s' doesn't support renderer '%s'.");
	  ERROR(errmsg.arg(userInterfaceName).arg( rendererName));
    }

    return MovieFailure;
}
