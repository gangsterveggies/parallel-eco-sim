#include "TopologyEngine.h"

TopologyEngine::TopologyEngine(int _verbose, int _n_th)
{
  verbose = _verbose;
  n_th = _n_th;
}

void TopologyEngine::init()
{
  memset(pos_grid, 0, sizeof pos_grid);
  memset(age_grid, 0, sizeof age_grid);
  memset(hun_grid, 0, sizeof hun_grid);

  int i;
  for (i = 0; i < n_th; i++)
    pthread_mutex_init(&(th_locks[i]), NULL);

  pthread_barrier_init(&barrier, NULL, n_th);
}

void TopologyEngine::insert_rabbit(Rabbit rabbit)
{
  pos_grid[rabbit.p_y][rabbit.p_x] = RABBIT_ID;
  age_grid[rabbit.p_y][rabbit.p_x] = rabbit.age;
}

void TopologyEngine::insert_fox(Fox fox)
{
  pos_grid[fox.p_y][fox.p_x] = FOX_ID;
  age_grid[fox.p_y][fox.p_x] = fox.age;
  hun_grid[fox.p_y][fox.p_x] = fox.hunger;
}

void TopologyEngine::insert_rock(Rock rock)
{
  pos_grid[rock.p_y][rock.p_x] = ROCK_ID;
}

void TopologyEngine::distribute_input()
{
  int i, i_x, i_y, acc = 0;
  for (i = 0; i < n_th; i++)
  {
    th_info[i].id = i;
    th_info[i].st_x = 0;
    th_info[i].fn_x = C - 1;
    th_info[i].st_y = acc;
    acc += R / n_th + ((R - n_th * (R / n_th)) > i);
    th_info[i].fn_y = acc - 1;

    for (i_y = th_info[i].st_y; i_y <= th_info[i].fn_y; i_y++)
      for (i_x = th_info[i].st_x; i_x <= th_info[i].fn_x; i_x++)
        owner[i_y][i_x] = i;
  }
}

void TopologyEngine::replace_rabbit(Rabbit rabbit, TInfo inf)
{
  if (owner[rabbit.p_y][rabbit.p_x] != inf.id)
  {
    int t_o = owner[rabbit.p_y][rabbit.p_x];
    pthread_mutex_lock(&(th_locks[t_o]));
    rabbit_queues[t_o].push(rabbit);
    pthread_mutex_unlock(&(th_locks[t_o]));

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

void TopologyEngine::replace_fox(Fox fox, TInfo inf)
{
  if (owner[fox.p_y][fox.p_x] != inf.id)
  {
    int t_o = owner[fox.p_y][fox.p_x];
    pthread_mutex_lock(&(th_locks[t_o]));
    fox_queues[t_o].push(fox);
    pthread_mutex_unlock(&(th_locks[t_o]));

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

void TopologyEngine::compute(TInfo inf)
{
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
  {
    pthread_barrier_wait(&barrier);
    print_gen(N_GEN);
  }
}

void TopologyEngine::print_gen(int gen)
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

void TopologyEngine::print_output()
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

void TopologyEngine::setup_input(int _GEN_PROC_RABBITS, int _GEN_PROC_FOXES, int _GEN_FOOD_FOXES, int _N_GEN, int _R, int _C, int _N)
{
  GEN_PROC_RABBITS = _GEN_PROC_RABBITS;
  GEN_PROC_FOXES = _GEN_PROC_FOXES;
  GEN_FOOD_FOXES = _GEN_FOOD_FOXES;
  N_GEN = _N_GEN;
  R = _R;
  C = _C;
  N = _N;
}

TInfo TopologyEngine::get_info(int id)
{
  return th_info[id];
}
