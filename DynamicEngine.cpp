#include "DynamicEngine.h"

DynamicEngine::DynamicEngine(int _verbose, int _n_th)
{
  verbose = _verbose;
  n_th = _n_th;
}

void DynamicEngine::init()
{
}

void DynamicEngine::insert_rabbit(Rabbit rabbit)
{
}

void DynamicEngine::insert_fox(Fox fox)
{
}

void DynamicEngine::insert_rock(Rock rock)
{
}

void DynamicEngine::distribute_input()
{
}

void DynamicEngine::compute(TInfo inf)
{

}

void DynamicEngine::print_gen(int gen)
{
}

void DynamicEngine::print_output()
{
  //printf("%d %d %d %d %d %d %d\n", GEN_PROC_RABBITS, GEN_PROC_FOXES, GEN_FOOD_FOXES, 0, R, C, ct);
}

void DynamicEngine::setup_input(int _GEN_PROC_RABBITS, int _GEN_PROC_FOXES, int _GEN_FOOD_FOXES, int _N_GEN, int _R, int _C, int _N)
{
  GEN_PROC_RABBITS = _GEN_PROC_RABBITS;
  GEN_PROC_FOXES = _GEN_PROC_FOXES;
  GEN_FOOD_FOXES = _GEN_FOOD_FOXES;
  N_GEN = _N_GEN;
  R = _R;
  C = _C;
  N = _N;
}

TInfo DynamicEngine::get_info(int id)
{
  return th_info[id];
}
