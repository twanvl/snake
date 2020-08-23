
//------------------------------------------------------------------------------
// Hamiltonian cycles
// represented as: at each grid point the coordinate of the next point
//------------------------------------------------------------------------------

using GridPath = Grid<Coord>;

bool is_hamiltonian_cycle(GridPath const& path) {
  // conditions:
  //  * each step points to a neighbor
  //  * after w*h steps we are back at the begining (we have a cycle)
  //  * and no sooner (cycle has length w*h, so it is the only one)
  Coord pos = {0,0};
  for (int i=0; i < path.size(); ++i) {
    if (!path.valid(path[pos])) return false;
    if (!is_neighbor(pos, path[pos])) return false;
    pos = path[pos];
    if (pos == Coord{0,0}) {
      return (i == path.size()-1);
    }
  }
  return false;
}

// Make a Hamiltonian cycle given a w/2 * h/2 tree
GridPath tree_to_hamiltonian_cycle(Grid<Coord> const& parent) {
  GridPath path(parent.w*2, parent.h*2, INVALID);
  for (int y=0; y<path.h; ++y) {
    for (int x=0; x<path.w; ++x) {
      Coord c = {x,y};
      Coord in, out;
      if (x%2 == 1 && y%2 == 0) { out = c+Dir::up;    in = c+Dir::left; }
      if (x%2 == 0 && y%2 == 1) { out = c+Dir::down;  in = c+Dir::right; }
      if (x%2 == 0 && y%2 == 0) { out = c+Dir::left;  in = c+Dir::down; }
      if (x%2 == 1 && y%2 == 1) { out = c+Dir::right; in = c+Dir::up; }
      Coord c_cell = {x/2, y/2};
      Coord o_cell = {out.x/2, out.y/2};
      assert(!path.valid(out) || o_cell != c_cell);
      if (path.valid(out) && (parent[o_cell] == c_cell || parent[c_cell] == o_cell)) {
        path[c] = out;
      } else {
        path[c] = in;
      }
    }
  }
  assert(is_hamiltonian_cycle(path));
  return path;
}

Grid<Coord> random_spanning_tree(CoordRange dims, RNG& rng) {
  Grid<Coord> tree(dims, INVALID);
  std::vector<std::pair<Coord,Coord>> queue;
  {
    Coord node = dims.random(rng);
    tree[node] = ROOT;
    for (auto d : dirs) {
      if (tree.valid(node+d)) queue.push_back({node, node+d});
    }
  }
  while (!queue.empty()) {
    int i = rng.random(queue.size());
    Coord parent = queue[i].first;
    Coord node   = queue[i].second;
    queue[i] = queue.back();
    queue.pop_back();
    if (tree[node] == INVALID) {
      tree[node] = parent;
      for (auto d : dirs) {
        if (tree.valid(node+d)) queue.push_back({node, node+d});
      }
    }
  }
  return tree;
}

GridPath random_hamiltonian_cycle(CoordRange dims, RNG& rng) {
  return tree_to_hamiltonian_cycle(random_spanning_tree({dims.w/2, dims.h/2}, rng));
}


int path_distane(GridPath const& path, Coord from, Coord to) {
  int dist = 0;
  while (from != to) {
    from = path[from];
    dist++;
  }
  return dist;
}

GridPath reverse(GridPath const& path) {
  GridPath reverse(path.dimensions(), INVALID);
  for (auto pos : path.coords()) {
    reverse[path[pos]] = pos;
  }
  return reverse;
}

Coord path_from(GridPath const& path, Coord to) {
  if (to.x > 0        && path[{to.x-1, to.y}] == to) return Coord{to.x-1, to.y};
  if (to.x < path.w-1 && path[{to.x+1, to.y}] == to) return Coord{to.x+1, to.y};
  if (to.y > 0        && path[{to.x, to.y-1}] == to) return Coord{to.x, to.y-1};
  if (to.y < path.h-1 && path[{to.x, to.y+1}] == to) return Coord{to.x, to.y+1};
  throw "No path from neighbor";
}

// Mark the nodes by setting mark[c]=value for all c on the path from...to (inclusive)
template <typename T>
void mark_path(GridPath const& path, Coord from, Coord to, Grid<T>& mark, T value) {
  mark[from] = value;
  while (from != to) {
    from = path[from];
    mark[from] = value;
  }
}

template <typename Color>
Grid<std::string> draw_cycle(GridPath const& cycle, Color color) {
  std::vector<Coord> path;
  for (Coord c = {0,0};;) {
    path.push_back(c);
    c = cycle[c];
    if (c == Coord{0,0}) break;
  }
  Grid<std::string> grid(cycle.dimensions(), ".");
  draw_path(grid, path, color, true);
  return grid;
}
template <typename Color>
Grid<std::string> draw_cycle2(GridPath const& cycle, Color color) {
  Grid<std::string> grid(cycle.dimensions(), ".");
  const char* vis[] = {"↑","↓","←","→"};
  for (auto c : cycle.coords()) {
    grid[c] = color(vis[static_cast<int>(cycle[c]-c)]);
  }
  return grid;
}


