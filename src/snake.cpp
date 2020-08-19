#include "util.hpp"
#include "game.hpp"
#include <unistd.h>
#include <climits>
#include <cmath>

//------------------------------------------------------------------------------
// Agent: Fixed path agent
//------------------------------------------------------------------------------

// A zig-zag path:
//  go up and down while going right, then move back left along the top row
Dir zig_zag_path(Coord c) {
  if (c.y == 0 && c.x > 0) {
    return Dir::left;
  } else if (c.x % 2 == 0) {
    if (c.y == h-1) {
      return Dir::right;
    } else {
      return Dir::down;
    }
  } else {
    if (c.y == 1 && c.x != w-1) {
      return Dir::right;
    } else {
      return Dir::up;
    }
  }
}

struct FixedAgent : Agent {
  Dir operator () (Game const& game) {
    Coord c = game.snake.front();
    return zig_zag_path(c);
  }
};

//------------------------------------------------------------------------------
// Agent: Fixed path agent (alternative)
//------------------------------------------------------------------------------

// A Hamiltonian cycle
Grid<Coord> make_path() {
  Grid<Coord> path;
  for (auto c : coords) {
    path[c] = c + zig_zag_path(c);
  }
  return path;
}

// agent that maintains a hamiltonian path
struct FixedPathAgent {
  Grid<Coord> path;
  
  Dir operator () (Game const& game) {
    Coord c = game.snake.front();
    return path[c] - c;
  }
};

//------------------------------------------------------------------------------
// Agent: Fixed path with shortcuts
//------------------------------------------------------------------------------

bool any(Grid<bool> const& grid, int x0, int x1, int y0, int y1) {
  for (int y=y0; y<y1; ++y) {
    for (int x=x0; x<x1; ++x) {
      if (grid[{x,y}]) return true;
    }
  }
  return false;
}

struct CutAgent : Agent {
  int cuts[w] = {1};
  bool move_right = true;
  bool quick_dir_change = true;

  Dir operator () (Game const& game) {
    Coord c = game.snake.front();
    Coord target = game.apple_pos;
    Grid<bool> const& grid = game.grid;
    if (c.x == 0) move_right = true;
    if (c.x == w-1) move_right = false;
    if (move_right) {
      if (c.x % 2 == 0) {
        if (quick_dir_change && target.x < c.x && game.snake.size() < w*h/4 && !any(grid,c.x+1,w,0,h) && !grid[{c.x,c.y-1}]) {
          move_right = false;
          return Dir::up;
        }
        if (c.y == h-1) {
          return Dir::right;
        } else {
          return Dir::down;
        }
      } else {
        // take a shortcut?
        if (c.y <= 1) {
          return Dir::right; // forced (would hit top)
        } if (grid[{c.x,c.y-1}]) {
          return Dir::right; // forced (snake in the way)
        } else if (any(grid, c.x, c.x+2, 0, c.y-1)) {
          // don't take shortcut, that would create a gap
          return Dir::up;
        } else if (target.x > c.x+1 || (target.x == c.x+1 && target.y >= c.y)) {
          return Dir::right; // shortcut
        } else {
          if (quick_dir_change && target.x < c.x) move_right = false;
          return Dir::up;
        }
      }
    } else {
      if (c.x % 2 == 1) {
        if (quick_dir_change && target.x > c.x && game.snake.size() < w*h/4 && !any(grid,0,c.x,0,h) && !grid[{c.x,c.y+1}]) {
          move_right = true;
          return Dir::down;
        }
        if (c.y == 0) {
          return Dir::left;
        } else {
          return Dir::up;
        }
      } else {
        // take a shortcut?
        if (c.y >= w-2) {
          return Dir::left; // forced (would hit bottom)
        } if (grid[{c.x,c.y+1}]) {
          return Dir::left; // forced (snake in the way)
        } else if (any(grid, c.x-1, c.x+1, c.y+1, h)) {
          // don't take shortcut, that would create a gap
          return Dir::down;
        } else if (target.x < c.x-1 || (target.x == c.x-1 && target.y <= c.y)) {
          return Dir::left; // shortcut
        } else {
          if (quick_dir_change && target.x > c.x) move_right = true;
          return Dir::down;
        }
      }
    }
  }
};

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
  for (int i=0; i < w*h; ++i) {
    if (!is_neighbor(pos, path[pos])) return false;
    pos = path[pos];
    if (pos == Coord{0,0}) {
      return (i==w*h-1);
    }
  }
  return false;
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
  GridPath reverse;
  for (auto pos : coords) {
    reverse[path[pos]] = pos;
  }
  return reverse;
}

