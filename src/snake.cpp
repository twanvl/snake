#include "util.hpp"
#include "game.hpp"

#include "zig_zag_agent.hpp"
#include "cell_tree_agent.hpp"
#include "hamiltonian_cycle.hpp"

#include <unistd.h>
#include <cmath>

//------------------------------------------------------------------------------
// Playing full games
//------------------------------------------------------------------------------

enum class Visualize {
  no, eat, all
};

template <typename Agent>
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
Stats play_multiple(AgentGen make_agent, int n = 100) {
  Stats stats;
  for (int i = 0; i < n; ++i) {
    Game game;
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
  Game game;
  //auto agent = []{return FixedAgent{};};
  //auto agent = []{return FixedCycleAgent{random_hamiltonian_cycle(global_rng)};};
  auto agent = []{return CutAgent{};};
  //auto agent = []{return CellTreeAgent{};};
  //auto agent = []{return PerturbedHamiltonianCycle(make_path(coords));};
  //auto agent = []{return PerturbedHamiltonianCycle(random_hamiltonian_cycle(global_rng));};
  
  if (1) {
    play(game, agent(), Visualize::no);
    std::cout << game;
  }
  
  if (1) {
    auto stats = play_multiple(agent);
    std::cout << stats << std::endl;
  }
}

