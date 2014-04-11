/*
  #
  # $RCSfile: main.cpp,v $
  # $Name:  $
  # 
  # ASCI Visualization Project 
  #
  # Lawrence Livermore National Laboratory
  # Information Management and Graphics Group
  # P.O. Box 808, Mail Stop L-561
  # Livermore, CA 94551-0808
  #
  # For information about this project see:
  # 	http://www.llnl.gov/sccd/lc/img/ 
  #
  # 	or contact: asciviz@llnl.gov
  #
  # For copyright and disclaimer information see:
  #	$(ASCIVIS_ROOT)/copyright_notice_1.txt
  #
  # $Id: main.cpp,v 1.19 2009/04/10 01:45:30 wealthychef Exp $
  #
  #   Abstract: The main() for the sidecar Qt application
  #
  #   Author: Rich Cook
  #
  # This work performed under the auspices of the U.S. Department of Energy by Lawrence Livermore National Laboratory under Contract DE-AC52-07NA27344.
  # This document was prepared as an account of work sponsored by an agency of the United States government. Neither the United States government nor Lawrence Livermore National Security, LLC, nor any of their employees makes any warranty, expressed or implied, or assumes any legal liability or responsibility for the accuracy, completeness, or usefulness of any information, apparatus, product, or process disclosed, or represents that its use would not infringe privately owned rights. Reference herein to any specific commercial product, process, or service by trade name, trademark, manufacturer, or otherwise does not necessarily constitute or imply its endorsement, recommendation, or favoring by the United States government or Lawrence Livermore National Security, LLC. The views and opinions of authors expressed herein do not necessarily state or reflect those of the United States government or Lawrence Livermore National Security, LLC, and shall not be used for advertising or product endorsement purposes.
  #
*/

#include "sidecar.h"
#include "QApplication" 
#include "QDir"
#include <QCleanlooksStyle>
#include <QPlastiqueStyle>
#include <QHostInfo>
#include "settings.h"
#include "common.h"
#include "../../config/version.h"
#include "Prefs.h"
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <boost/filesystem.hpp> 
#include <boost/program_options.hpp>

using namespace std; 
using namespace boost::program_options; 

void usage(void) {
  cerr << "Usage:  sidecar [options] [cuefile]" << endl; 
  cerr << "" << endl;
  cerr << "BASIC OPTIONS: " << endl;
  cerr << "--help/-h:  display this menu" << endl;
  cerr << "--keyhelp/-k:  display list of keyboard controls" << endl;
  cerr << "--dmx/-d:  by default, set the dmx checkbox when running blockbuster" << endl;
  cerr << "--movie/-m filename[@host]:  launch blockbuster and open the given movie on the given host.  If not host is given, blockbuster will run locally" << endl;
  cerr << "--play/-p  filename[@host]:  Same as -movie, but play the movie when you load it." << endl;
  cerr << "--rsh/-r rshcmd:  connect to remote hosts using the given command instead of rsh.  Note that password prompts appear on the command line. "<< endl; 
  cerr << "--verbose/-v num: verbosity level"<< endl; 
  cerr << "" << endl;
  cerr << "ADVANCED OPTIONS:" << endl;
  cerr << "--stresstest/-s:  (DEBUGGING ONLY) -- execute each cue numerous times instead of just once when clicked" << endl;
  cerr << "" << endl;
}

string GetSidecarDir(string myname) {
  if (!myname.size()) {
    myname="sidecar";
  }
  if (myname[0] != '/') {
    long pathsize =  pathconf(".", _PC_PATH_MAX);
    vector<char> pathbuf(pathsize, 0); 
    myname = string(getcwd(&pathbuf[0], pathsize-1)) + "/" +  myname; 
  }
  boost::filesystem::path p(myname);
  boost::filesystem::path dir = p.parent_path();
  return dir.string(); 
}

void ParseOptions(int &argc, char *argv[], Preferences &gPrefs) {

  gPrefs.SetValue("rsh", "rsh"); 
  gPrefs.SetValue("verbose", 0); 
  gPrefs.SetValue("sidecarDir", GetSidecarDir(argv[0])); 
  gPrefs.ReadFromFile(false); 

  gPrefs.DeleteValue("verbose"); // do not inherit this from previous
  gPrefs.DeleteValue("movie"); // do not inherit this from previous
  gPrefs.DeleteValue("play"); // do not inherit this from previous

  gPrefs.ReadFromEnvironment(); 

  vector<argType> args; 
  args.push_back(argType("help", "bool")); 
  args.push_back(argType("keyhelp", "bool")); 
  args.push_back(argType("dmx", "bool")); 
  args.push_back(argType("movie", "string")); 
  args.push_back(argType("play", "string")); 
  args.push_back(argType("--profile", "-p", "SIDECAR_DEFAULT_PROFILE", "string")); 
  args.push_back(argType("rsh", "string")); 
  args.push_back(argType("stresstest", "bool")); 
  args.push_back(argType("verbose", "long")); 
  gPrefs.SetValidArgs(args); 
  gPrefs.GetFromArgs(argc, argv, args); 
}

