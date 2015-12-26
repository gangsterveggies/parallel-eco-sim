#ifndef _COMMON_
#define _COMMON_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <pthread.h>
#include <vector>
#include <queue>
#include <algorithm>

using namespace std;

#define MAX_THREAD 65
#define MAX_SIZE 1100
//#define MAX_SIZE 250

#define EMPTY_ID 0
#define RABBIT_ID 1
#define FOX_ID 2
#define ROCK_ID 3

typedef pair<int, int> Position;
#define pX first
#define pY second

struct TInfo
{
  int id;
  int st_x, st_y;
  int fn_x, fn_y;
} typedef TInfo;

struct Rabbit
{
  int p_x, p_y;
  int age;

  bool operator < (const Rabbit& rb) const
  {
    return age > rb.age;
  }
} typedef Rabbit;

struct Fox
{
  int p_x, p_y;
  int age, hunger;

  bool operator < (const Fox& fx) const
  {
    if (age == fx.age)
      return hunger < fx.hunger;
    return age > fx.age;
  }
} typedef Fox;

struct Rock
{
  int p_x, p_y;
} typedef Rock;

#endif
