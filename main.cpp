#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <vector>
#include <queue>
#include <algorithm>
#include "Timer.h"

using namespace std;

#define MAX_THREAD 65
#define MAX_SIZE 250

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

TInfo th_info[MAX_THREAD];
pthread_mutex_t th_locks[MAX_THREAD];
pthread_barrier_t barrier;
int n_th = 0, verbose = 0, to_print = 0, to_time = 0;
queue<Fox> fox_queues[MAX_THREAD];
queue<Rabbit> rabbit_queues[MAX_THREAD];

int dx[4] = {0, 1, 0, -1};
int dy[4] = {-1, 0, 1, 0};
int GEN_PROC_RABBITS, GEN_PROC_FOXES, GEN_FOOD_FOXES, N_GEN, R, C, N;
int pos_grid[MAX_SIZE][MAX_SIZE];
int age_grid[MAX_SIZE][MAX_SIZE];
int hun_grid[MAX_SIZE][MAX_SIZE];
int tmp_pos_grid[MAX_SIZE][MAX_SIZE];
int tmp_age_grid[MAX_SIZE][MAX_SIZE];
int tmp_hun_grid[MAX_SIZE][MAX_SIZE];

void print_gen(int gen);

void print_help()
{
  printf("usage:\n\tsim -np <np> [arguments]\n\n"
         "Available arguments:\n"
         "\t-h\t\tdisplay this help file\n"
         "\t-np <np>\t\tuse <np> threads\n");
}

void init()
{
  memset(pos_grid, 0, sizeof pos_grid);
  memset(age_grid, 0, sizeof age_grid);
  memset(hun_grid, 0, sizeof hun_grid);

  int i;
  for (i = 0; i < n_th; i++)
    pthread_mutex_init(&(th_locks[i]), NULL);

  pthread_barrier_init(&barrier, NULL, n_th);
}

void insert_rabbit(Rabbit rabbit)
{
  pos_grid[rabbit.p_y][rabbit.p_x] = RABBIT_ID;
  age_grid[rabbit.p_y][rabbit.p_x] = rabbit.age;
}

void insert_fox(Fox fox)
{
  pos_grid[fox.p_y][fox.p_x] = FOX_ID;
  age_grid[fox.p_y][fox.p_x] = fox.age;
  hun_grid[fox.p_y][fox.p_x] = fox.hunger;
}

void insert_rock(Rock rock)
{
  pos_grid[rock.p_y][rock.p_x] = ROCK_ID;
}

void read_input()
{
  scanf("%d %d %d %d %d %d %d", &GEN_PROC_RABBITS, &GEN_PROC_FOXES, &GEN_FOOD_FOXES, &N_GEN, &R, &C, &N);

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

      insert_rabbit(obj);
    }
    else if (strcmp(object_name, "FOX") == 0)
    {
      Fox obj;
      obj.p_x = cur_x;
      obj.p_y = cur_y;
      obj.age = 0;
      obj.hunger = 0;

      insert_fox(obj);
    }
    else if (strcmp(object_name, "ROCK") == 0)
    {
      Rock obj;
      obj.p_x = cur_x;
      obj.p_y = cur_y;

      insert_rock(obj);
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
  int i, acc = 0;
  for (i = 0; i < n_th; i++)
  {
    th_info[i].id = i;
    th_info[i].st_x = 0;
    th_info[i].fn_x = C - 1;
    th_info[i].st_y = acc;
    acc += R / n_th + ((R - n_th * (R / n_th)) > i);
    th_info[i].fn_y = acc - 1;
  }
}

