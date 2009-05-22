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

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include "settings.h"
#include "errmsg.h"

void printargv(int argc, char *argv[]) {
  fprintf(stderr, "argv is: {"); 
  int argnum = 0; 
  while (argnum < argc) {
    fprintf(stderr, argv[argnum]); 
    if (argnum < argc-1) {
      fprintf(stderr, ", "); 
    } else {
      fprintf(stderr, "}\n"); 
    }
    argnum ++; 
  }
}
/* "consume" the first arg in argv by shifting all later args one to the left */ 
void ConsumeArg(int &argc, char *argv[], int position) {
  //  fprintf(stderr, "Before ConsumeArg(%d, argv, %d): ", argc, position); 
  //printargv(argc, argv); 
  if (position == 0 and argc == 1) {
    argv[0] = NULL; 
  } else {
    while (position < argc) {
      argv[position] = argv[position+1];
      ++position; 
    }
  }
  argc--; 
  //fprintf(stderr, "After ConsumeArg: "); 
  //printargv(argc, argv); 
  return; 
}


void *CreateBlankSettings(void)
{
    Settings *settings;
    settings = (Settings *)calloc(1, sizeof(Settings));
    if (settings == NULL) {
	return NULL;
    }
    settings->head = settings->tail = NULL;
    return settings;
}

static Setting *FindSetting(Settings *settings, const char *variable)
{
    Setting *setting;

    if (settings == NULL) return NULL;

    setting = settings->head;
    while (setting != NULL) {
	if (strcmp(setting->variable, variable) == 0) {
	    return setting;
	}
	setting = setting->next;
    }
    return NULL;
}

static void Set(Settings *settings, const char *variable, const char *value,
	const char *origin)
{
    Setting *setting;

    if (settings == NULL) return;
    if (variable == NULL) return;
    if (value == NULL) return;

    setting = FindSetting(settings, variable);
    if (setting != NULL && strcmp(setting->value, value) == 0) {
	/* Setting back to the original value = do nothing */
	return;
    }
    if (setting == NULL) {
	/* We have to create our own setting.  Set up the
	 * structure and link it in; we'll allocate the 
	 * value later.
	 */
	setting = (Setting *)calloc(1, sizeof(Setting));
	if (setting == NULL) {
	    WARNING("Could not allocate new setting");
	    return;
	}
	setting->variable = (char *)malloc(strlen(variable) + 1);
	if (setting->variable == NULL) {
	    WARNING("could not allocate new setting name");
	    free(setting);
	    return;
	}
	strcpy(setting->variable, variable);

	if (origin == NULL) {
	    setting->origin = NULL;
	}
	else {
	    setting->origin = (char *)malloc(strlen(origin) + 1);
	    if (setting->origin == NULL) {
		WARNING("could not allocate setting origin");
		free(setting->variable);
		free(setting);
		return;
	    }
	    strcpy(setting->origin, origin);
	}

	setting->next = NULL;
	if (settings->head == NULL) {
	    settings->head = setting;
	    settings->tail = setting;
	}
	else {
	    settings->tail->next = setting;
	    settings->tail = setting;
	}
    }
    else {
	/* We found the setting; its name and origin don't change,
	 * but we have to free and reallocate the value.
	 */
	if (setting->value != NULL) {
	    free(setting->value);
	}
    }

    if (value == NULL) {
	setting->value = NULL;
    }
    else {
	setting->value = (char *)malloc(strlen(value) + 1);
	if (setting->value == NULL) {
	    WARNING("could not allocate setting value; using NULL");
	}
	else {
	    strcpy(setting->value, value);
	}
    }
    setting->changed = 1;
}

static void DestroySetting(Setting *setting)
{
    if (setting == NULL) return;

    if (setting->variable != NULL) {
	free(setting->variable);
    }
    if (setting->origin != NULL) {
	free(setting->origin);
    }
    if (setting->value != NULL) {
	free(setting->value);
    }
    free(setting);
}

void DestroySettings(void *s)
{
    Setting *setting;
    Settings *settings = (Settings *)s;
    if (settings == NULL) return;

    setting = settings->head;
    while (setting != NULL) {
	Setting *nextSetting = setting->next;
	DestroySetting(setting);
	setting = nextSetting;
    }

    free(settings);
}

