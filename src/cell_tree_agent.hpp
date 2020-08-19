#include "game.hpp"
#include "shortest_path.hpp"

//------------------------------------------------------------------------------
// Agent: tree based
//------------------------------------------------------------------------------

// Consider the grid to be a smaller grid of 2x2 cells, like a bunch of two lane streets.
// Each cell in the smaller grid can be one of 2^4-1 types, depending on which sides it is connected to (at least one)
// These connected cells form a tree, to ensure that the path they represent is a Hamiltonian cycle.

// We follow right-hand drive.
// For example the cell:
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

bool inside_tree_cell(int x, int y, Dir dir) {
  if (x == 1 && y == 0) return dir == Dir::left;
  if (x == 0 && y == 1) return dir == Dir::right;
  if (x == 0 && y == 0) return dir == Dir::down;
  if (x == 1 && y == 1) return dir == Dir::up;
  return false;
}

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
const CellCoord not_visited = {-1,-1};
const CellCoord root = {-2,-2};

// Find current tree (represented as parent pointers)
// note: the returned grid is only w/2 * h/2
// {-1,-1} indicates cell is not visited
// {-2,-2} indicates cell is root
Grid<Coord> cell_tree_parents(Game const& game) {
  Grid<CellCoord> parents(not_visited, w/2, h/2);
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
  // condition 2 (only move to parent or unvisted cell)
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
  bool prevent_unreachable = true;

  Dir operator () (Game const& game) {
    Coord pos = game.snake_pos();
    // Find shortest path satisfying 1,2
    auto cell_parents = cell_tree_parents(game);
    auto edge = [&game,&cell_parents](Coord a, Coord b, Dir dir) {
      if (can_move_in_cell_tree(cell_parents, a, b, dir) && !game.grid[b]) {
        // small penalty for moving inside the same cell, this should cause the path to hug walls more?
        return 1000 + 1*!inside_tree_cell(a.x%2, a.y%2, dir);
      } else {
        return INT_MAX;
      }
    };
    auto path = astar_shortest_path(edge, pos, game.apple_pos, 1000);
    auto pos2 = first_step(path, pos, game.apple_pos);
    
    // Heuristic 3A: prevent making parts of the grid unreachable with this move
    if (prevent_unreachable) {
      // what the grid looks like after moving to pos2
      Grid<bool> grid_after = game.grid;
      grid_after[pos2] = true;
      auto cell_parents_after = cell_parents;
      if (cell_parents_after[cell(pos2)] == not_visited) {
        cell_parents_after[cell(pos2)] = cell(pos);
      }
      auto can_move_after = [&grid_after,&cell_parents_after](Coord a, Coord b, Dir dir) {
        return can_move_in_cell_tree(cell_parents_after, a, b, dir) && !grid_after[b];
      };
      auto reachable = flood_fill(can_move_after, pos2);
      bool any_unreachable = false;
      for (auto a : coords) {
        if (!grid_after[a] && !reachable[a]) {
          any_unreachable = true;
          break;
        }
      }
      if (any_unreachable) {
        for (auto dir : dirs) {
          if (edge(pos,pos+dir,dir) != INT_MAX && pos+dir != pos2) {
            std::cout << game << "Moving " << dir << " instead of " << (pos2-pos) << " to avoid unreachables" << std::endl;
            return dir;
          }
        }
        std::cout << game;
        std::cout << "Unreachable grid points exist, but no alternative moves" << std::endl;
        throw "bad";
      }
    }
    /*
    if (false && !is_neighbor(c,c2)) {
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
    */
    return pos2 - pos;
  }
};

