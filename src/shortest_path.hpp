#pragma once

#include "util.hpp"
#include <climits>
#include <queue>
#include <vector>

//------------------------------------------------------------------------------
// Shortest paths by breath first search
//------------------------------------------------------------------------------

// Find shortest paths using breath first search
struct Step {
  int dist;
  Coord from;
  bool reachable() const {
    return dist < INT_MAX;
  }
};

template <typename CanMove>
Grid<Step> generic_shortest_path(CoordRange dims, CanMove const& can_move, Coord from, Coord to = {-1,-1}) {
  Grid<Step> out(dims, Step{INT_MAX, NOT_VISITED});
  std::vector<Coord> queue, next;
  queue.push_back(from);
  out[from].dist = 0;
  out[from].from = ROOT;
  int dist = 0;
  while (!queue.empty()) {
    dist++;
    for (auto a : queue) {
      for (auto d : dirs) {
        Coord b = a + d;
        if (dims.valid(b) && can_move(a,b,d) && out[b].dist > dist) {
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
  return generic_shortest_path(grid.coords(), [&grid](Coord a, Coord b, Dir d){ return !grid[b]; }, from, to);
}

Coord first_step(Grid<Step> const& path, Coord from, Coord to) {
  while (to != ROOT && path[to].from != from) {
    if (to == NOT_VISITED) break;
    to = path[to].from;
  }
  return to;
}

// Find all coords in a path from from to to, excluding the start point
// Note: returned in reverse order, that is result.back() is the first step, result.front() == to
std::vector<Coord> read_path(Grid<Step> const& paths, Coord from, Coord to) {
  std::vector<Coord> steps;
  while (to != ROOT && to != from) {
    steps.push_back(to);
    if (to == NOT_VISITED) break;
    to = paths[to].from;
  }
  return steps;
}

std::ostream& operator << (std::ostream& out, Grid<Step> const& paths) {
  Grid<std::string> vis(paths.dimensions());
  std::transform(paths.begin(), paths.end(), vis.begin(), [](Step s) -> std::string {
    if (s.dist < 10) return std::to_string(s.dist);
    if (s.dist == INT_MAX) return "-";
    else return "9";
  });
  return std::cout << vis;
}

//------------------------------------------------------------------------------
// Shortest paths with A-star algorithm
//------------------------------------------------------------------------------

template <typename Edge>
Grid<Step> astar_shortest_path(CoordRange dims, Edge const& edges, Coord from, Coord to, int min_distance_cost=1) {
  Grid<Step> out(dims, Step{INT_MAX, INVALID});
  struct Item {
    Coord c;
    int dist;
    inline bool operator < (Item const& b) const {
      return dist > b.dist;
    }
  };
  std::priority_queue<Item> queue;
  auto bound = [=](Coord a) { return min_distance_cost * (abs(a.x-to.x) + abs(a.y-to.y));};
  out[from].dist = 0;
  queue.push(Item{from, 0+bound(from)});
  while (!queue.empty()) {
    auto item = queue.top();
    queue.pop();
    if (item.c == to) break;
    for (auto d : dirs) {
      Coord b = item.c + d;
      if (!dims.valid(b)) continue;
      auto edge = edges(item.c,b,d);
      if (edge == INT_MAX) continue;
      int new_dist = out[item.c].dist + edge;
      if (new_dist < out[b].dist) {
        out[b].dist = new_dist;
        out[b].from = item.c;
        queue.push(Item{b, new_dist+bound(b)});
      }
    }
  }
  return out;
}

//------------------------------------------------------------------------------
// Flood fill
//------------------------------------------------------------------------------

#include <assert.h>

template <typename CanMove>
void flood_fill_go(Grid<bool>& out, CanMove const& can_move, Coord a) {
  if (!out.valid(a)) {
    std::cout << "ABOUT TO BREAK THINGS " << a << std::endl;
  }
  int y = a.y;
  // find left/rightmost points
  int min_x = a.x;
  while (min_x > 0) {
    if (can_move(Coord{min_x,y},Coord{min_x-1,y},Dir::left) && !out[Coord{min_x-1,y}]) {
      min_x--;
    } else {
      break;
    }
  }
  int max_x = a.x;
  while (max_x + 1 < out.w) {
    if (can_move(Coord{max_x,y},Coord{max_x+1,y},Dir::right) && !out[Coord{max_x+1,y}]) {
      max_x++;
    } else {
      break;
    }
  }
  // mark
  std::fill(&out[Coord{min_x,y}], &out[Coord{max_x,y}]+1, true);
  // up/down
  for (int x=min_x; x<=max_x; ++x) {
    if (y > 0 && can_move(Coord{x,y},Coord{x,y-1},Dir::up) && !out[Coord{x,y-1}]) {
      flood_fill_go(out, can_move, Coord{x,y-1});
    }
    if (y+1 < out.h && can_move(Coord{x,y},Coord{x,y+1},Dir::down) && !out[Coord{x,y+1}]) {
      flood_fill_go(out, can_move, Coord{x,y+1});
    }
  }
}

template <typename CanMove>
Grid<bool> flood_fill(CoordRange dims, CanMove const& can_move, Coord from) {
  Grid<bool> out(dims, false);
  flood_fill_go(out, can_move, from);
  return out;
}

// flood fill starting at a neihbor of from
Grid<bool> flood_fill_from_neighbor(Grid<bool> const& grid, Coord from) {
  Grid<bool> out(grid.dimensions(), false);
  for (Dir d : dirs) {
    if (grid.is_clear(from + d)) {
      flood_fill_go(out, [&](Coord, Coord to, Dir){ return !grid[to]; }, from + d);
      break;
    }
  }
  return out;
}
// flood fill from a single position
Grid<bool> flood_fill(Grid<bool> const& grid) {
  Grid<bool> out(grid.dimensions(), false);
  for (Coord c : grid.coords()) {
    if (!grid[c]) {
      flood_fill_go(out, [&](Coord, Coord to, Dir){ return !grid[to]; }, c);
      break;
    }
  }
  return out;
}

