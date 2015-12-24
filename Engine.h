#ifndef _ENGINE_
#define _ENGINE_

#include "Common.h"

class Engine
{
 public:
  virtual void init() = 0;
  virtual void insert_rabbit(Rabbit rabbit) = 0;
  virtual void insert_fox(Fox fox) = 0;
  virtual void insert_rock(Rock rock) = 0;
  virtual void distribute_input() = 0;
  virtual void compute(TInfo inf) = 0;
  virtual void print_gen(int gen) = 0;
  virtual void print_output() = 0;

  virtual void setup_input(int _GEN_PROC_RABBITS, int _GEN_PROC_FOXES, int _GEN_FOOD_FOXES, int _N_GEN, int _R, int _C, int _N) = 0;
  virtual TInfo get_info(int id) = 0;
};

#endif
