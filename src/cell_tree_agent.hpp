#include "game.hpp"
#include "shortest_path.hpp"

//------------------------------------------------------------------------------
// Look ahead to what would happen if we were to follow a path
//------------------------------------------------------------------------------

enum class Lookahead {
  one,            // only look at what would happen with the move about to be made
  many_keep_tail, // extend the snake along the path, keeping the current tail
  many_move_tail, // move the snake along the path, also moving the tail
};

GameBase after_moves(GameBase const& game, std::vector<Coord> const& path, Lookahead lookahead) {
  assert(is_neighbor(path.back(), game.snake_pos()));
  GameBase after = game;
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
};

// Find unreachable parts of the grid
// Usually used with after_moves()
template <typename CanMove, typename GameLike>
Unreachables unreachables(CanMove can_move, GameLike const& game, Grid<Step> const& dists) {
  // are there unreachable coords?
  auto reachable = flood_fill(game.dimensions(), can_move, game.snake_pos());
  Unreachables out;
  for (auto a : game.grid.coords()) {
    if (!game.grid[a] && !reachable[a]) {
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

const Dir cell_moves_inside[] =
  {Dir::down,  Dir::left,
   Dir::right, Dir::up};
const Dir cell_moves_outside[] =
  {Dir::left, Dir::up,
   Dir::down, Dir::right};

// Direction that stays inside the cell
inline Dir cell_move_inside(Coord c) {
  return cell_moves_inside[(c.y%2)*2 + c.x%2];
}

// Direction that moves out of the cell
inline Dir cell_move_outside(Coord c) {
  return cell_moves_outside[(c.y%2)*2 + c.x%2];
}

inline bool is_cell_move(Coord c, Dir dir) {
  return cell_move_inside(c) == dir || cell_move_outside(c) == dir;
}


bool can_move_in_tree_cell(int x, int y, Dir dir) {
  if (x == 1 && y == 0) return dir == Dir::up    || dir == Dir::left;
  if (x == 0 && y == 1) return dir == Dir::down  || dir == Dir::right;
  if (x == 0 && y == 0) return dir == Dir::left  || dir == Dir::down;
  if (x == 1 && y == 1) return dir == Dir::right || dir == Dir::up;
  return false;
}

//------------------------------------------------------------------------------
// Agent: tree based
//------------------------------------------------------------------------------

// We should be able to use a shortest-path algorithm on the original snake-level that maintains the cell and tree constraints
// 1. Cell contraint means that only some moves are possible (2 dirs in each coord instead of 4)
//    See can_move_in_tree_cell
// 2. Tree constraint means that we can't move into a cell except from its direct children
//    If we take the snake's tail to be the root of the tree then
//     * moving to parent from child means retracing our steps, this is always possible
//     * moving to unvisited cells is always possible
//     * moving to an existing child from a parent never happens
// 3. We have to be able to cover all cells with a tree.
//    Equivalently, 
// Condition 1 and 2 are doable, but combining with 3 is (probably) NP-hard (it is in the general graph case)

// Simple heuristic (3A):
//  * use shortest path satisfying 1,2
//  * if the resulting move makes some parts of the grid unreachable, then perform the another move instead
//    (there are always at most two possible moves)

// Better(?) heuristic (3B):
//  * first find shortest path satisfying 1,2.
//  * then check the state after following the path to the goal.
//  * if some coords become unreachable at that time, then we have clearly failed to maintain a tree.
//  * in that case, instead use the shortest path to one of the unreachable cells 

// Bonus (4):
// It would also be good to hug walls, to avoid creating large almost closed regions
// that could be added as a factor to the shortest path code

using CellCoord = Coord;
CellCoord cell(Coord c) {
  return {c.x/2, c.y/2};
}

// Find current tree (represented as parent pointers)
// note: the returned grid is only w/2 * h/2
// {-1,-1} indicates cell is not visited
// {-2,-2} indicates cell is root
Grid<CellCoord> cell_tree_parents(CoordRange dims, RingBuffer<Coord> const& snake) {
  Grid<CellCoord> parents(dims.w/2, dims.h/2, NOT_VISITED);
  CellCoord parent = ROOT;
  for (int i=snake.size()-1 ; i >= 0; --i) {
    Coord c = snake[i];
    CellCoord cell_coord = cell(c);
    if (parents[cell_coord] == NOT_VISITED) {
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
  // condition 2 (only move to parent or unvisted cell)
  Coord cell_a = cell(a);
  Coord cell_b = cell(b);
  return cell_b == cell_a || cell_parents[cell_b] == NOT_VISITED || cell_parents[cell_a] == cell_b;
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


Unreachables cell_tree_unreachables(GameBase const& game, Grid<Step> const& dists) {
  auto cell_parents = cell_tree_parents(game.dimensions(), game.snake);
  auto can_move = [&](Coord from, Coord to, Dir dir) {
    return can_move_in_cell_tree(cell_parents, from, to, dir) && !game.grid[to];
  };
  return unreachables(can_move, game, dists);
}

enum class DetourStrategy {
  none,
  any,
  nearest_unreachable
};

struct CellTreeAgent : Agent {
public:
  // config
  bool recalculate_path = true;
  Lookahead lookahead = Lookahead::many_move_tail;
  DetourStrategy detour = DetourStrategy::nearest_unreachable;
  // penalties
  int same_cell_penalty = 0;
  int new_cell_penalty = 0;
  int parent_cell_penalty = 0;

private:
  std::vector<Coord> cached_path;

public:
  Dir operator () (Game const& game) {
    Coord pos = game.snake_pos();
    if (!cached_path.empty() && !recalculate_path) {
      Coord pos2 = cached_path.back();
      cached_path.pop_back();
      return pos2 - pos;
    }
    
    // Find shortest path satisfying 1,2
    auto cell_parents = cell_tree_parents(game.dimensions(), game.snake);
    auto edge = [&](Coord a, Coord b, Dir dir) {
      if (can_move_in_cell_tree(cell_parents, a, b, dir) && !game.grid[b]) {
        // small penalty for moving to same/different cell
        bool to_parent = cell(b) == cell_parents[cell(a)];
        bool to_same   = cell(b) == cell(a);
        return 1000
          + (to_parent ? parent_cell_penalty : to_same ? same_cell_penalty : new_cell_penalty);
      } else {
        return INT_MAX;
      }
    };
    auto dists = astar_shortest_path(game.grid.coords(), edge, pos, game.apple_pos, 1000);
    auto path = read_path(dists, pos, game.apple_pos);
    auto pos2 = path.back();
    
    // Heuristic 3: prevent making parts of the grid unreachable
    if (detour != DetourStrategy::none) {
      auto after = after_moves(game, path, lookahead);
      auto unreachable = cell_tree_unreachables(after, dists);
      if (unreachable.any) {
        if (detour == DetourStrategy::any) {
          // 3A: move in any other direction
          for (auto dir : dirs) {
            if (edge(pos,pos+dir,dir) != INT_MAX && pos+dir != pos2) {
              //std::cout << game << "Moving " << dir << " instead of " << (pos2-pos) << " to avoid unreachables" << std::endl;
              cached_path.clear();
              return dir;
            }
          }
        } else if (detour == DetourStrategy::nearest_unreachable) {
          // 3B: move to one of the unreachable coords
          if (unreachable.dist_to_nearest < INT_MAX) {
            // move to an unreachable coord first
            pos2 = first_step(dists, pos, unreachable.nearest);
            cached_path.clear();
            return pos2 - pos;
          }
          // failed to find detour
          // This can happen because it previously looked like everything would be reachable upon reaching the apple,
          // but moving the snake's tail opened up a shorter path
          // Solution: just continue along previous path
          if (!cached_path.empty()) {
            pos2 = cached_path.back();
            cached_path.pop_back();
            return pos2 - pos;
          }
        }
        if (0) {
          std::cout << game;
          std::cout << "Unreachable grid points (will) exist, but no alternative moves or cached path" << std::endl;
        }
      }
    }
    
    // use as new cached path
    cached_path = std::move(path);
    cached_path.pop_back();
    
    return pos2 - pos;
  }
};