void replace_rabbit(Rabbit rabbit, TInfo inf)
{
  if (!(rabbit.p_x >= inf.st_x &&
        rabbit.p_x <= inf.fn_x &&
        rabbit.p_y >= inf.st_y &&
        rabbit.p_y <= inf.fn_y))
  {
    int i;
    for (i = 0; i < n_th; i++)
    {
      TInfo n_inf = th_info[i];
      if ((rabbit.p_x >= n_inf.st_x &&
           rabbit.p_x <= n_inf.fn_x &&
           rabbit.p_y >= n_inf.st_y &&
           rabbit.p_y <= n_inf.fn_y))
        break;
    }

    pthread_mutex_lock(&(th_locks[i]));
    rabbit_queues[i].push(rabbit);
    pthread_mutex_unlock(&(th_locks[i]));

    return;
  }

  int larger_flag = 1;
  if (tmp_pos_grid[rabbit.p_y][rabbit.p_x] == RABBIT_ID)
  {
    Rabbit cur_rabbit;
    cur_rabbit.age = tmp_age_grid[rabbit.p_y][rabbit.p_x];
    larger_flag = rabbit < cur_rabbit;
  }

  if (!larger_flag)
    return;

  tmp_pos_grid[rabbit.p_y][rabbit.p_x] = RABBIT_ID;
  tmp_age_grid[rabbit.p_y][rabbit.p_x] = rabbit.age;
}

void replace_fox(Fox fox, TInfo inf)
{
  if (!(fox.p_x >= inf.st_x &&
        fox.p_x <= inf.fn_x &&
        fox.p_y >= inf.st_y &&
        fox.p_y <= inf.fn_y))
  {
    int i;
    for (i = 0; i < n_th; i++)
    {
      TInfo n_inf = th_info[i];
      if ((fox.p_x >= n_inf.st_x &&
           fox.p_x <= n_inf.fn_x &&
           fox.p_y >= n_inf.st_y &&
           fox.p_y <= n_inf.fn_y))
        break;
    }

    pthread_mutex_lock(&(th_locks[i]));
    fox_queues[i].push(fox);
    pthread_mutex_unlock(&(th_locks[i]));

    return;
  }

  int larger_flag = 1;
  if (tmp_pos_grid[fox.p_y][fox.p_x] == FOX_ID)
  {
    Fox cur_fox;
    cur_fox.age = tmp_age_grid[fox.p_y][fox.p_x];
    cur_fox.hunger = tmp_hun_grid[fox.p_y][fox.p_x];
    larger_flag = fox < cur_fox;
  }

  if (!larger_flag)
    return;

  tmp_pos_grid[fox.p_y][fox.p_x] = FOX_ID;
  tmp_age_grid[fox.p_y][fox.p_x] = fox.age;
  tmp_hun_grid[fox.p_y][fox.p_x] = fox.hunger;
}

