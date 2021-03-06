#include "agent.hpp"
#include "game_util.hpp"
#include "shortest_path.hpp"

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
  
  Dir operator () (Game const& game, AgentLog* log = nullptr) override {
    if (log && game.turn == 0) {
      log->add(game.turn, AgentLog::Key::cycle, cycle_to_path(cycle));
    }
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
  int wall_follow_overshoot = 0; // 0 to disable
  int wall_follow_mode = 0;
  std::vector<Coord> cached_path;
  
  DynamicHamiltonianCycleRepair(GridPath const& cycle) : cycle(cycle) {}
  
  Dir operator () (Game const& game, AgentLog* log = nullptr) override {
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
    Coord target = path.back();
    // wall follow/nascar mode
    // I don't understand what the code is trying to do exactly
    if (wall_follow_overshoot > 0) {
      // would this path make nodes unreachable?
      auto after = after_moves(game, path, Lookahead::many_keep_tail);
      auto can_move_after = [&](Coord from, Coord to, Dir) {
        return !after.grid[to];
      };
      auto unreachable = unreachables(can_move_after, after, dists);
      if (unreachable.any) {
        wall_follow_mode = wall_follow_overshoot; // called nascar mode in original code;
      } else if (wall_follow_mode) {
        wall_follow_mode--;
      }
      if (wall_follow_mode > 0) {
        Coord prev = game.snake.size() <= 1 ? pos + Dir::down : game.snake[1];
        Dir last_dir = pos - prev;
        // always go clockwise/counterclockwise
        Dir turn = rotate_clockwise(last_dir);
        if (game.grid.is_clear(prev + turn)) {
          turn = - turn;
        }
        if (!game.grid.is_clear(prev + turn)) {
          if (game.grid.is_clear(pos + turn)) {
            target = pos + turn;
          } else if (game.grid.is_clear(pos + last_dir)) {
            target = pos + last_dir;
          }
        }
      }
    }
    // find chokepoints
    /*
    for (auto p : path) {
    }
    */
    // try to repair hamiltonian cycle
    bool changed_cycle = game.turn == 0;
    if (cycle[pos] != target) {
      // cycle needs to be changed
      if (!repair_cycle(game.grid, cycle, pos, target)) {
        // Failed to repair, continue along previous cycle
      } else {
        changed_cycle = true;
      }
    }
    if (log) {
      if (changed_cycle) {
        log->add(game.turn, AgentLog::Key::cycle, cycle_to_path(cycle));
      } else {
        log->add(game.turn, AgentLog::Key::cycle, AgentLog::CopyEntry{});
      }
      auto path_copy = path;
      path_copy.push_back(pos);
      log->add(game.turn, AgentLog::Key::plan, std::move(path_copy));
    }
    // move along cycle
    return cycle[pos] - pos;
  }
};

