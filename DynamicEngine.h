#ifndef _DNENGINE_
#define _DNENGINE_

#include "Common.h"
#include "Engine.h"

class DynamicEngine : public Engine
{
 public:
  DynamicEngine(int _verbose, int _n_th);

  void init();
  void insert_rabbit(Rabbit rabbit);
  void insert_fox(Fox fox);
  void insert_rock(Rock rock);
  void distribute_input();
  void compute(TInfo inf);
  void print_gen(int gen);
  void print_output();

  void setup_input(int _GEN_PROC_RABBITS, int _GEN_PROC_FOXES, int _GEN_FOOD_FOXES, int _N_GEN, int _R, int _C, int _N);
  TInfo get_info(int id);

 private:
  int GEN_PROC_RABBITS, GEN_PROC_FOXES, GEN_FOOD_FOXES, N_GEN, R, C, N;
  TInfo th_info[MAX_THREAD];

  int n_th = 0, verbose;
  int dx[4] = {0, 1, 0, -1};
  int dy[4] = {-1, 0, 1, 0};
};

#endif
