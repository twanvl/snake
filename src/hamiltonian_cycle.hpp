
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


//------------------------------------------------------------------------------
// Perturbated Hamiltonian Cycle algorithm
// see https://johnlfux.com/
//------------------------------------------------------------------------------

struct PerturbedHamiltonianCycle {
  GridPath cycle;
  Grid<int> cycle_order;
  
  const bool use_shortest_path = false;
  
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
  
  Dir operator () (Game const& game) const {
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

// Change a Hamiltonian cycle to have path[a] == b
// patches up the path so it remains a cycle
// returns success
bool repair_cycle(GridPath& cycle, Coord a, Coord b) {
  if (cycle[a] == b) return true; // already done
  // Path is [...,a,c,...,d,b,...]
  Coord c = cycle[a];
  Coord d = path_from(cycle, b);
  // Setting it to [...,a,b,...] would break off a path c -> ... -> d
  // Can that be made into a cycle?
  if (is_neighbor(c,d)) {
    // Try to join [c,...,d] into another part of the path
    // We can do that if there are nodes u,v not in the path adjacent to two nodes x,y in the c..d-cycle
    // in that case we can change
    //   u -> v            u   v
    //             into    ↓   ↑
    //   x <- y            x   y
    // Only consider u,v that
    //  * do not currently contain the snake
    //  * are after the goal(?)
    Coord x=d, y=c; // [d,c], [c,c+], ... ,[d-,d]
    for (Coord x=c, y=d; x!=c; x=y,y=cycle[x]) {
      y=cycle[x];
    }
  }
  return false;
}

struct DynamicHamiltonianCycleRepair {
  GridPath cycle;
  bool recalculate_path = true;
  const int wall_follow_overshoot = 0; // 0 to disable
  int wall_follow_mode = 0;
  std::vector<Coord> cached_path;
  
  Dir operator () (Game const& game) {
    Coord pos = game.snake_pos();
    Coord goal = game.apple_pos;
    // use cache?
    if (!cached_path.empty() && !recalculate_path) {
      Coord pos2 = cached_path.back();
      cached_path.pop_back();
      return pos2 - pos;
    }
    // find path to goal
    auto can_move = [&](Coord from, Coord to, Dir) {
      return !game.grid[to];
    };
    auto edge = [&](Coord from, Coord to, Dir dir) {
      return can_move(from,to,dir) ? 1 : INT_MAX;
    };
    auto dists = astar_shortest_path(game.grid.coords(), edge, pos, goal);
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
      
      repair_cycle(cycle, pos, path.back());
    }
    // move along cycle
    return cycle[pos] - pos;
  }
};

