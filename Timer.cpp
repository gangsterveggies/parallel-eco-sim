#include "Timer.h"

// Static variables
struct timeval Timer::cstart;
struct timeval Timer::cend;

// Stop the clock of a timer
void Timer::start()
{
  gettimeofday(&cstart, NULL);
}

// Elapsed time of a timer
void Timer::stop()
{
  gettimeofday(&cend, NULL);
}

double Timer::elapsed()
{
  return (cend.tv_sec - cstart.tv_sec) + (double)(cend.tv_usec - cstart.tv_usec)/1000000.0;
}
