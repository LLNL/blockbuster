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
#include <algorithm>
#include <boost/format.hpp>
using namespace std; 

static ProgramOptions gProgramOptions ; // global options

ProgramOptions *GetGlobalOptions (void) { 
  return &gProgramOptions; 
}

void printargv(int argc, char *argv[]) {
  dbprintf(0, "argv is: {"); 
  int argnum = 0; 
  while (argnum < argc) {
    dbprintf(0, argv[argnum]); 
    if (argnum < argc-1) {
      dbprintf(0, ", "); 
    } else {
      dbprintf(0, "}\n"); 
    }
    argnum ++; 
  }
}
/* "consume" the first arg in argv by shifting all later args one to the left */ 
void ConsumeArg(int &argc, char *argv[], int position) {
  //  dbprintf(0, "Before ConsumeArg(%d, argv, %d): ", argc, position); 
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
  //dbprintf(0, "After ConsumeArg: "); 
  //printargv(argc, argv); 
  return; 
}


void *CreateBlankSettings(void)
{
    Settings *settings;
    settings = new Settings; 
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
      if (setting->variable == variable) {
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
    if (setting == NULL) {
	/* We have to create our own setting.  Set up the
	 * structure and link it in; we'll allocate the 
	 * value later.
	 */
      setting = new Setting;
      if (setting == NULL) {
	    WARNING("Could not allocate new setting");
	    return;
      }
    setting->variable = variable; 
      if (origin == NULL) {
	    setting->origin = "";
      }
      else {
       setting->origin = origin;
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
       setting->value = value;
    }
    setting->changed = 1;
}

static void DestroySetting(Setting *setting)
{
    if (setting == NULL) return;

    delete setting;
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

    delete settings; 
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
	    while (*endOfLine) {
          if (*endOfLine == '\n') 
            *endOfLine = '\t';
          ++endOfLine;
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

void WriteSettingsToFile(void *s, string filename, unsigned int flags)
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
	    ((flags & SETTINGS_FROM_PROGRAM) && setting->origin == "") ||
	    ((flags & SETTINGS_FROM_FILE) && setting->origin == filename) ||
	    ((flags & SETTINGS_FROM_ALL_FILES) && setting->origin != "")
	) {
	    /* This setting was requested.  Write it out if it has
	     * a valid value.
	     */
	    if (setting->value != "") {
		/* We need to write out the setting (woohoo!).  But
		 * first we must open the file, if we haven't already.
		 */
		if (f == NULL) {
          f = fopen(filename.c_str(), "w");
		    if (f == NULL) {
              ERROR("could not open file '%s' for writing", filename.c_str());
			return;
		    }
		}
        string cleaned = setting->value; 
        std::replace(cleaned.begin(), cleaned.end(), '\n', '\t'); 
		fprintf(f, "%s=%s\n", setting->variable.c_str(), cleaned.c_str());
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

void ChangeSetting(void *s, string variable, string value)
{
    Settings *settings = (Settings *)s;
    if (settings == NULL) return;

    /* A NULL origin shows that it came from the program while running */
    Set(settings, variable.c_str(), value.c_str(), NULL);
}

string GetSetting(void *s, string variable)
{
    Setting *setting;
    Settings *settings = (Settings *)s;
    if (settings == NULL) return "";

    setting = FindSetting(settings, variable.c_str());
    if (setting == NULL) {
      return "";
    }

    return setting->value;
     
}

/* This is a special setting operation */
void SetRecentFileSetting(void *settings, const char *filename)
{
  //char buffer[100];
    vector<string> recentFileNames; 
    recentFileNames.push_back(filename); 

    uint32_t i;

    /* Look to see if the given filename is already within the
     * list of recent files.
     */
    for (i = 0; i < NUM_RECENT_FILES; i++) {
      string found =  str(boost::format("recentFile%d")%i);   
      found = GetSetting(settings, found); 
      if (found == filename) {
	    /* Match.  Go no further. */
	    return;
      }
      if (found == "") 
        break; // out of items, stop.
      recentFileNames.push_back(found);
    }
    /* No match.  Store the new list of filenames
     */
    for (i = 0; i< NUM_RECENT_FILES && i<recentFileNames.size(); i++) {      
      ChangeSetting(settings, str(boost::format("recentFile%d")%i), recentFileNames[i]);
    }
    return; 
    
}
