#pragma once

#include "game.hpp"
#include "shortest_path.hpp"
#include <assert.h>
#include <climits>

//------------------------------------------------------------------------------
// Look ahead to what would happen if we were to follow a path
//------------------------------------------------------------------------------

enum class Lookahead {
  one,            // only look at what would happen with the move about to be made
  many_keep_tail, // extend the snake along the path, keeping the current tail
  many_move_tail, // move the snake along the path, also moving the tail
};

GameBase after_moves(GameBase const& game, std::vector<Coord> const& path, Lookahead lookahead) {
  GameBase after = game;
  assert(is_neighbor(path.back(), game.snake_pos()));
  if (lookahead == Lookahead::one) {
    auto pos_after = path.back();
    after.grid[pos_after] = true;
    after.snake.push_front(pos_after);
  } else {
    for (auto it = path.rbegin() ; it != path.rend() ; ++it) {
      auto p = *it;
      after.grid[p] = true;
      after.snake.push_front(p);
      if (lookahead == Lookahead::many_move_tail && p != game.apple_pos) {
        after.grid[after.snake.back()] = false;
        after.snake.pop_back();
      }
    }
  }
  return after;
}

//------------------------------------------------------------------------------
// Unreachable parts of the grid
//------------------------------------------------------------------------------

struct Unreachables {
  bool any = false;
  Coord nearest = {-1,-1};
  int dist_to_nearest = INT_MAX;
  Grid<bool> reachable;
  
  Unreachables(Grid<bool> const& reachable)
    : reachable(reachable)
  {}
};

// Find parts of the grid that are unreachable from the snake's position
// Usually used with after_moves()
//
// Note: this is not exactly the same as the snake splitting the grid into two (or more) parts
template <typename CanMove, typename GameLike>
Unreachables unreachables(CanMove can_move, GameLike const& game, Grid<Step> const& dists) {
  // are there unreachable coords?
  Unreachables out = flood_fill(game.dimensions(), can_move, game.snake_pos());
  for (auto a : game.grid.coords()) {
    if (game.grid[a]) {
      out.reachable[a] = true; // count grid cells containing the snake as reachable
    } else if (!out.reachable[a]) {
      out.any = true;
      if (dists[a].dist < out.dist_to_nearest) {
        out.nearest = a;
        out.dist_to_nearest = dists[a].dist;
      }
    }
  }
  return out;
}

//------------------------------------------------------------------------------
// Cell moves
//------------------------------------------------------------------------------

// Consider the grid to be a smaller grid of 2x2 cells, like a bunch of two lane streets.
// Each cell in the smaller grid can be one of 2^4-1 types, depending on which sides it is connected to (at least one)
// When the connected cells form a spanning tree, the path they represent is a Hamiltonian cycle.

// We follow right-hand drive.
// For example the cell:
//   #←#←
//   ↓
//   # #→
//   ↓ ↑
// is connected to bottom and right

// Direction that stays inside the cell
inline constexpr Dir cell_move_inside(Coord c) {
  if ((c.y & 1) == 0) {
    return (c.x & 1) == 0 ? Dir::down : Dir::left;
  } else {
    return (c.x & 1) == 0 ? Dir::right : Dir::up;
  }
}

// Direction that moves out of the cell
inline constexpr Dir cell_move_outside(Coord c) {
  if ((c.y & 1) == 0) {
    return (c.x & 1) == 0 ? Dir::left : Dir::up;
  } else {
    return (c.x & 1) == 0 ? Dir::down : Dir::right;
  }
}

inline constexpr bool is_cell_move(Coord c, Dir dir) {
  return cell_move_inside(c) == dir || cell_move_outside(c) == dir;
}

using CellCoord = Coord;
inline constexpr CellCoord cell(Coord c) {
  return {c.x/2, c.y/2};
}


//------------------------------------------------------------------------------
// Hamiltonian cycles
//------------------------------------------------------------------------------

// A Hamiltonian cycle represented as: at each grid point the coordinate of the next point
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
  for (Coord c : path.coords()) {
    Coord in  = c + cell_move_inside(c);
    Coord out = c + cell_move_outside(c);
    CellCoord c_cell = cell(c);
    CellCoord o_cell = cell(out);
    assert(!path.valid(out) || o_cell != c_cell);
    if (path.valid(out) && (parent[o_cell] == c_cell || parent[c_cell] == o_cell)) {
      path[c] = out;
    } else {
      path[c] = in;
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


// distance between two points along the Hamiltonian cycle
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

// step in reverse path
Coord path_from(GridPath const& path, Coord to) {
  if (to.x > 0        && path[{to.x-1, to.y}] == to) return Coord{to.x-1, to.y};
  if (to.x < path.w-1 && path[{to.x+1, to.y}] == to) return Coord{to.x+1, to.y};
  if (to.y > 0        && path[{to.x, to.y-1}] == to) return Coord{to.x, to.y-1};
  if (to.y < path.h-1 && path[{to.x, to.y+1}] == to) return Coord{to.x, to.y+1};
  throw std::logic_error("No path from neighbor");
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


std::vector<Coord> cycle_to_path(GridPath const& cycle) {
  std::vector<Coord> path;
  for (Coord c = {0,0};;) {
    path.push_back(c);
    c = cycle[c];
    if (c == Coord{0,0}) break;
  }
  return path;
}

template <typename Color>
Grid<std::string> draw_cycle(GridPath const& cycle, Color color) {
  Grid<std::string> grid(cycle.dimensions(), ".");
  draw_path(grid, cycle_to_path(cycle), color, true);
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