void ReadSettingsFromFile(void *s, const char *filename)
{
    FILE *f;
    char buffer[BLOCKBUSTER_PATH_MAX];
    int lineNumber;
    Settings *settings = (Settings *)s;

    if (settings == NULL) return;

    f = fopen(filename, "r");
    if (f == NULL) {
	/* File doesn't exist, which is okay. */
	return;
    }
    lineNumber = 0;
    while (fgets(buffer, BLOCKBUSTER_PATH_MAX, f) != NULL) {
	char *variable;
	char *value;

	/* Count another line */
	lineNumber++;

	/* Ignore leading whitespace */
	variable = buffer;
	while (isspace(*variable)) variable++;

	/* Ignore the whole line if it starts with a # sign */
	if (*variable == '#') {
	    continue;
	}

	/* Split the string at an equals sign */
	value = variable;
	while (*value && *value != '=') value++;
	if (*value == '=') {
	    char *endOfLine;

	    *value = '\0';
	    value++;

	    /* Eliminate the trailing newline, if any */
	    endOfLine = value;
	    while (*endOfLine != '\0' && *endOfLine != '\n') endOfLine++;
	    if (*endOfLine == '\n') {
		*endOfLine = '\0';
	    }

	    Set(settings, variable, value, filename);
	}
	else {
	    DEBUGMSG("No equals sign found in line %d of file %s:", 
		    lineNumber, filename);
	    DEBUGMSG("\"%s\"", buffer);
	}
    }

    fclose(f);
}

/* The "flags" value will tell which settings to write out to the
 * file.
 */

void WriteSettingsToFile(void *s, const char *filename, unsigned int flags)
{
    Setting *setting;
    Settings *settings = (Settings *)s;
    FILE *f = NULL;

    if (settings == NULL) return;

    /* We'll delay on opening the file until we determine whether
     * we need to, so we don't create a file that we aren't going
     * to put anything into.
     */

    /* Loop through all the settings */
    setting = settings->head;
    while (setting != NULL) {
	/* See if this setting is one of the ones we want. */
	if (
	    ((flags & SETTINGS_CHANGED) && setting->changed) ||
	    ((flags & SETTINGS_FROM_PROGRAM) && setting->origin == NULL) ||
	    ((flags & SETTINGS_FROM_FILE) && setting->origin != NULL && strcmp(setting->origin, filename) == 0) ||
	    ((flags & SETTINGS_FROM_ALL_FILES) && setting->origin != NULL)
	) {
	    /* This setting was requested.  Write it out if it has
	     * a valid value.
	     */
	    if (setting->value != NULL) {
		/* We need to write out the setting (woohoo!).  But
		 * first we must open the file, if we haven't already.
		 */
		if (f == NULL) {
		    f = fopen(filename, "w");
		    if (f == NULL) {
			ERROR("could not open file '%s' for writing", filename);
			return;
		    }
		}

		fprintf(f, "%s=%s\n", setting->variable, setting->value);
	    }
	}

	/* Go on to the next setting */
	setting = setting->next;
    }

    /* Done with the file */
    if (f != NULL) {
	fclose(f);
    }
}

void ChangeSetting(void *s, const char *variable, const char *value)
{
    Settings *settings = (Settings *)s;
    if (settings == NULL) return;

    /* A NULL origin shows that it came from the program while running */
    Set(settings, variable, value, NULL);
}

char *GetSetting(void *s, const char *variable)
{
    Setting *setting;
    Settings *settings = (Settings *)s;
    if (settings == NULL) return NULL;

    setting = FindSetting(settings, variable);
    if (setting == NULL) {
	return NULL;
    }
    else {
	return setting->value;
    }
}

/* This is a special setting operation */
void SetRecentFileSetting(void *settings, const char *filename)
{
    char buffer[100];
    char *recentFileNames[NUM_RECENT_FILES];
    int i;

    /* Look to see if the given filename is already within the
     * list of recent files.
     */
    for (i = 0; i < NUM_RECENT_FILES; i++) {
	sprintf(buffer, "recentFile%d", i);
	recentFileNames[i] = GetSetting(settings, buffer);
	if (recentFileNames[i] != NULL && 
		strcmp(recentFileNames[i], filename) == 0) {
	    /* Match.  Go no further. */
	    return;
	}
    }

    /* No match here.  Slide all the other recent file names down by
     * one, and add ours at the beginning.  The order is important;
     * when we set recentFileX, the pointer to its value will be 
     * freed, so we have to start at the top and work down.
     */
    for (i = NUM_RECENT_FILES; --i > 0; ) {
	sprintf(buffer, "recentFile%d", i);
	ChangeSetting(settings, buffer, recentFileNames[i - 1]);
    }
    ChangeSetting(settings, "recentFile0", filename);
}
