#include "Timer.h"
#include "StaticEngine.h"
#include "Engine.h"

int ids[MAX_THREAD];
int n_th = 0, verbose = 0, to_print = 0, to_time = 0, to_type = 0;
int GEN_PROC_RABBITS, GEN_PROC_FOXES, GEN_FOOD_FOXES, N_GEN, R, C, N;
Engine* engine;

void print_help()
{
  printf("usage:\n\tsim -np <np> [arguments]\n\n"
         "Available arguments:\n"
         "\t-h\t\tdisplay this help file\n"
         "\t-np <np>\t\tuse <np> threads\n");
}

void init()
{
  int i;
  for (i = 0; i < n_th; i++)
    ids[i] = i;

  switch (to_type)
  {
    case 0:
      engine = new StaticEngine(verbose, n_th);
      break;
    default:
      engine = new StaticEngine(verbose, n_th);
      break;
  }
  engine->init();
}

void read_input()
{
  scanf("%d %d %d %d %d %d %d", &GEN_PROC_RABBITS, &GEN_PROC_FOXES, &GEN_FOOD_FOXES, &N_GEN, &R, &C, &N);
  engine->setup_input(GEN_PROC_RABBITS, GEN_PROC_FOXES, GEN_FOOD_FOXES, N_GEN, R, C, N);

  int i;
  char object_name[10];
  int cur_x, cur_y;
  for (i = 0; i < N; i++)
  {
    scanf(" %s %d %d", object_name, &cur_y, &cur_x);

    if (strcmp(object_name, "RABBIT") == 0)
    {
      Rabbit obj;
      obj.p_x = cur_x;
      obj.p_y = cur_y;
      obj.age = 0;

      engine->insert_rabbit(obj);
    }
    else if (strcmp(object_name, "FOX") == 0)
    {
      Fox obj;
      obj.p_x = cur_x;
      obj.p_y = cur_y;
      obj.age = 0;
      obj.hunger = 0;

      engine->insert_fox(obj);
    }
    else if (strcmp(object_name, "ROCK") == 0)
    {
      Rock obj;
      obj.p_x = cur_x;
      obj.p_y = cur_y;

      engine->insert_rock(obj);
    }
    else
    {
      fprintf(stderr, "Invalid object in input (line %d)\n", i + 2);
      exit(1);
    }
  }
}

void distribute_input()
{
  engine->distribute_input();
}

void* compute(void* arg)
{
  int id = *((int *) arg);
  TInfo inf = engine->get_info(id);

  engine->compute(inf);
}

void print_output()
{
  engine->print_output();
}

int main(int argc, char* argv[])
{
  pthread_t *threads;
  pthread_attr_t pthread_custom_attr;

  int i;
  for (i = 1; i < argc; i++)
    if (strcmp(argv[i], "-v") == 0)
      verbose = 1;
    else if (strcmp(argv[i], "-pr") == 0)
      to_print = 1;
    else if (strcmp(argv[i], "-tm") == 0)
      to_time = 1;
    else if (strcmp(argv[i], "-np") == 0)
    {
      n_th = atoi(argv[i + 1]);
      i++;
    }
    else if (strcmp(argv[i], "-al") == 0)
    {
      to_type = atoi(argv[i + 1]);
      i++;
    }
    else if (strcmp(argv[i], "-h") == 0)
    {
      print_help();
      exit(0);
    }

  if ((n_th < 1) || (n_th > MAX_THREAD - 1))
  {
    printf("The number of thread should between 1 and %d\n", MAX_THREAD - 1);
    exit(1);
  }

  init();
  read_input();

  Timer::start();
  distribute_input();

  threads = (pthread_t *)malloc(n_th * sizeof(*threads));
  pthread_attr_init(&pthread_custom_attr);

  for (i = 0; i < n_th; i++)
    pthread_create(&threads[i], &pthread_custom_attr, compute, (void *)(ids + i));

  for (i = 0; i < n_th; i++)
    pthread_join(threads[i], NULL);
  free(threads);

  Timer::stop();

  if (to_print)
    print_output();

  if (to_time)
    printf("Computation Time (ms): %0.4lf\n", Timer::elapsed());

  return 0;
}