Coord path_from(GridPath const& path, Coord to) {
  if (to.x > 0   && path[{to.x-1, to.y}] == to) return Coord{to.x-1, to.y};
  if (to.x < w-1 && path[{to.x+1, to.y}] == to) return Coord{to.x+1, to.y};
  if (to.y > 0   && path[{to.x, to.y-1}] == to) return Coord{to.x, to.y-1};
  if (to.y < h-1 && path[{to.x, to.y+1}] == to) return Coord{to.x, to.y+1};
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

// Change a path to have path[a] == b
// patches up the path so it remains a cycle
// returns success
bool patch_path(GridPath& path, Coord a, Coord b) {
  if (path[a] == b) return true; // already done
  // Path is [...,a,c,...,d,b,...]
  Coord c = path[a];
  Coord d = path_from(path, b);
  // Setting it to [...,a,b,...] would break off a path c -> ... -> d
  // Can that be made into a cycle?
  if (is_neighbor(c,d)) {
    // Try to join [c,...,d] into another part of the path
    // We can do that if there are nodes u,v not in the path adjacent to two nodes x,y in the c..d-cycle
    // in that case we can change
    //   u -> v            u   v
    //             into    ↓   ↑
    //   x <- y            x   y
    // Only consider u,v that do not currently contain the snake
  }
  return false;
}

//------------------------------------------------------------------------------
// Shortest paths
//------------------------------------------------------------------------------

// Find shortest paths using breath first search
struct Step {
  int dist;
  Coord from;
};

template <typename CanMove>
Grid<Step> generic_shortest_path(CanMove const& can_move, Coord from, Coord to = {-1,-1}) {
  Grid<Step> out(Step{INT_MAX,{-1,-1}});
  std::vector<Coord> queue, next;
  queue.push_back(from);
  out[from].dist = 0;
  int dist = 0;
  while (!queue.empty()) {
    dist++;
    for (auto a : queue) {
      for (auto d : dirs) {
        Coord b = a + d;
        if (valid(b) && can_move(a,b,d) && out[b].dist > dist) {
          out[b].dist = dist;
          out[b].from = a;
          next.push_back(b);
          if (b == to) return out;
        }
      }
    }
    std::swap(queue,next);
    next.clear();
  }
  return out;
}

Grid<Step> shortest_path(Grid<bool> const& grid, Coord from, Coord to = {-1,-1}) {
  return generic_shortest_path([&grid](Coord a, Coord b, Dir d){ return !grid[b]; }, from, to);
}

Coord first_step(Grid<Step> const& path, Coord from, Coord to) {
  while (to != Coord{-1,-1} && path[to].from != from) {
    to = path[to].from;
  }
  return to;
}

//------------------------------------------------------------------------------
// Agent: tree based
//------------------------------------------------------------------------------

// Consider the grid to be a smaller grid of 2x2 cells, like a two lane street.
// Each cell can be one of 2^4-1 types, depending on which sides it is connected to (at least one)
// These connected cells form a tree, to ensure that the path they represent is a Hamiltonian cycle.

// We follow right-hand drive.
// example cells:
//   #←#←
//   ↓
//   # #→
//   ↓ ↑
// is connected to bottom and right

Dir tree_cell_move(bool connected[4], int x, int y) {
  if (x == 1 && y == 0) return connected[0] ? Dir::up    : Dir::left;
  if (x == 0 && y == 1) return connected[1] ? Dir::down  : Dir::right;
  if (x == 0 && y == 0) return connected[2] ? Dir::left  : Dir::down;
  if (x == 1 && y == 1) return connected[3] ? Dir::right : Dir::up;
  throw "tree_cell_move";
}

bool can_move_in_tree_cell(int x, int y, Dir dir) {
  if (x == 1 && y == 0) return dir == Dir::up    || dir == Dir::left;
  if (x == 0 && y == 1) return dir == Dir::down  || dir == Dir::right;
  if (x == 0 && y == 0) return dir == Dir::left  || dir == Dir::down;
  if (x == 1 && y == 1) return dir == Dir::right || dir == Dir::up;
  return false;
}

// We should be able to use a shortest-path algorithm on the original snake-level that maintains the cell and tree constraints
// 1. cell contraint means that only some moves are possible (2 dirs in each coord instead of 4)
// 2. tree constraint means that we can't move into a cell except from its direct children
//     * moving to parent from child means retracing steps, this is always possible
//     * moving to unvisited is always possible
//     * moving to child from parent should be considered when we are retracing our tail
//         that is only needed after breaking condition 3
//     (moving in from parent shouldn't happen if the snake's tail is the tree's root, I think)
// 3. We still have to cover all cells with a tree.
//    That means that there must be no
// 
// Condition 1 and 2 are doable, but combining with 3 is (probably) NP-hard.
// Heuristic:
//  * first find shortest path satisfying 1,2.
//  * then check the state after following the path to the goal.
//  * if some coords become unreachable at that time, then we have clearly failed to maintain a tree.
//  * in that case, instead use the shortest path to one of the unreachable cells 

// 
//    * if connected status can remain ambiguous that is fine
// Condition 3 is probably not important if the snake doesn't suddenly grow continuously (very unlikely)
// Actually, 3 is important, we can cut off a region

// To check for 3:
//  * update/extend tree as well when finding shortest paths (updating cell parents pointers),
//  * pay for marked cells that become unreachable when making a certain move
// or
//  * include visited cells in state space that is searched? Makes space exponentially large!

// If tail of the snake is in the root node...

using CellCoord = Coord;
CellCoord cell(Coord c) {
  return {c.x/2, c.y/2};
}
const CellCoord not_visited = {-1,-1};
const CellCoord root = {-2,-2};

// Find current tree (represented as parent pointers)
// note: the returned grid is only w/2 * h/2
// {-1,-1} indicates cell is not visited
// {-2,-2} indicates cell is root
Grid<Coord> cell_tree_parents(Game const& game) {
  Grid<CellCoord> parents(not_visited);
  CellCoord parent = root;
  for (int i=game.snake.size()-1 ; i >= 0; --i) {
    Coord c = game.snake[i];
    CellCoord cell_coord = cell(c);
    if (parents[cell_coord] == not_visited) {
      parents[cell_coord] = parent;
    }
    parent = cell_coord;
  }
  return parents;
}

// can you move from a to b?
bool can_move_in_cell_tree(Grid<Coord> const& cell_parents, Coord a, Coord b, Dir dir) {
  // condition 1
  if (!can_move_in_tree_cell(a.x % 2, a.y % 2, dir)) return false;
  // condition 2 (only move to parent or new
  Coord cell_a = cell(a);
  Coord cell_b = cell(b);
  return cell_b == cell_a || cell_parents[cell_b] == not_visited || cell_parents[cell_a] == cell_b;
}

Dir move_to_parent(Grid<Coord> const& cell_parents, Coord a) {
  Coord cell_a = cell(a);
  Coord parent = cell_parents[cell_a];
  int x = a.x % 2, y = a.y % 2;
  if (x == 1 && y == 0) return parent.y < cell_a.y ? Dir::up    : Dir::left;
  if (x == 0 && y == 1) return parent.y > cell_a.y ? Dir::down  : Dir::right;
  if (x == 0 && y == 0) return parent.x < cell_a.x ? Dir::left  : Dir::down;
  if (x == 1 && y == 1) return parent.x > cell_a.x ? Dir::right : Dir::up;
  throw "move_to_parent";
}

struct CellTreeAgent {
  Dir operator () (Game const& game) {
    Coord c = game.snake.front();
    auto cell_parents = cell_tree_parents(game);
    auto path = generic_shortest_path(
        [&game,&cell_parents](Coord a, Coord b, Dir dir) {
          return can_move_in_cell_tree(cell_parents, a, b, dir) && !game.grid[b];
        }, c, game.apple_pos);
    auto c2 = first_step(path, c, game.apple_pos);
    if (!is_neighbor(c,c2)) {
      // no path, chase our tail until we escape
      // but be careful not to bump into our tail
      if (cell_parents[cell(c)] == root) {
        // About to eat tail
        for (auto dir : dirs) {
          auto b = c + dir;
          if (can_move_in_cell_tree(cell_parents, c, b, dir) && !game.grid[b]) return dir;
        }
        std::cout << "About to eat tail" << std::endl;
        std::cout << game;
        std::cout << std::endl;
      }
      return move_to_parent(cell_parents, c);
      // debug
      std::cout << std::endl;
      std::cout << "No path" << std::endl;
      std::cout << game;
      std::cout << std::endl;
      for (int y=0; 2*y<h; ++y) {
        for (int x=0; 2*x<w; ++x) {
          printf("%x%x",x,y);
        }
        printf("\n");
        for (int x=0; 2*x<w; ++x) {
          Coord p = cell_parents[{x,y}];
          if (p == Coord{-1,-1}) printf("--");
          else if (p == Coord{-2,-2}) printf("RR");
          else printf("%x%x",p.x,p.y);
        }
        printf("\n");
      }
      std::cout << std::endl;
      for (int y=0; y<h; ++y) {
        for (int x=0; x<w; ++x) {
          auto p = path[{x,y}];
          if (p.dist < INT_MAX) std::cout << std::min(p.dist,9);
          else std::cout << "-";
        }
        std::cout << std::endl;
      }
      std::cout << std::endl;
    }
    //std::cout << (c2 - c);
    if (!can_move_in_tree_cell(c.x % 2, c.y % 2, c2 - c)) {
      std::cout << game;
    }
    return c2 - c;
  }
};

//------------------------------------------------------------------------------
// Statistics utilities
//------------------------------------------------------------------------------

template <typename T>
double mean(std::vector<T> const& xs) {
  // work around emscripten bug (missing accumulate function)
  //return std::accumulate(xs.begin(), xs.end(), 0.) / xs.size();
  double sum = 0.;
  for(auto x : xs) sum += x;
  return sum / std::max(1, (int)xs.size());
}

template <typename T>
double variance(std::vector<T> const& xs) {
  double m = mean(xs);
  double sum = 0.;
  for(auto x : xs) sum += (x-m)*(x-m);
  return sum / std::max(1, (int)xs.size()-1);
}

//------------------------------------------------------------------------------
// Playing full games
//------------------------------------------------------------------------------

template <typename Agent>
void play(Game& game, Agent agent, bool visualize = false) {
  while (!game.done()) {
    if (visualize) {
      std::cout << "\033[H";
      std::cout << game;
      usleep(game.turn < 100 ? 20000 : game.turn < 1000 ? 2000 : 200);
    }
    //std::cout << game;
    game.move(agent(game));
    if (game.turn > 1000000) game.state = Game::State::loss;
  }
}
struct Stats {
  std::vector<int> turns;
  std::vector<bool> wins;
};

template <typename AgentGen>
Stats play_multiple(AgentGen make_agent, int n = 100) {
  Stats stats;
  for (int i = 0; i < n; ++i) {
    Game game;
    play(game, make_agent());
    stats.turns.push_back(game.turn);
    stats.wins.push_back(game.win());
    if (!game.win()) std::cerr << game;
  }
  return stats;
}


//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

int main() {
  std::cout << "\033[H\033[J";
  Game game;
  //play(game, [](Game const&) { return Dir::left; }, true);
  //play(game, FixedAgent{}, false);
  //play(game, FixedPathAgent{make_path()}, false);
  //play(game, CutAgent{}, false);
  play(game, CellTreeAgent{}, false);
  std::cout << game;
  return 0;
  //auto stats = play_multiple([]{return FixedAgent{};});
  auto stats = play_multiple([]{return CutAgent{};});
  std::cout << "turns: mean " << mean(stats.turns) << ", stddev " << std::sqrt(variance(stats.turns)) << std::endl;
  if (mean(stats.wins) < 1) {
    std::cout << "LOST: " << (1-mean(stats.wins))*100 << "%" << std::endl;
  }
}

