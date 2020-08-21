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
        if (coords.valid(b) && can_move(a,b,d) && out[b].dist > dist) {
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

// Find all coords in a path from from to to, excluding the start point
// Note: returned in reverse order, that is result.back() is the first step, result.front() == to
std::vector<Coord> read_path(Grid<Step> const& paths, Coord from, Coord to) {
  std::vector<Coord> steps;
  while (to != Coord{-1,-1} && to != from) {
    steps.push_back(to);
    to = paths[to].from;
  }
  return steps;
}

std::ostream& operator << (std::ostream& out, Grid<Step> const& paths) {
  Grid<std::string> vis;
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
Grid<Step> astar_shortest_path(Edge const& edges, Coord from, Coord to, int min_distance_cost=1) {
  Grid<Step> out(Step{INT_MAX,{-1,-1}});
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
      if (!coords.valid(b)) continue;
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
  while (max_x + 1 < w) {
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
    if (y+1 < h && can_move(Coord{x,y},Coord{x,y+1},Dir::down) && !out[Coord{x,y+1}]) {
      flood_fill_go(out, can_move, Coord{x,y+1});
    }
  }
}

template <typename CanMove>
Grid<bool> flood_fill(CanMove const& can_move, Coord from) {
  Grid<bool> out(false);
  flood_fill_go(out, can_move, from);
  return out;
}

