#include "DDEngine.h"

DDEngine::DDEngine(int _verbose, int _n_th, int _redistribute)
{
  verbose = _verbose;
  n_th = _n_th;
  redistribute = _redistribute;
}

void DDEngine::init()
{
  memset(pos_grid, 0, sizeof pos_grid);

  pthread_mutex_init(&(th_rlock), NULL);
  pthread_mutex_init(&(th_flock), NULL);
  pthread_barrier_init(&barrier, NULL, n_th);
}

void DDEngine::insert_rabbit(Rabbit rabbit)
{
  pos_grid[rabbit.p_y][rabbit.p_x] = RABBIT_ID;
  global_rabbit_list.push_back(rabbit);
}

void DDEngine::insert_fox(Fox fox)
{
  pos_grid[fox.p_y][fox.p_x] = FOX_ID;
  global_fox_list.push_back(fox);
}

void DDEngine::insert_rock(Rock rock)
{
  pos_grid[rock.p_y][rock.p_x] = ROCK_ID;
}

bool r_pos_cmp(Rabbit a, Rabbit b)
{
  if (a.p_y == b.p_y)
    return a.p_x < b.p_x;
  return a.p_y < b.p_y;
}

bool f_pos_cmp(Fox a, Fox b)
{
  if (a.p_y == b.p_y)
    return a.p_x < b.p_x;
  return a.p_y < b.p_y;
}

void DDEngine::distribute_input()
{
  int i, j;
  for (i = 0; i < n_th; i++)
    th_info[i].id = i;

  for (i = 0; i < C; i++)
    pthread_mutex_init(&(th_locks[i]), NULL);

  for (i = 0; i < R; i++)
    for (j = 0; j < C; j++)
      test_mark_grid[i][j] = make_pair(-1, -1);

  sort(global_rabbit_list.begin(), global_rabbit_list.end(), r_pos_cmp);
  int r_sz = (int)global_rabbit_list.size();

  for (i = 0; i < n_th; i++)
  {
    int to_add = r_sz / n_th + ((r_sz - n_th * (r_sz / n_th)) > i);
    for (j = 0; j < to_add; j++)
    {
      rabbit_queue[i].push(global_rabbit_list.back());
      global_rabbit_list.pop_back();
    }
  }

  sort(global_fox_list.begin(), global_fox_list.end(), f_pos_cmp);
  int f_sz = (int)global_fox_list.size();

  for (i = 0; i < n_th; i++)
  {
    int to_add = f_sz / n_th + ((f_sz - n_th * (f_sz / n_th)) > i);
    for (j = 0; j < to_add; j++)
    {
      fox_queue[i].push(global_fox_list.back());
      global_fox_list.pop_back();
    }
  }
}

