#ifndef TIMER_H
#define TIMER_H

/* thanks to
   Kenneth Wilder
   Department of Statistics
   The University of Chicago
   5734 South University Avenue
   Chicago, IL 60637 
   
   Office: Eckhart 105
   Phone: (773) 702-8325
   Email: wilder@galton.uchicago.edu
   http://oldmill.uchicago.edu/~wilder/

   example usage: 
 int main()
{
  timer t;

  // Start the timer and then report the time required to
  // run a big loop.  The string argument to start() is
  // optional.
  t.start("Timer started");
  for (int j = 0; j < 2; ++j)
    for (unsigned int i = 0; i < 2000000000; ++i)
      ;
  cout << "Check 1: " << t << endl;

  // Restart the timer and time another loop.
  t.restart("Timer restarted");
  for (int i = 0; i < 1000000000; ++i)
    ;
  cout << "Check 2: " << t << endl;

  // Stop the timer and repeat the loop.  The third
  // timer check should report the same value as the
  // second.
  t.stop("Timer stopped");
  for (int i = 0; i < 1000000000; ++i)
    ;
  cout << "Check 3: " << t << endl;

  // Start the timer again.  Since there is no restart,
  // the timer will start from where it left off the
  // last time it was stopped.
  t.start("Timer started, not restarted");
  for (int i = 0; i < 1000000000; ++i)
    ;
  // The 'check' member function gives a default
  // method of printing the current elapsed time.
  t.check("Check 4");

  cout << "Now sleeping for 2 hours...\n";
  sleep(60*60*2);
  cout << "Check 5: " << t << endl;


  return 0;
}

  
*/

#include <ctime>
#include <sys/time.h>
#include <iostream>
#include <iomanip>
#include <string>
#include "stringutil.h"

//===============================================
// Here is a nice quick way to do it, which does not require a global member 

inline double GetExactSecondsDouble(void) {
  struct timeval t; 
  gettimeofday(&t, NULL); 
  return t.tv_sec + (double)t.tv_usec/1000000.0; 
}

inline std::string GetExactSeconds(void) {
  //return QTime::currentTime().toString("ssss.zzz").toStdString(); 
  std::string s = doubleToString(GetExactSecondsDouble(), 3);
  int first = s.size() -8; 
  if (first<0) first = 0; 
  return s.substr(first, 8); 
}

// And here is my class-based method, which I stole from someone else.  
// Example: 
//  cerr << gTimer.mHostname << ": " << name ", line " << __LINE__ << ": " << gTimer << endl;  

int Progress(class timer &iTimer, double iNum, double iMax, 
			 double &oPercent, double iPercentDelta, 
			 double &oTime, double iTimeDelta, char *iMsg);


using namespace std; 
class timer
{
 public:
  // 'running' is initially false.  A timer needs to be explicitly started
  // using 'start' or 'restart'
  timer() : running(false), start_clock(0),  acc_time(0) {
    time_per_tick = 1.0/(double)CLOCKS_PER_SEC;
    char buf[2048]; 
    if (gethostname(buf, 2047) != -1) 
      mHostname = buf; 
    else
      mHostname = "unknown host"; 
    return; 
  }
    static double getExactSeconds(void) {
      struct timeval t; 
      gettimeofday(&t, NULL); 
      return t.tv_sec + (double)t.tv_usec/1000000.0; 
    }
  double elapsed_time(void);
  double total_time(void);
  void start(const char* msg = 0);
  void restart(const char* msg = 0);
  void stop(const char* msg = 0);
  void check(const char* msg = 0);
 friend std::ostream& operator<<(std::ostream& os, timer& t);
 string mHostname; 
 private:
  bool running;
  double start_clock;
  double acc_time;
  double time_per_tick; 

}; // class timer

// __________________________________________________________________________

inline double timer::total_time(void) {
  return acc_time + elapsed_time(); 
}
// __________________________________________________________________________
// Return the time that has passed since the timer was started last.  If the
// total time is less than a minute, the time is reported to two decimal
// places.

inline double timer::elapsed_time(void)
{  
  if (running) {
    return getExactSeconds() - start_clock; 
  }

  return 0; 

} // timer::elapsed_time

// __________________________________________________________________________
// Start a timer.  If it is already running, let it continue running.
// Print an optional message.

inline void timer::start(const char* msg)
{
  // Report the timer message
  if (msg)
    std::cout << msg << std::endl;
  
  // Return immediately if the timer is already running
  if (running)
    return;
  
  // Change timer status to running
  running = true;
  
  // Set the start time;
  start_clock=getExactSeconds(); 
  
} // timer::start

// __________________________________________________________________________
// Turn the timer off and start it again from 0.  Print an optional message.

inline void timer::restart(const char* msg)
{
  // Report the timer message
  if (msg)
    std::cout << msg << std::endl;
  
  // Set the timer status to running
  running = true;
  
  // Set the accumulated time to 0 and the start time to now
  acc_time = 0;
  start_clock = getExactSeconds(); 
  
} // timer::restart

// __________________________________________________________________________
// Stop the timer and print an optional message.

inline void timer::stop(const char* msg)
{
  // Report the timer message
  if (msg)
    std::cout << msg << std::endl;
  
  // Recalculate and store the total accumulated time up until now
  if (running)
    acc_time += elapsed_time();
  
  running = false;
  
} // timer::stop


// __________________________________________________________________________
// Allow timers to be printed to ostreams using the syntax 'os << t'
// for an ostream 'os' and a timer 't'.

inline std::ostream& operator<<(std::ostream& os, timer& t)
{
  /*std::ios::fmtflags saveFlags = os.flags(); 
    long savePrec = os.precision(); 
    return os << std::setprecision(4) << std::setiosflags(std::ios::fixed)
    << t.total_time() << std::setprecision(savePrec) << std::setiosflags(saveFlags); */
  return os << t.total_time();
}

#endif // TIMER_H