//------------------------------------------------------------------------------
// Perturbated Hamiltonian Cycle algorithm
// see https://johnflux.com/2015/05/02/nokia-6110-part-3-algorithms/
//------------------------------------------------------------------------------

struct PerturbedHamiltonianCycle : Agent {
public:
  const bool use_shortest_path = false;

private:
  GridPath cycle;
  Grid<int> cycle_order;

public:
  PerturbedHamiltonianCycle(GridPath const& cycle)
    : cycle(cycle)
    , cycle_order(cycle.dimensions(), -1)
  {
    Coord c = {0,0};
    for (int i=0; i<cycle.size(); ++i) {
      cycle_order[c] = i;
      c = cycle[c];
    }
  }
  
  int cycle_distance(Coord a, Coord b) const {
    int order_a = cycle_order[a];
    int order_b = cycle_order[b];
    if (order_a < order_b) return order_b - order_a;
    else return order_b - order_a + cycle_order.size();
  }
  int cycle_distance_round_down(Coord a, Coord b) const {
    int order_a = cycle_order[a];
    int order_b = cycle_order[b];
    if (order_a <= order_b) return order_b - order_a;
    else return order_b - order_a + cycle_order.size();
  }
  
  Dir operator () (Game const& game) {
    Coord pos = game.snake_pos();
    Coord goal = game.apple_pos;
    Coord next = cycle[pos];
    // can we take a shortcut?
    // this is possible if a neighbor b is between pos and goal in the cycle order
    // but not after the tail, so  pos < b <= goal < tail
    // note: goal <= tail might not hold if we previously took shortcuts
    
    int dist_goal = cycle_distance(pos, goal);
    int dist_tail = cycle_distance(pos, game.snake.back());
    int dist_next = 1;
    int max_shortcut = std::min(dist_goal, dist_tail - 3);
    if (game.snake.size() > game.grid.size()*50/100) {
      max_shortcut = 0;
    }
    if (dist_goal < dist_tail) {
      max_shortcut --; // account for growth
      // we might find more apples along the way
      if ((dist_tail - dist_goal) * 4 > (game.grid.size() - game.snake.size())) max_shortcut -= 10;
    }
    
    if (max_shortcut > 0) {
      if (use_shortest_path) {
        auto edge = [&](Coord from, Coord to, Dir) {
          int dist_from = cycle_distance_round_down(pos, from);
          int dist_to   = cycle_distance(pos, to);
          if (dist_to > dist_from && dist_to < dist_tail && !game.grid[to]) {
            return 1;
          } else if (dist_to == dist_from + 1) {
            // can always move to next in cycle
            return 1;
          } else {
            return INT_MAX;
          }
        };
        Coord to = dist_goal < dist_tail ? goal : game.snake.back();
        auto paths = astar_shortest_path(game.grid.coords(), edge, pos, to);
        Coord better_next = first_step(paths, pos, to);
        if (game.grid.is_clear(better_next)) {
          next = better_next;
        }
      } else {
        // best shortcut direction
        for (Dir dir : dirs) {
          Coord b = pos + dir;
          if (game.grid.valid(b) && !game.grid[b]) {
            int dist_b = cycle_distance(pos, b);
            if (dist_b <= max_shortcut && dist_b > dist_next) {
              next = b;
              dist_next = dist_b;
            }
          }
        }
      }
    }
    return next - pos;
  }
};

//------------------------------------------------------------------------------
// Dynamic Hamiltonian Cycle Repair
//------------------------------------------------------------------------------

// see:
// Snake AI: Dynamic Hamiltonian Cycle Repair (with some strategic enhancements)
// 2019-2020, Brian Haidet, AlphaPhoenix, youtube.com/c/alphaphoenixchannel
// https://github.com/BrianHaidet/AlphaPhoenix/tree/master/Snake_AI_(2020a)_DHCR_with_strategy
//
// This is a reimplementation

// Here the idea is to maintain and update a Hamiltonian cycle