void *static_compute(void *arg)
{
  TInfo inf = *((TInfo *)arg);

  int iter, i, i_x, i_y;
  for (iter = 0; iter < N_GEN; iter++)
  {
    // Sync threads
    pthread_barrier_wait(&barrier);
    if (verbose && inf.id == 0)
      print_gen(iter);

    // Simulate Rabbits
    for (i_y = inf.st_y; i_y <= inf.fn_y; i_y++)
      for (i_x = inf.st_x; i_x <= inf.fn_x; i_x++)
      {
        tmp_pos_grid[i_y][i_x] = 0;
        tmp_age_grid[i_y][i_x] = 0;
      }

    for (i_y = inf.st_y; i_y <= inf.fn_y; i_y++)
      for (i_x = inf.st_x; i_x <= inf.fn_x; i_x++)
        if (pos_grid[i_y][i_x] == RABBIT_ID)
        {
          vector<Position> candidate_moves;

          for (i = 0; i < 4; i++)
          {
            int nx = i_x + dx[i];
            int ny = i_y + dy[i];

            if ((nx >= 0 && nx < C && ny >= 0 && ny < R) && pos_grid[ny][nx] == EMPTY_ID)
              candidate_moves.push_back(Position(nx, ny));
          }

          if (candidate_moves.empty())
          {
            tmp_pos_grid[i_y][i_x] = pos_grid[i_y][i_x];
            tmp_age_grid[i_y][i_x] = age_grid[i_y][i_x] + 1;

            Rabbit n_rabbit;
            n_rabbit.p_x = i_x;
            n_rabbit.p_y = i_y;
            n_rabbit.age = age_grid[i_y][i_x] + 1;
            replace_rabbit(n_rabbit, inf);
          }
          else
          {
            int proc_flag = age_grid[i_y][i_x] >= GEN_PROC_RABBITS;

            if (proc_flag)
            {
              Rabbit n_rabbit;
              n_rabbit.p_x = i_x;
              n_rabbit.p_y = i_y;
              n_rabbit.age = 0;
              replace_rabbit(n_rabbit, inf);
            }

            Position n_pos = candidate_moves[(iter + i_x + i_y) % ((int) candidate_moves.size())];
            Rabbit n_rabbit;
            n_rabbit.p_x = n_pos.pX;
            n_rabbit.p_y = n_pos.pY;
            n_rabbit.age = (proc_flag ? 0 : age_grid[i_y][i_x] + 1);
            replace_rabbit(n_rabbit, inf);
          }
        }

    // Sync threads
    pthread_barrier_wait(&barrier);

    while (!rabbit_queues[inf.id].empty())
    {
      replace_rabbit(rabbit_queues[inf.id].front(), inf);
      rabbit_queues[inf.id].pop();
    }

    for (i_y = inf.st_y; i_y <= inf.fn_y; i_y++)
      for (i_x = inf.st_x; i_x <= inf.fn_x; i_x++)
        if (tmp_pos_grid[i_y][i_x] == RABBIT_ID)
        {
          pos_grid[i_y][i_x] = tmp_pos_grid[i_y][i_x];
          age_grid[i_y][i_x] = tmp_age_grid[i_y][i_x];
        }
        else if (pos_grid[i_y][i_x] == RABBIT_ID)
        {
          pos_grid[i_y][i_x] = EMPTY_ID;
          age_grid[i_y][i_x] = 0;
        }

    // Sync threads
    pthread_barrier_wait(&barrier);

    // Simulate Foxes
    for (i_y = inf.st_y; i_y <= inf.fn_y; i_y++)
      for (i_x = inf.st_x; i_x <= inf.fn_x; i_x++)
      {
        tmp_pos_grid[i_y][i_x] = 0;
        tmp_age_grid[i_y][i_x] = 0;
        tmp_hun_grid[i_y][i_x] = 0;
      }

    for (i_y = inf.st_y; i_y <= inf.fn_y; i_y++)
      for (i_x = inf.st_x; i_x <= inf.fn_x; i_x++)
        if (pos_grid[i_y][i_x] == FOX_ID)
        {
          vector<Position> candidate_moves;

          for (i = 0; i < 4; i++)
          {
            int nx = i_x + dx[i];
            int ny = i_y + dy[i];

            if ((nx >= 0 && nx < C && ny >= 0 && ny < R) && pos_grid[ny][nx] == RABBIT_ID)
              candidate_moves.push_back(Position(nx, ny));
          }

          if (candidate_moves.empty())
          {
            for (i = 0; i < 4; i++)
            {
              int nx = i_x + dx[i];
              int ny = i_y + dy[i];

              if ((nx >= 0 && nx < C && ny >= 0 && ny < R) && pos_grid[ny][nx] == EMPTY_ID)
                candidate_moves.push_back(Position(nx, ny));
            }
          }

          if (candidate_moves.empty())
          {
            if (hun_grid[i_y][i_x] + 1 >= GEN_FOOD_FOXES)
              continue;

            Fox n_fox;
            n_fox.p_x = i_x;
            n_fox.p_y = i_y;
            n_fox.age = age_grid[i_y][i_x] + 1;
            n_fox.hunger = hun_grid[i_y][i_x] + 1;
            replace_fox(n_fox, inf);
          }
          else
          {
            Position n_pos = candidate_moves[(iter + i_x + i_y) % ((int) candidate_moves.size())];
            int proc_flag = age_grid[i_y][i_x] >= GEN_PROC_FOXES, eat_flag = (pos_grid[n_pos.pY][n_pos.pX] == RABBIT_ID);

            if (!eat_flag && hun_grid[i_y][i_x] + 1 >= GEN_FOOD_FOXES)
              continue;

            if (proc_flag)
            {
              Fox n_fox;
              n_fox.p_x = i_x;
              n_fox.p_y = i_y;
              n_fox.age = 0;
              n_fox.hunger = 0;
              replace_fox(n_fox, inf);
            }

            Fox n_fox;
            n_fox.p_x = n_pos.pX;
            n_fox.p_y = n_pos.pY;
            n_fox.age = (proc_flag ? 0 : age_grid[i_y][i_x] + 1);
            n_fox.hunger = (eat_flag ? 0 : hun_grid[i_y][i_x] + 1);
            replace_fox(n_fox, inf);
          }
        }

    // Sync threads
    pthread_barrier_wait(&barrier);

    while (!fox_queues[inf.id].empty())
    {
      replace_fox(fox_queues[inf.id].front(), inf);
      fox_queues[inf.id].pop();
    }

    for (i_y = inf.st_y; i_y <= inf.fn_y; i_y++)
      for (i_x = inf.st_x; i_x <= inf.fn_x; i_x++)
        if (tmp_pos_grid[i_y][i_x] == FOX_ID)
        {
          pos_grid[i_y][i_x] = tmp_pos_grid[i_y][i_x];
          age_grid[i_y][i_x] = tmp_age_grid[i_y][i_x];
          hun_grid[i_y][i_x] = tmp_hun_grid[i_y][i_x];
        }
        else if (pos_grid[i_y][i_x] == FOX_ID)
        {
          pos_grid[i_y][i_x] = EMPTY_ID;
          age_grid[i_y][i_x] = 0;
          hun_grid[i_y][i_x] = 0;
        }
  }

  if (verbose && inf.id == 0)
    print_gen(N_GEN);
}

