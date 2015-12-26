#ifndef _STENGINE_
#define _STENGINE_

#include "Common.h"
#include "Engine.h"

class TopologyEngine : public Engine
{
 public:
  TopologyEngine(int _verbose, int _n_th);

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

  pthread_mutex_t th_locks[MAX_THREAD];
  pthread_barrier_t barrier;
  int n_th = 0, verbose;
  queue<Fox> fox_queues[MAX_THREAD];
  queue<Rabbit> rabbit_queues[MAX_THREAD];
  int owner[MAX_SIZE][MAX_SIZE];

  int dx[4] = {0, 1, 0, -1};
  int dy[4] = {-1, 0, 1, 0};
  int pos_grid[MAX_SIZE][MAX_SIZE];
  int age_grid[MAX_SIZE][MAX_SIZE];
  int hun_grid[MAX_SIZE][MAX_SIZE];
  int tmp_pos_grid[MAX_SIZE][MAX_SIZE];
  int tmp_age_grid[MAX_SIZE][MAX_SIZE];
  int tmp_hun_grid[MAX_SIZE][MAX_SIZE];

  void replace_rabbit(Rabbit rabbit, TInfo inf);
  void replace_fox(Fox fox, TInfo inf);
};

#endif