// Change a Hamiltonian cycle to have next[a] == d
// patches up the path so it remains a cycle
// returns success
bool repair_cycle(Grid<bool> const& grid, GridPath& next, Coord a, Coord d) {
  assert(is_neighbor(a,d));
  assert(is_hamiltonian_cycle(next));
  if (next[a] == d) return true; // already done
  // Path is [...,a,b,...,c,d,...]
  Coord b = next[a];
  Coord c = path_from(next, d);
  // Setting it to [...,a,d,...] would break off a path [b,..,c]
  // Can that be made into a cycle?
  if (is_neighbor(b,c)) {
    // [c,b,...] is also 
    // Mark cycle of nodes except those between a and d, call that cycle1
    Grid<bool> cycle1(grid.dimensions(),false);
    for (Coord x = d; x != a; x = next[x]) {
      assert(!cycle1[x]);
      cycle1[x] = true;
    }
    cycle1[a] = true;
    // Try to join [b,....,c] into another part of cycle1
    // We can do that if there are nodes u,v not in the path adjacent to two nodes x,y in the c..d-cycle
    // in that case we can change
    //   x -> y            x   y
    //             into    ↓   ↑
    //   v <- u            v   u
    // Only consider u,v that
    //  * do not currently contain the snake
    //  * are after the goal(?)
    for (Coord x=b; x!=c; x=next[x]) {
      Coord y = next[x];
      Dir xy = y-x;
      for (Dir dir : {rotate_clockwise(xy), rotate_counter_clockwise(xy)}) {
        Coord u = y+dir, v = x+dir;
        if (cycle1.valid(u) && cycle1.valid(v) && cycle1[u] && cycle1[v] && next[u] == v && !grid[u] && !grid[v]) {
          // we can fix the cycle
          /*
          std::cout << std::endl << "REPAIR" << std::endl;
          std::cout << cycle1;
          std::cout << draw_cycle(next, gray);
          auto g = draw_cycle2(next, white);
          g[a] = green("a");
          g[b] = green("b");
          g[c] = green("c");
          g[d] = green("d");
          g[x] = yellow("x");
          g[y] = yellow("y");
          g[u] = yellow("u");
          g[v] = yellow("v");
          std::cout << g;
          */
          next[a] = d;
          next[c] = b;
          next[x] = v;
          next[u] = y;
          //std::cout << draw_cycle(next, yellow);
          assert(is_hamiltonian_cycle(next));
          return true;
        }
      }
    }
  }
  /*
  auto g = draw_cycle2(next, white);
  g[a] = green("a");
  g[b] = green("b");
  g[c] = green("c");
  g[d] = green("d");
  std::cout << g;
  std::cout << "Repair failed" << std::endl;
  */
  return false;
}

Grid<int> cycle_distances(GridPath cycle, Coord goal) {
  Grid<int> dists(cycle.dimensions());
  Coord c = goal;
  int dist = cycle.size()-1;
  do {
    c = cycle[c];
    dists[c] = dist--;
  } while (c != goal);
  return dists;
}

struct DynamicHamiltonianCycleRepair : Agent {
  GridPath cycle;
  bool recalculate_path = true;
  const int wall_follow_overshoot = 0; // 0 to disable
  int wall_follow_mode = 0;
  std::vector<Coord> cached_path;
  
  DynamicHamiltonianCycleRepair(GridPath const& cycle) : cycle(cycle) {}
  
  Dir operator () (Game const& game) {
    Coord pos = game.snake_pos();
    Coord goal = game.apple_pos;
    // use cache?
    if (!cached_path.empty() && !recalculate_path) {
      Coord pos2 = cached_path.back();
      cached_path.pop_back();
      return pos2 - pos;
    }
    // distance to goal along current cycle
    auto cycle_distance = cycle_distances(cycle, goal);
    // find path to goal
    auto can_move = [&](Coord from, Coord to, Dir) {
      return !game.grid[to];
    };
    auto edge = [&](Coord from, Coord to, Dir dir) {
      return can_move(from,to,dir) ? 1000000 + cycle_distance[to] : INT_MAX;
    };
    auto dists = astar_shortest_path(game.grid.coords(), edge, pos, goal, 1000000);
    auto path = read_path(dists, pos, goal);
    Coord step = path.back();
    /*
    if (wall_follow_overshoot > 0) {
      // would this path make nodes unreachable?
      auto after = after_moves(game, path, Lookahead::many_keep_tail);
      auto unreachable = unreachables(can_move, after, dists);
      if (unreachable.any) {
        wall_follow_mode = wall_follow_overshoot; // called nascar mode in original code
      } else if (wall_follow_mode) {
        wall_follow_mode--;
      }
      if (wall_follow_mode > 0) {
        
      }
    }
    */
    // try to repair hamiltonian cycle
    if (cycle[pos] != step) {
      // cycle needs to be changed
      auto after = after_moves(game, path, Lookahead::one);
      
      if (!repair_cycle(after.grid, cycle, pos, path.back())) {
        // Failed to repair, continue along path
      }
    }
    // move along cycle
    return cycle[pos] - pos;
  }
};

