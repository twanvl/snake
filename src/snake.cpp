#include "util.hpp"
#include "game.hpp"

#include "zig_zag_agent.hpp"
#include "cell_tree_agent.hpp"
#include "hamiltonian_cycle.hpp"

#include <unistd.h>
#include <cmath>
#include <fstream>

//------------------------------------------------------------------------------
// Config
//------------------------------------------------------------------------------

//constexpr CoordRange board_size = {30,30};
constexpr CoordRange board_size = {20,20};

static_assert(board_size.w % 2 == 0 && board_size.h % 2 == 0);

//------------------------------------------------------------------------------
// Logging games
//------------------------------------------------------------------------------

struct Log {
  std::vector<Coord> snake_pos;
  std::vector<int>   snake_size;
  std::vector<Coord> apple_pos;

  void log(GameBase const& game) {
    snake_pos.push_back(game.snake_pos());
    snake_size.push_back(game.snake.size());
    apple_pos.push_back(game.apple_pos);
  }
};

class LoggedGame : public Game {
public:
  Log log;
  
  LoggedGame(CoordRange dims) : Game(dims) {
    log.log(*this);
  }
  void move(Dir d) {
    Game::move(d);
    log.log(*this);
  }
};

//------------------------------------------------------------------------------
// Json output
//------------------------------------------------------------------------------

void write_json(std::ostream& out, int x) {
  out << x;
}
void write_json(std::ostream& out, Coord c) {
  out << "[" << c.x << "," << c.y << "]";
}
void write_json(std::ostream& out, CoordRange c) {
  out << "[" << c.w << "," << c.h << "]";
}

template <typename T>
void write_json(std::ostream& out, std::vector<T> const& xs) {
  out << "[";
  bool first = true;
  for (auto x : xs) {
    if (!first) {
      out << ", ";
    }
    first = false;
    write_json(out, x);
  }
  out << "]";
}

void write_json(std::ostream& out, LoggedGame const& game) {
  out << "game = ";
  out << "{" << std::endl;
  out << "  \"size\": "; write_json(out, game.dimensions()); out << "," << std::endl;
  out << "  \"snake_pos\": "; write_json(out, game.log.snake_pos); out << "," << std::endl;
  out << "  \"snake_size\": "; write_json(out, game.log.snake_size); out << "," << std::endl;
  out << "  \"apple_pos\": "; write_json(out, game.log.apple_pos); out << "," << std::endl;
  out << "}" << std::endl;
  out << ";";
}

void write_json(std::string const& filename, LoggedGame const& game) {
  std::ofstream out(filename);
  write_json(out, game);
}

//------------------------------------------------------------------------------
// Playing full games
//------------------------------------------------------------------------------

enum class Visualize {
  no, eat, all
};

template <typename Game, typename Agent>
void play(Game& game, Agent agent, Visualize visualize = Visualize::no) {
  if (visualize == Visualize::all) {
    std::cout << "\033[H\033[J";
  }
  while (!game.done()) {
    if (visualize == Visualize::all) {
      std::cout << "\033[H";
      std::cout << game;
      usleep(game.turn < 100 ? 20000 : game.turn < 1000 ? 2000 : 200);
    }
    int snake_size = game.snake.size();
    game.move(agent(game));
    if (visualize == Visualize::eat && game.snake.size() > snake_size) {
      std::cout << game;
    }
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
Stats play_multiple(AgentGen make_agent, CoordRange dims = board_size, int n = 100) {
  Stats stats;
  for (int i = 0; i < n; ++i) {
    Game game(dims);
    play(game, make_agent());
    stats.turns.push_back(game.turn);
    stats.wins.push_back(game.win());
    if (!game.win()) std::cout << game;
    std::cout << (i+1) << "/" << n << "  " << stats << "  \r" << std::flush;
  }
  std::cout << "\033[K";
  return stats;
}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

int main(int argc, const char** argv) {
  // Parse command line args
  /*
  std::string mode = argc >= ? argv[1] : "";
  if (mode == "")
  for (int i=1; i<argc; ++i) {
    std::string arg = argv[i];
    if (arg
  }
  */
  //
  //auto agent = []{return FixedAgent{};};
  //auto agent = []{return FixedCycleAgent{random_hamiltonian_cycle(global_rng)};};
  //auto agent = []{return CutAgent{};};
  //auto agent = []{return CellTreeAgent{};};
  //auto agent = []{return PerturbedHamiltonianCycle(make_path(board_size));};
  //auto agent = []{return PerturbedHamiltonianCycle(random_hamiltonian_cycle(global_rng));};
  auto agent = []{return DynamicHamiltonianCycleRepair{make_path(board_size)};};
  
  if (1) {
    LoggedGame game(board_size);
    play(game, agent(), Visualize::no);
    std::cout << game;
    
    write_json("game.json", game);
  }
  
  if (1) {
    auto stats = play_multiple(agent);
    std::cout << stats << std::endl;
  }
}