void print_gen(int gen)
{
  int i, j;

  printf("Generation %d\n", gen);

  printf("-");
  for (i = 0; i < C; i++)
    printf("-");
  printf("-\n");

  for (i = 0; i < R; i++)
  {
    printf("|");
    for (j = 0; j < C; j++)
      if (pos_grid[i][j] == RABBIT_ID)
        printf("R");
      else if (pos_grid[i][j] == FOX_ID)
        printf("F");
      else if (pos_grid[i][j] == ROCK_ID)
        printf("*");
      else
        printf(" ");
    printf("|\n");
  }

  printf("-");
  for (i = 0; i < C; i++)
    printf("-");
  printf("-\n\n");
}

void print_output()
{
  int i, j, ct = 0;
  for (i = 0; i < R; i++)
    for (j = 0; j < C; j++)
      if (pos_grid[i][j] == RABBIT_ID)
        ct++;
      else if (pos_grid[i][j] == FOX_ID)
        ct++;
      else if (pos_grid[i][j] == ROCK_ID)
        ct++;

  printf("%d %d %d %d %d %d %d\n", GEN_PROC_RABBITS, GEN_PROC_FOXES, GEN_FOOD_FOXES, 0, R, C, ct);

  for (i = 0; i < R; i++)
    for (j = 0; j < C; j++)
      if (pos_grid[i][j] == RABBIT_ID)
        printf("RABBIT %d %d\n", i, j);
      else if (pos_grid[i][j] == FOX_ID)
        printf("FOX %d %d\n", i, j);
      else if (pos_grid[i][j] == ROCK_ID)
        printf("ROCK %d %d\n", i, j);
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
    pthread_create(&threads[i], &pthread_custom_attr, static_compute, (void *)(th_info + i));

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
