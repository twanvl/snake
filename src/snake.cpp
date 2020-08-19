#include "util.hpp"
#include "game.hpp"

#include "zig_zag_agent.hpp"
#include "cell_tree_agent.hpp"

#include <unistd.h>
#include <cmath>

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

std::ostream& operator << (std::ostream& out, Stats const& stats) {
  out << "turns: mean " << mean(stats.turns);
  out << ", stddev " << stddev(stats.turns);
  out << ", quantiles " << quantiles(stats.turns);
  if (mean(stats.wins) < 1) {
    out << "  LOST: " << (1-mean(stats.wins))*100 << "%";
  }
  return out;
}

template <typename AgentGen>
Stats play_multiple(AgentGen make_agent, int n = 100) {
  Stats stats;
  for (int i = 0; i < n; ++i) {
    Game game;
    play(game, make_agent());
    stats.turns.push_back(game.turn);
    stats.wins.push_back(game.win());
    if (!game.win()) std::cout << game;
    std::cout << (i+1) << "/" << n << "  " << stats << "  \r";
  }
  std::cout << "\033[K";
  return stats;
}


//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

int main() {
  Game game;
  //auto agent = []{return FixedAgent{};};
  auto agent = []{return CutAgent{};};
  //auto agent = []{return CellTreeAgent{};};
  
  if (0) {
    std::cout << "\033[H\033[J";
    play(game, agent(), false);
    std::cout << game;
  }
  
  if (1) {
    auto stats = play_multiple(agent);
    std::cout << stats << std::endl;
  }
}

