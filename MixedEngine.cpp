#include "MixedEngine.h"

MixedEngine::MixedEngine(int _verbose, int _n_th, int _redistribute)
{
  verbose = _verbose;
  n_th = _n_th;
  redistribute = _redistribute;
}

void MixedEngine::init()
{
  memset(pos_grid, 0, sizeof pos_grid);
  memset(age_grid, 0, sizeof age_grid);
  memset(hun_grid, 0, sizeof hun_grid);
  memset(tmp_pos_grid, 0, sizeof tmp_pos_grid);
  memset(tmp_age_grid, 0, sizeof tmp_age_grid);
  memset(tmp_hun_grid, 0, sizeof tmp_hun_grid);

  int i;
  for (i = 0; i < n_th; i++)
    pthread_mutex_init(&(th_locks[i]), NULL);

  pthread_barrier_init(&barrier, NULL, n_th);
}

void MixedEngine::insert_rabbit(Rabbit rabbit)
{
  pos_grid[rabbit.p_y][rabbit.p_x] = RABBIT_ID;
  age_grid[rabbit.p_y][rabbit.p_x] = rabbit.age;
}

void MixedEngine::insert_fox(Fox fox)
{
  pos_grid[fox.p_y][fox.p_x] = FOX_ID;
  age_grid[fox.p_y][fox.p_x] = fox.age;
  hun_grid[fox.p_y][fox.p_x] = fox.hunger;
}

void MixedEngine::insert_rock(Rock rock)
{
  pos_grid[rock.p_y][rock.p_x] = ROCK_ID;
}

void MixedEngine::distribute_input()
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
      {
        owner[i_y][i_x] = i;

        if (pos_grid[i_y][i_x] == RABBIT_ID)
          rabbit_list[i].push(Position(i_x, i_y));

        if (pos_grid[i_y][i_x] == FOX_ID)
          fox_list[i].push(Position(i_x, i_y));

        test_mark_grid[i_y][i_x] = make_pair(-1, -1);
      }
  }
}

void MixedEngine::replace_rabbit(Rabbit rabbit, TInfo inf, int id)
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
  test_mark_grid[rabbit.p_y][rabbit.p_x] = make_pair(inf.id, id);
}

void MixedEngine::replace_fox(Fox fox, TInfo inf, int id)
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
  test_mark_grid[fox.p_y][fox.p_x] = make_pair(inf.id, id);
}

