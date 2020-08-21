#include "game.hpp"

//------------------------------------------------------------------------------
// Agent: Fixed path agent
//------------------------------------------------------------------------------

// A zig-zag path:
//  go up and down while going right, then move back left along the top row
Dir zig_zag_path(CoordRange dims, Coord c) {
  if (c.y == 0 && c.x > 0) {
    return Dir::left;
  } else if (c.x % 2 == 0) {
    if (c.y == dims.h-1) {
      return Dir::right;
    } else {
      return Dir::down;
    }
  } else {
    if (c.y == 1 && c.x != dims.w-1) {
      return Dir::right;
    } else {
      return Dir::up;
    }
  }
}

// Follow a fixed path
struct FixedAgent : Agent {
  Dir operator () (Game const& game) {
    Coord c = game.snake_pos();
    return zig_zag_path(game.grid.coords(), c);
  }
};

//------------------------------------------------------------------------------
// Agent: Fixed path agent (alternative)
//------------------------------------------------------------------------------

// A Hamiltonian cycle
Grid<Coord> make_path(CoordRange dims) {
  Grid<Coord> path(dims);
  for (auto c : dims) {
    path[c] = c + zig_zag_path(dims, c);
  }
  return path;
}

// agent that maintains a hamiltonian path
struct FixedCycleAgent {
  Grid<Coord> path;
  
  Dir operator () (Game const& game) {
    Coord c = game.snake_pos();
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

const int MAX_W = 30;
struct CutAgent : Agent {
  int cuts[MAX_W] = {1};
  bool move_right = true;
  bool quick_dir_change = true;

  Dir operator () (Game const& game) {
    Coord c = game.snake_pos();
    Coord target = game.apple_pos;
    int w = game.grid.w, h = game.grid.h;
    Grid<bool> const& grid = game.grid;
    if (c.x == 0) move_right = true;
    if (c.x == w-1 || (c.y == 0 && c.x > 0)) move_right = false;
    if (move_right) {
      if (c.x % 2 == 0) {
        if (quick_dir_change && target.x < c.x && game.snake.size() < game.grid.size()/4 && !any(grid,c.x+1,w,0,h) && !grid[{c.x,c.y-1}]) {
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

