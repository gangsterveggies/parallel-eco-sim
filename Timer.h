#ifndef _TIMER_
#define _TIMER_

#include <sys/time.h>
#include <stdlib.h>

class Timer
{
 private:
  static struct timeval cstart; // Start times
  static struct timeval cend; // End times

 public:
  static void start(); // Start the clock of a timer
  static void stop(); // Stop the clock of a timer
  static double elapsed(); // Elapsed time of a timer
};

#endif
