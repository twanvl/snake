#include "util.hpp"
#include <climits>

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