int main(int argc, char *argv[]) {
  // Try new argument parsing: 
  // ========================================================
  options_description general("General options");
  general.add_options()
    ("help,h", "Output this menu")
    ("help-advanced,a", "Output this menu plus advanced options")
    ("keyhelp,k", "display list of keyboard controls")
    ("dmx,d", "by default, set the dmx checkbox when running blockbuster")
    ("movie,m", value<string>(), "arg is either a filename or a string of the form filename[@host].  Launch blockbuster and open the given movie on the given host without playing it.  If not host is given, blockbuster will run locally")
    ("play,p", value<string>(), "Same as --movie, but play the movie when you load it.")
    ("rsh,r", value<string>(), "connect to remote hosts using the given command 'arg' instead of rsh (example: --rsh=ssh).  Note that password prompts appear on the command line, not in the GUI. ")
    ("verbose,v", value<int>(),  "verbosity level (0-5)") ;

  // ========================================================
  options_description advanced("Advanced options"); 
  advanced.add_options()
    ("stresstest,s", "(DEBUGGING ONLY) -- execute each cue numerous times instead of just once when clicked"); 

  // ========================================================
  options_description positional_desc("Positional"); 
  positional_desc.add_options()
    ("cuefile", value<string>(), "Cue file name"); 

  positional_options_description positional_obj_desc; 
  positional_obj_desc.add("cuefile", 1); 

  
  // ========================================================
  options_description visible("Allowed options");
  visible.add(general); 

  // ========================================================
  options_description all; 
  all.add(general).add(advanced).add(positional_desc); 

  variables_map vm;
  store(command_line_parser(argc, argv).options(all).positional(positional_obj_desc).run(), vm);
  
  // ========================================================
  cerr << "sidecar version " << BLOCKBUSTER_VERSION << endl; 
  QApplication app(argc, argv);
  QString prefsdir = QDir::homePath() + "/.sidecar"; 
  mkdir(prefsdir.toStdString().c_str(), 0777); 
  Preferences gPrefs; 
  gPrefs.SetValue("prefsdir", prefsdir.toStdString()); 
  gPrefs.SetFile((prefsdir + "/prefs.cnf").toStdString()); 
  SideCar sidecar(&app, &gPrefs);
  //QStyle *myStyle = new QCleanlooksStyle();
  QStyle *myStyle = new QPlastiqueStyle();
  app.setStyle(myStyle); 
  sidecar.show();


  try {
    ParseOptions(argc, argv, gPrefs); 
  } catch (string err) {
    cerr << "Error in ParseOptions: " << err; 
    exit(1); 
  }
  if (gPrefs.GetBoolValue("help")) {
    usage(); 
    exit(0); 
  }
  if (gPrefs.GetBoolValue("keyhelp")) {
    cerr << "When remote keyboard control is active in sidecar, the following keystrokes will operate on blockbuster." << endl; 
    PrintKeyboardControls(); 
    exit(0); 
  }
  set_verbose(gPrefs.GetLongValue("verbose")); 

  // if nothing from environment, prefs, or args, then try local host name as default profile
  if (gPrefs.GetValue("SIDECAR_DEFAULT_PROFILE") == "") {
    gPrefs.SetValue("SIDECAR_DEFAULT_PROFILE",  
                    QHostInfo::localHostName().toStdString()); 
  }

  if (argc > 1) { 
    sidecar.ReadCueFile(argv[1]);
  }
  { 
    string moviename; 
    if ((gPrefs.TryGetValue("movie", moviename) ||
         gPrefs.TryGetValue("play", moviename) ) 
        && moviename != "(nil)") {  
      sidecar.askLaunchBlockbuster(NULL, moviename.c_str(), true); 
    }
  }
  int retval = app.exec();
  gPrefs.DeleteValue("verbose"); // do not inherit this from previous
  gPrefs.DeleteValue("movie"); // do not inherit this from previous
  gPrefs.DeleteValue("play"); // do not inherit this from previous
  gPrefs.SaveToFile(true, true); 
  return retval; 
}
