#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <vector>
#include <algorithm>

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
} typedef Rabbit;

struct Fox
{
  int p_x, p_y;
  int age, hunger;
} typedef Fox;

struct Rock
{
  int p_x, p_y;
} typedef Rock;

TInfo th_info[MAX_THREAD];
int n_th;

int dx[4] = {0, 1, 0, -1};
int dy[4] = {-1, 0, 1, 0};
int GEN_PROC_RABBITS, GEN_PROC_FOXES, GEN_FOOD_FOXES, N_GEN, R, C, N;
int pos_grid[MAX_SIZE][MAX_SIZE];
int age_grid[MAX_SIZE][MAX_SIZE];
int hun_grid[MAX_SIZE][MAX_SIZE];

void print_gen(int gen);

void init()
{
  memset(pos_grid, 0, sizeof pos_grid);
  memset(age_grid, 0, sizeof age_grid);
  memset(hun_grid, 0, sizeof hun_grid);
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

void *static_compute(void *arg)
{
  TInfo inf = *((TInfo *)arg);
  int tmp_pos_grid[inf.fn_y - inf.st_y + 1][inf.fn_x - inf.st_x + 1];
  int tmp_age_grid[inf.fn_y - inf.st_y + 1][inf.fn_x - inf.st_x + 1];
  int tmp_hun_grid[inf.fn_y - inf.st_y + 1][inf.fn_x - inf.st_x + 1];

  int iter, i, i_x, i_y;
  for (iter = 0; iter < N_GEN; iter++)
  {
    print_gen(iter);
    // Simulate Rabbits
    memset(tmp_pos_grid, -1, sizeof tmp_pos_grid);
    memset(tmp_age_grid, 0, sizeof tmp_age_grid);
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
          }
          else
          {
            int proc_flag = 0;

            if (age_grid[i_y][i_x] >= GEN_PROC_RABBITS)
            {
              tmp_pos_grid[i_y][i_x] = pos_grid[i_y][i_x];
              tmp_age_grid[i_y][i_x] = max(tmp_age_grid[i_y][i_x], 0);
              proc_flag = 1;
            }
            else
            {
              if (tmp_pos_grid[i_y][i_x] == -1)
              {
                tmp_pos_grid[i_y][i_x] = EMPTY_ID;
                tmp_age_grid[i_y][i_x] = 0;
              }
            }

            Position n_pos = candidate_moves[(iter + i_x + i_y) % ((int) candidate_moves.size())];
            tmp_pos_grid[n_pos.pY][n_pos.pX] = pos_grid[i_y][i_x];
            tmp_age_grid[n_pos.pY][n_pos.pX] = max(tmp_age_grid[n_pos.pY][n_pos.pX],
                                                   (proc_flag ? 0 : age_grid[i_y][i_x] + 1));
          }
        }

    for (i_y = inf.st_y; i_y <= inf.fn_y; i_y++)
      for (i_x = inf.st_x; i_x <= inf.fn_x; i_x++)
        if (tmp_pos_grid[i_y][i_x] != -1)
        {
          pos_grid[i_y][i_x] = tmp_pos_grid[i_y][i_x];
          age_grid[i_y][i_x] = tmp_age_grid[i_y][i_x];
        }

    // Simulate Foxes
    memset(tmp_pos_grid, -1, sizeof tmp_pos_grid);
    memset(tmp_age_grid, 0, sizeof tmp_age_grid);
    memset(tmp_hun_grid, 0, sizeof tmp_hun_grid);

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
            tmp_pos_grid[i_y][i_x] = pos_grid[i_y][i_x];
            tmp_age_grid[i_y][i_x] = age_grid[i_y][i_x] + 1;
            tmp_hun_grid[i_y][i_x] = hun_grid[i_y][i_x] + 1;
          }
          else
          {
            int proc_flag = 0, eat_flag = 0;
            Position n_pos = candidate_moves[(iter + i_x + i_y) % ((int) candidate_moves.size())];
            eat_flag = (pos_grid[n_pos.pY][n_pos.pX] == RABBIT_ID);

            if (!eat_flag && hun_grid[i_y][i_x] + 1 >= GEN_FOOD_FOXES)
            {
              if (tmp_pos_grid[i_y][i_x] == -1)
              {
                tmp_pos_grid[i_y][i_x] = EMPTY_ID;
                tmp_age_grid[i_y][i_x] = 0;
                tmp_hun_grid[i_y][i_x] = 0;
              }
              continue;
            }

            if (age_grid[i_y][i_x] >= GEN_PROC_FOXES)
            {
              if (tmp_pos_grid[i_y][i_x] == FOX_ID)
              {
                if (tmp_age_grid[i_y][i_x] < 0)
                {
                  tmp_age_grid[i_y][i_x] = 0;
                  tmp_hun_grid[i_y][i_x] = 0;
                }
                else if (tmp_age_grid[i_y][i_x] == 0 &&
                         tmp_hun_grid[i_y][i_x] > 0)
                  tmp_hun_grid[i_y][i_x] = 0;
              }
              tmp_pos_grid[i_y][i_x] = pos_grid[i_y][i_x];

              proc_flag = 1;
            }
            else
            {
              if (tmp_pos_grid[i_y][i_x] == -1)
              {
                tmp_pos_grid[i_y][i_x] = EMPTY_ID;
                tmp_age_grid[i_y][i_x] = 0;
                tmp_hun_grid[i_y][i_x] = 0;
              }
            }

            if (tmp_pos_grid[n_pos.pY][n_pos.pX] == FOX_ID)
            {
              if (tmp_age_grid[n_pos.pY][n_pos.pX] < (proc_flag ? 0 : age_grid[i_y][i_x] + 1))
              {
                tmp_age_grid[n_pos.pY][n_pos.pX] = (proc_flag ? 0 : age_grid[i_y][i_x] + 1);
                tmp_hun_grid[n_pos.pY][n_pos.pX] = (eat_flag ? 0 : hun_grid[i_y][i_x] + 1);
              }
              else if (tmp_age_grid[n_pos.pY][n_pos.pX] == (proc_flag ? 0 : age_grid[i_y][i_x] + 1) &&
                       tmp_hun_grid[n_pos.pY][n_pos.pX] > (eat_flag ? 0 : hun_grid[i_y][i_x] + 1))
                tmp_hun_grid[n_pos.pY][n_pos.pX] = (eat_flag ? 0 : hun_grid[i_y][i_x] + 1);
            }
            else
            {
              tmp_age_grid[n_pos.pY][n_pos.pX] = (proc_flag ? 0 : age_grid[i_y][i_x] + 1);
              tmp_hun_grid[n_pos.pY][n_pos.pX] = (eat_flag ? 0 : hun_grid[i_y][i_x] + 1);
            }

            tmp_pos_grid[n_pos.pY][n_pos.pX] = pos_grid[i_y][i_x];
          }
        }

    for (i_y = inf.st_y; i_y <= inf.fn_y; i_y++)
      for (i_x = inf.st_x; i_x <= inf.fn_x; i_x++)
        if (tmp_pos_grid[i_y][i_x] != -1)
        {
          pos_grid[i_y][i_x] = tmp_pos_grid[i_y][i_x];
          age_grid[i_y][i_x] = tmp_age_grid[i_y][i_x];
          hun_grid[i_y][i_x] = tmp_hun_grid[i_y][i_x];
        }

    // Sync threads
  }

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

  if (argc != 2)
  {
    printf("Usage: %s n\n  where n is number of threads\n", argv[0]);
    exit(1);
  }

  n_th = atoi(argv[1]);

  if ((n_th < 1) || (n_th > MAX_THREAD - 1))
  {
    printf("The number of thread should between 1 and %d\n", MAX_THREAD - 1);
    exit(1);
  }

  init();
  read_input();
  distribute_input();

  threads = (pthread_t *)malloc(n_th * sizeof(*threads));
  pthread_attr_init(&pthread_custom_attr);

  int i;
  for (i = 0; i < n_th; i++)
    pthread_create(&threads[i], &pthread_custom_attr, static_compute, (void *)(th_info + i));

  for (i = 0; i < n_th; i++)
    pthread_join(threads[i], NULL);
  free(threads);

  print_output();

  return 0;
}
