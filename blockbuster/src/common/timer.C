#include <cstdio>
#include "timer.h"
using namespace std;

/*!
  Progress(theTimer, nodenum, numnodes, thePercent, 5, theTime, 60, "Reading dumpfile"); 
 if either delta is exceeded, increase oPercent & oTime and report the progress
 return 1 if something is printed out, else return 0 
*/
int Progress(timer &iTimer, double iNum, double iMax, 
             double &oPercent, double iPercentDelta, 
             double &oTime, double iTimeDelta,  string iMsg) {
  double percent = iNum/iMax*100.0;
  double newtime = iTimer.elapsed_time();
  if (iNum == iMax || percent - oPercent > iPercentDelta || newtime-oTime > iTimeDelta) {      
    double remaining = (iMax-iNum)/(iNum!=0?iNum:1)*newtime;
    double persec= iNum/newtime;
    char msg[4096]; 
    sprintf(msg, "\r%s: %6.5g%%, %8g/%-8g, %6.5g secs, %8.5g/sec, %8.5g sec left", /*datestring(), */iMsg.c_str(), percent, iNum, iMax, newtime, persec, remaining);
    cerr << msg << flush;
    oPercent=percent;
    oTime=newtime; 
    return 1; 
  }
  return 0; 
}

// __________________________________________________________________________

double timer::total_time(void) {
  return acc_time + elapsed_time(); 
}
// __________________________________________________________________________
// Return the time that has passed since the timer was started last.  If the
// total time is less than a minute, the time is reported to two decimal
// places.

double timer::elapsed_time(void)
{  
  if (running) {
    if (mUseWallTime) {
      return (time(NULL) - start_clock); 
    } else {
      return GetExactSeconds() - start_clock; 
      //      return (((double)clock() - start_clock) * time_per_tick);
    }
  }
  return 0; 

} // timer::elapsed_time

// __________________________________________________________________________
// Start a timer.  If it is already running, let it continue running.
// Print an optional message.

void timer::start(const char* msg)
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
  if (mUseWallTime) {
    start_clock = time(NULL); 
  } else {
    start_clock=GetExactSeconds(); 
    //    start_clock = clock();
  }
  
} // timer::start

// __________________________________________________________________________
// Turn the timer off and start it again from 0.  Print an optional message.

void timer::restart(const char* msg)
{
  // Report the timer message
  if (msg)
    std::cout << msg << std::endl;

  // Set the timer status to running
  running = true;

  // Set the accumulated time to 0 and the start time to now
  acc_time = 0;
  if (mUseWallTime) {
    start_clock = time(NULL); 
  } else {
    start_clock = GetExactSeconds(); 
    //  start_clock = clock();
  }

} // timer::restart

// __________________________________________________________________________
// Stop the timer and print an optional message.

void timer::stop(const char* msg)
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

std::ostream& operator<<(std::ostream& os, timer& t)
{
  /*std::ios::fmtflags saveFlags = os.flags(); 
  long savePrec = os.precision(); 
  return os << std::setprecision(4) << std::setiosflags(std::ios::fixed)
  << t.total_time() << std::setprecision(savePrec) << std::setiosflags(saveFlags); */
  return os << t.total_time();
}