void DDEngine::compute(TInfo inf)
{
  int sq_gen = (int)sqrt(N_GEN);

  int iter, i;
  for (iter = 0; iter < N_GEN; iter++)
  {
    if (verbose && inf.id == 0)
    {
      pthread_barrier_wait(&barrier);
      print_gen(iter);
    }

    // Simulate Rabbits
    while (!rabbit_queue[inf.id].empty())
    {
      Rabbit rabbit = rabbit_queue[inf.id].front();
      rabbit_queue[inf.id].pop();

      if (pos_grid[rabbit.p_y][rabbit.p_x] != RABBIT_ID)
        continue;

      vector<Position> candidate_moves;

      for (i = 0; i < 4; i++)
      {
        int nx = rabbit.p_x + dx[i];
        int ny = rabbit.p_y + dy[i];

        if ((nx >= 0 && nx < C && ny >= 0 && ny < R) && pos_grid[ny][nx] == EMPTY_ID)
          candidate_moves.push_back(Position(nx, ny));
      }

      if (candidate_moves.empty())
      {
        Rabbit n_rabbit;
        n_rabbit.p_x = rabbit.p_x;
        n_rabbit.p_y = rabbit.p_y;
        n_rabbit.age = rabbit.age + 1;
        add_rabbit[inf.id].push_back(make_pair(n_rabbit, Position(-1, -1)));
      }
      else
      {
        int proc_flag = rabbit.age >= GEN_PROC_RABBITS;

        if (proc_flag)
        {
          Rabbit n_rabbit;
          n_rabbit.p_x = rabbit.p_x;
          n_rabbit.p_y = rabbit.p_y;
          n_rabbit.age = 0;
          add_rabbit[inf.id].push_back(make_pair(n_rabbit, Position(-1, -1)));
        }

        Position n_pos = candidate_moves[(iter + rabbit.p_x + rabbit.p_y) % ((int) candidate_moves.size())];
        Rabbit n_rabbit;
        n_rabbit.p_x = n_pos.pX;
        n_rabbit.p_y = n_pos.pY;
        n_rabbit.age = (proc_flag ? 0 : rabbit.age + 1);
        add_rabbit[inf.id].push_back(make_pair(n_rabbit, Position(proc_flag ? -1 : rabbit.p_x, rabbit.p_y)));
      }
    }

    for (i = 0; i < (int)add_rabbit[inf.id].size(); i++)
    {
      Rabbit rabbit = add_rabbit[inf.id][i].first;
      if (test_mark_grid[rabbit.p_y][rabbit.p_x].first == -1 || rabbit < test_rabbit_grid[rabbit.p_y][rabbit.p_x])
      {
        pthread_mutex_lock(&(th_locks[rabbit.p_x]));

        if (test_mark_grid[rabbit.p_y][rabbit.p_x].first == -1 || rabbit < test_rabbit_grid[rabbit.p_y][rabbit.p_x])
        {
          test_rabbit_grid[rabbit.p_y][rabbit.p_x] = rabbit;
          test_mark_grid[rabbit.p_y][rabbit.p_x] = make_pair(inf.id, i);
        }

        pthread_mutex_unlock(&(th_locks[rabbit.p_x]));
      }
    }

    pthread_barrier_wait(&barrier);

    for (i = 0; i < (int)add_rabbit[inf.id].size(); i++)
    {
      Rabbit rabbit = add_rabbit[inf.id][i].first;
      Position prev = add_rabbit[inf.id][i].second;

      if (prev.pX != -1)
        pos_grid[prev.pY][prev.pX] = EMPTY_ID;

      if (test_mark_grid[rabbit.p_y][rabbit.p_x].first == inf.id &&
          test_mark_grid[rabbit.p_y][rabbit.p_x].second == i)
      {
        pos_grid[rabbit.p_y][rabbit.p_x] = RABBIT_ID;
        test_mark_grid[rabbit.p_y][rabbit.p_x] = make_pair(-1, -1);
        rabbit_queue[inf.id].push(rabbit);
      }
    }

    add_rabbit[inf.id].clear();

    pthread_barrier_wait(&barrier);


    // Simulate Foxes
    while (!fox_queue[inf.id].empty())
    {
      Fox fox = fox_queue[inf.id].front();
      fox_queue[inf.id].pop();

      vector<Position> candidate_moves;

      for (i = 0; i < 4; i++)
      {
        int nx = fox.p_x + dx[i];
        int ny = fox.p_y + dy[i];

        if ((nx >= 0 && nx < C && ny >= 0 && ny < R) && pos_grid[ny][nx] == RABBIT_ID)
          candidate_moves.push_back(Position(nx, ny));
      }

      if (candidate_moves.empty())
      {
        for (i = 0; i < 4; i++)
        {
          int nx = fox.p_x + dx[i];
          int ny = fox.p_y + dy[i];

          if ((nx >= 0 && nx < C && ny >= 0 && ny < R) && pos_grid[ny][nx] == EMPTY_ID)
            candidate_moves.push_back(Position(nx, ny));
        }
      }

      if (candidate_moves.empty())
      {
        if (fox.hunger + 1 >= GEN_FOOD_FOXES)
        {
          add_fox[inf.id].push_back(make_pair(fox, Position(-1, -2)));
          continue;
        }

        Fox n_fox;
        n_fox.p_x = fox.p_x;
        n_fox.p_y = fox.p_y;
        n_fox.age = fox.age + 1;
        n_fox.hunger = fox.hunger + 1;
        add_fox[inf.id].push_back(make_pair(n_fox, Position(-1, -1)));
      }
      else
      {
        Position n_pos = candidate_moves[(iter + fox.p_x + fox.p_y) % ((int) candidate_moves.size())];
        int proc_flag = fox.age >= GEN_PROC_FOXES, eat_flag = (pos_grid[n_pos.pY][n_pos.pX] == RABBIT_ID);

        if (!eat_flag && fox.hunger + 1 >= GEN_FOOD_FOXES)
        {
          add_fox[inf.id].push_back(make_pair(fox, Position(-1, -2)));
          continue;
        }

        if (proc_flag)
        {
          Fox n_fox;
          n_fox.p_x = fox.p_x;
          n_fox.p_y = fox.p_y;
          n_fox.age = 0;
          n_fox.hunger = 0;
          add_fox[inf.id].push_back(make_pair(n_fox, Position(-1, -1)));
        }

        Fox n_fox;
        n_fox.p_x = n_pos.pX;
        n_fox.p_y = n_pos.pY;
        n_fox.age = (proc_flag ? 0 : fox.age + 1);
        n_fox.hunger = (eat_flag ? 0 : fox.hunger + 1);
        add_fox[inf.id].push_back(make_pair(n_fox, Position(proc_flag ? -1 : fox.p_x, fox.p_y)));
      }
    }

    for (i = 0; i < (int)add_fox[inf.id].size(); i++)
    {
      Fox fox = add_fox[inf.id][i].first;
      if (test_mark_grid[fox.p_y][fox.p_x].first == -1 || fox < test_fox_grid[fox.p_y][fox.p_x])
      {
        pthread_mutex_lock(&(th_locks[fox.p_x]));

        if (test_mark_grid[fox.p_y][fox.p_x].first == -1 || fox < test_fox_grid[fox.p_y][fox.p_x])
        {
          test_fox_grid[fox.p_y][fox.p_x] = fox;
          test_mark_grid[fox.p_y][fox.p_x] = make_pair(inf.id, i);
        }

        pthread_mutex_unlock(&(th_locks[fox.p_x]));
      }
    }

    pthread_barrier_wait(&barrier);

    for (i = 0; i < (int)add_fox[inf.id].size(); i++)
    {
      Fox fox = add_fox[inf.id][i].first;
      Position prev = add_fox[inf.id][i].second;

      if (prev.pX != -1)
        pos_grid[prev.pY][prev.pX] = EMPTY_ID;

      if (prev.pY == -2)
      {
        test_mark_grid[fox.p_y][fox.p_x] = make_pair(-1, -1);
        pos_grid[fox.p_y][fox.p_x] = EMPTY_ID;
        continue;
      }

      if (test_mark_grid[fox.p_y][fox.p_x].first == inf.id &&
          test_mark_grid[fox.p_y][fox.p_x].second == i)
      {
        pos_grid[fox.p_y][fox.p_x] = FOX_ID;
        test_mark_grid[fox.p_y][fox.p_x] = make_pair(-1, -1);
        fox_queue[inf.id].push(fox);
      }
    }

    add_fox[inf.id].clear();

    pthread_barrier_wait(&barrier);

    if (redistribute && iter && iter % sq_gen == 0)
    {
      pthread_mutex_lock(&(th_rlock));
      while (!rabbit_queue[inf.id].empty())
      {
        global_rabbit_list.push_back(rabbit_queue[inf.id].front());
        rabbit_queue[inf.id].pop();
      }
      pthread_mutex_unlock(&(th_rlock));

      pthread_mutex_lock(&(th_flock));
      while (!fox_queue[inf.id].empty())
      {
        global_fox_list.push_back(fox_queue[inf.id].front());
        fox_queue[inf.id].pop();
      }
      pthread_mutex_unlock(&(th_flock));

      pthread_barrier_wait(&barrier);

      if (inf.id == 0)
      {
        sort(global_rabbit_list.begin(), global_rabbit_list.end(), r_pos_cmp);
        int r_sz = (int)global_rabbit_list.size();

        int i, j;
        for (i = 0; i < n_th; i++)
        {
          int to_add = r_sz / n_th + ((r_sz - n_th * (r_sz / n_th)) > i);
          for (j = 0; j < to_add; j++)
          {
            rabbit_queue[i].push(global_rabbit_list.back());
            global_rabbit_list.pop_back();
          }
        }
      }

      if (inf.id == 1 || (n_th == 1 && inf.id == 0))
      {
        sort(global_fox_list.begin(), global_fox_list.end(), f_pos_cmp);
        int f_sz = (int)global_fox_list.size();

        int i, j;
        for (i = 0; i < n_th; i++)
        {
          int to_add = f_sz / n_th + ((f_sz - n_th * (f_sz / n_th)) > i);
          for (j = 0; j < to_add; j++)
          {
            fox_queue[i].push(global_fox_list.back());
            global_fox_list.pop_back();
          }
        }
      }

      pthread_barrier_wait(&barrier);
    }
  }

  if (verbose && inf.id == 0)
  {
    pthread_barrier_wait(&barrier);
    print_gen(N_GEN);
  }
}

void DDEngine::print_gen(int gen)
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

void DDEngine::print_output()
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

void DDEngine::setup_input(int _GEN_PROC_RABBITS, int _GEN_PROC_FOXES, int _GEN_FOOD_FOXES, int _N_GEN, int _R, int _C, int _N)
{
  GEN_PROC_RABBITS = _GEN_PROC_RABBITS;
  GEN_PROC_FOXES = _GEN_PROC_FOXES;
  GEN_FOOD_FOXES = _GEN_FOOD_FOXES;
  N_GEN = _N_GEN;
  R = _R;
  C = _C;
  N = _N;
}

TInfo DDEngine::get_info(int id)
{
  return th_info[id];
}