void MixedEngine::compute(TInfo inf)
{
  int iter, i, i_x, i_y;
  for (iter = 0; iter < N_GEN; iter++)
  {
    // Sync threads
    pthread_barrier_wait(&barrier);
    if (verbose && inf.id == 0)
      print_gen(iter);

    // Simulate Rabbits
    while (!rabbit_list[inf.id].empty())
    {
      Position rabbit = rabbit_list[inf.id].front();
      rabbit_list[inf.id].pop();

      if (pos_grid[rabbit.pY][rabbit.pX] != RABBIT_ID)
        continue;

      vector<Position> candidate_moves;

      for (i = 0; i < 4; i++)
      {
        int nx = rabbit.pX + dx[i];
        int ny = rabbit.pY + dy[i];

        if ((nx >= 0 && nx < C && ny >= 0 && ny < R) && pos_grid[ny][nx] == EMPTY_ID)
          candidate_moves.push_back(Position(nx, ny));
      }

      if (candidate_moves.empty())
      {
        Rabbit n_rabbit;
        n_rabbit.p_x = rabbit.pX;
        n_rabbit.p_y = rabbit.pY;
        n_rabbit.age = age_grid[rabbit.pY][rabbit.pX] + 1;
        add_rabbit[inf.id].push_back(make_pair(n_rabbit, Position(-1, -1)));
      }
      else
      {
        int proc_flag = age_grid[rabbit.pY][rabbit.pX] >= GEN_PROC_RABBITS;

        if (proc_flag)
        {
          Rabbit n_rabbit;
          n_rabbit.p_x = rabbit.pX;
          n_rabbit.p_y = rabbit.pY;
          n_rabbit.age = 0;
          add_rabbit[inf.id].push_back(make_pair(n_rabbit, Position(-1, -1)));
        }

        Position n_pos = candidate_moves[(iter + rabbit.pX + rabbit.pY) % ((int) candidate_moves.size())];
        Rabbit n_rabbit;
        n_rabbit.p_x = n_pos.pX;
        n_rabbit.p_y = n_pos.pY;
        n_rabbit.age = (proc_flag ? 0 : age_grid[rabbit.pY][rabbit.pX] + 1);
        add_rabbit[inf.id].push_back(make_pair(n_rabbit, Position(proc_flag ? -1 : rabbit.pX, rabbit.pY)));
      }
    }

    for (i = 0; i < (int)add_rabbit[inf.id].size(); i++)
    {
      Rabbit rabbit = add_rabbit[inf.id][i].first;
      replace_rabbit(rabbit, inf, i);
    }

    // Sync threads
    pthread_barrier_wait(&barrier);

    while (!rabbit_queues[inf.id].empty())
    {
      replace_rabbit(rabbit_queues[inf.id].front(), inf, (int)add_rabbit[inf.id].size());
      add_rabbit[inf.id].push_back(make_pair(rabbit_queues[inf.id].front(), Position(-1, -1)));
      rabbit_queues[inf.id].pop();
    }

    for (i = 0; i < (int)add_rabbit[inf.id].size(); i++)
    {
      Rabbit rabbit = add_rabbit[inf.id][i].first;
      Position prev = add_rabbit[inf.id][i].second;

      if (prev.pX != -1)
      {
        pos_grid[prev.pY][prev.pX] = EMPTY_ID;
        age_grid[prev.pY][prev.pX] = 0;
      }

      if (test_mark_grid[rabbit.p_y][rabbit.p_x].first == inf.id &&
          test_mark_grid[rabbit.p_y][rabbit.p_x].second == i)
      {
        pos_grid[rabbit.p_y][rabbit.p_x] = RABBIT_ID;
        age_grid[rabbit.p_y][rabbit.p_x] = tmp_age_grid[rabbit.p_y][rabbit.p_x];
        test_mark_grid[rabbit.p_y][rabbit.p_x] = make_pair(-1, -1);
        rabbit_list[inf.id].push(Position(rabbit.p_x, rabbit.p_y));
        tmp_pos_grid[rabbit.p_y][rabbit.p_x] = 0;
        tmp_age_grid[rabbit.p_y][rabbit.p_x] = 0;
      }
    }

    add_rabbit[inf.id].clear();

    // Sync threads
    pthread_barrier_wait(&barrier);


    // Simulate Foxes
    while (!fox_list[inf.id].empty())
    {
      Position fox = fox_list[inf.id].front();
      fox_list[inf.id].pop();

      vector<Position> candidate_moves;

      for (i = 0; i < 4; i++)
      {
        int nx = fox.pX + dx[i];
        int ny = fox.pY + dy[i];

        if ((nx >= 0 && nx < C && ny >= 0 && ny < R) && pos_grid[ny][nx] == RABBIT_ID)
          candidate_moves.push_back(Position(nx, ny));
      }

      if (candidate_moves.empty())
      {
        for (i = 0; i < 4; i++)
        {
          int nx = fox.pX + dx[i];
          int ny = fox.pY + dy[i];

          if ((nx >= 0 && nx < C && ny >= 0 && ny < R) && pos_grid[ny][nx] == EMPTY_ID)
            candidate_moves.push_back(Position(nx, ny));
        }
      }

      if (candidate_moves.empty())
      {
        if (hun_grid[fox.pY][fox.pX] + 1 >= GEN_FOOD_FOXES)
        {
          Fox n_fox;
          n_fox.p_x = fox.pX;
          n_fox.p_y = fox.pY;
          n_fox.age = age_grid[fox.pY][fox.pX];
          n_fox.hunger = hun_grid[fox.pY][fox.pX];
          add_fox[inf.id].push_back(make_pair(n_fox, Position(-1, -2)));
          continue;
        }

        Fox n_fox;
        n_fox.p_x = fox.pX;
        n_fox.p_y = fox.pY;
        n_fox.age = age_grid[fox.pY][fox.pX] + 1;
        n_fox.hunger = hun_grid[fox.pY][fox.pX] + 1;
        add_fox[inf.id].push_back(make_pair(n_fox, Position(-1, -1)));
      }
      else
      {
        Position n_pos = candidate_moves[(iter + fox.pX + fox.pY) % ((int) candidate_moves.size())];
        int proc_flag = age_grid[fox.pY][fox.pX] >= GEN_PROC_FOXES, eat_flag = (pos_grid[n_pos.pY][n_pos.pX] == RABBIT_ID);

        if (!eat_flag && hun_grid[fox.pY][fox.pX] + 1 >= GEN_FOOD_FOXES)
        {
          Fox n_fox;
          n_fox.p_x = fox.pX;
          n_fox.p_y = fox.pY;
          n_fox.age = age_grid[fox.pY][fox.pX];
          n_fox.hunger = hun_grid[fox.pY][fox.pX];
          add_fox[inf.id].push_back(make_pair(n_fox, Position(-1, -2)));
          continue;
        }

        if (proc_flag)
        {
          Fox n_fox;
          n_fox.p_x = fox.pX;
          n_fox.p_y = fox.pY;
          n_fox.age = 0;
          n_fox.hunger = 0;
          add_fox[inf.id].push_back(make_pair(n_fox, Position(-1, -1)));
        }

        Fox n_fox;
        n_fox.p_x = n_pos.pX;
        n_fox.p_y = n_pos.pY;
        n_fox.age = (proc_flag ? 0 : age_grid[fox.pY][fox.pX] + 1);
        n_fox.hunger = (eat_flag ? 0 : hun_grid[fox.pY][fox.pX] + 1);
        add_fox[inf.id].push_back(make_pair(n_fox, Position(proc_flag ? -1 : fox.pX, fox.pY)));
      }
    }

    for (i = 0; i < (int)add_fox[inf.id].size(); i++)
    {
      Fox fox = add_fox[inf.id][i].first;
      replace_fox(fox, inf, i);
    }

    pthread_barrier_wait(&barrier);

    while (!fox_queues[inf.id].empty())
    {
      replace_fox(fox_queues[inf.id].front(), inf, (int)add_fox[inf.id].size());
      add_fox[inf.id].push_back(make_pair(fox_queues[inf.id].front(), Position(-1, -1)));
      fox_queues[inf.id].pop();
    }

    for (i = 0; i < (int)add_fox[inf.id].size(); i++)
    {
      Fox fox = add_fox[inf.id][i].first;
      Position prev = add_fox[inf.id][i].second;

      if (prev.pX != -1)
      {
        pos_grid[prev.pY][prev.pX] = EMPTY_ID;
        age_grid[prev.pY][prev.pX] = 0;
        hun_grid[prev.pY][prev.pX] = 0;
      }

      if (prev.pY == -2)
      {
        test_mark_grid[fox.p_y][fox.p_x] = make_pair(-1, -1);
        pos_grid[fox.p_y][fox.p_x] = EMPTY_ID;
        age_grid[fox.p_y][fox.p_x] = 0;
        hun_grid[fox.p_y][fox.p_x] = 0;

        tmp_pos_grid[fox.p_y][fox.p_x] = 0;
        tmp_age_grid[fox.p_y][fox.p_x] = 0;
        tmp_hun_grid[fox.p_y][fox.p_x] = 0;
        continue;
      }

      if (test_mark_grid[fox.p_y][fox.p_x].first == inf.id &&
          test_mark_grid[fox.p_y][fox.p_x].second == i)
      {
        pos_grid[fox.p_y][fox.p_x] = FOX_ID;
        age_grid[fox.p_y][fox.p_x] = tmp_age_grid[fox.p_y][fox.p_x];
        hun_grid[fox.p_y][fox.p_x] = tmp_hun_grid[fox.p_y][fox.p_x];
        test_mark_grid[fox.p_y][fox.p_x] = make_pair(-1, -1);
        fox_list[inf.id].push(Position(fox.p_x, fox.p_y));

        tmp_pos_grid[fox.p_y][fox.p_x] = 0;
        tmp_age_grid[fox.p_y][fox.p_x] = 0;
        tmp_hun_grid[fox.p_y][fox.p_x] = 0;
      }
    }

    add_fox[inf.id].clear();
  }

  if (verbose)
    pthread_barrier_wait(&barrier);
  if (verbose && inf.id == 0)
    print_gen(N_GEN);
}

void MixedEngine::print_gen(int gen)
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

void MixedEngine::print_output()
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

void MixedEngine::setup_input(int _GEN_PROC_RABBITS, int _GEN_PROC_FOXES, int _GEN_FOOD_FOXES, int _N_GEN, int _R, int _C, int _N)
{
  GEN_PROC_RABBITS = _GEN_PROC_RABBITS;
  GEN_PROC_FOXES = _GEN_PROC_FOXES;
  GEN_FOOD_FOXES = _GEN_FOOD_FOXES;
  N_GEN = _N_GEN;
  R = _R;
  C = _C;
  N = _N;
}

TInfo MixedEngine::get_info(int id)
{
  return th_info[id];
}
