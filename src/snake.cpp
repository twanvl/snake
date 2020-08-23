#include "util.hpp"
#include "game.hpp"

#include "zig_zag_agent.hpp"
#include "cell_tree_agent.hpp"
#include "hamiltonian_cycle.hpp"

#include <unistd.h>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <memory>
#include <functional>

//------------------------------------------------------------------------------
// Logging games
//------------------------------------------------------------------------------

struct Log {
  std::vector<Coord> snake_pos;
  std::vector<int>   snake_size;
  std::vector<Coord> apple_pos;
  std::vector<int>   eat_turns;

  void log(Game const& game, Game::Event event) {
    snake_pos.push_back(game.snake_pos());
    snake_size.push_back(game.snake.size());
    apple_pos.push_back(game.apple_pos);
    if (event == Game::Event::eat) {
      eat_turns.push_back(game.turn);
    }
  }
};

class LoggedGame : public Game {
public:
  Log log;
  
  LoggedGame(CoordRange dims) : Game(dims) {
    log.log(*this, Game::Event::none);
  }
  Event move(Dir d) {
    Event e = Game::move(d);
    log.log(*this, e);
    return e;
  }
};

//------------------------------------------------------------------------------
// Stats of multiple games
//------------------------------------------------------------------------------

struct Stats {
  std::vector<int> turns;
  std::vector<bool> wins;
  
  void add(Game const& game);
};

void Stats::add(Game const& game) {
  wins.push_back(game.win());
  if (game.win()) {
    turns.push_back(game.turn);
  }
}

std::ostream& operator << (std::ostream& out, Stats const& stats) {
  out << "turns: mean " << mean(stats.turns);
  out << ", stddev " << stddev(stats.turns);
  out << ", quantiles " << quantiles(stats.turns);
  if (mean(stats.wins) < 1) {
    out << "  LOST: " << (1-mean(stats.wins))*100 << "%";
  }
  return out;
}

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------

enum class Trace {
  no, eat, all
};
struct Config {
  int num_rounds = 100;
  CoordRange board_size = {30,30};
  Trace trace = Trace::no;
  bool quiet = false;
  std::string json_file;
  RNG rng = global_rng;
  
  void parse_optional_args(int argc, const char** argv);
};

//------------------------------------------------------------------------------
// Agents
//------------------------------------------------------------------------------

struct AgentFactory {
  std::string name;
  std::string description;
  std::function<std::unique_ptr<Agent>(Config&)> make;
};
AgentFactory agents[] = {
  {"zig-zag", "Follows a fixed zig-zag cycle", [](Config&) {
    return std::make_unique<FixedZigZagAgent>();
  }},
  {"fixed", "Follows a fixed but random cycle", [](Config& config) {
    return std::make_unique<FixedCycleAgent>(random_hamiltonian_cycle(config.board_size, config.rng));
  }},
  {"zig-zag-cut", "Follows a zig-zag cycle, but can take shortcuts", [](Config& config) {
    return std::make_unique<CutAgent>(config.board_size);
  }},
  {"cell", "Limit movement to a tree of 2x2 cells", [](Config&) {
    return std::make_unique<CellTreeAgent>();
  }},
  {"cell1", "Cell tree agent with limited lookahead", [](Config&) {
    auto agent = std::make_unique<CellTreeAgent>();
    agent->lookahead = Lookahead::one;
    return agent;
  }},
  {"cell-fixed", "Cell agent that doesn't recalculate paths", [](Config&) {
    auto agent = std::make_unique<CellTreeAgent>();
    agent->recalculate_path = false;
    return agent;
  }},
  {"phc", "Perturbed Hamiltonian cycle (zig-zag cycle)", [](Config& config) {
    auto agent = std::make_unique<PerturbedHamiltonianCycle>(make_path(config.board_size));
    return agent;
  }},
  {"dhcr", "Dynamic Hamiltonian Cycle Repair", [](Config& config) {
    auto agent = std::make_unique<DynamicHamiltonianCycleRepair>(make_path(config.board_size));
    return agent;
  }},
};

void list_agents(std::ostream& out = std::cout) {
  out << "Available agents:" << std::endl;
  for (auto const& a : agents) {
    out << "  " << std::left << std::setw(20) << a.name;
    out << a.description << std::endl;
  }
}

AgentFactory const& find_agent(std::string const& name) {
  for (auto const& a : agents) {
    if (a.name == name) return a;
  }
  throw std::invalid_argument("Unknown agent: " + name + "\nUse `list` command to list available agents.");
}

//------------------------------------------------------------------------------
// Argument handling
//------------------------------------------------------------------------------

void print_help(const char* name, std::ostream& out = std::cout) {
  Config def;
  using namespace std;
  out << "Usage: " << name << " <mode> <args>" << endl;
  out << endl;
  out << "These modes are available:" << endl;
  out << "  help                Show this message." << endl;
  out << "  list                List available agents." << endl;
  out << "  all                 Play all agents against each other, output csv summary." << endl;
  out << "  <agent>             Play with the given agent." << endl;
  out << endl;
  out << "Optional arguments:" << endl;
  out << "  -n, --n <rounds>    Run the given number of rounds (default: " << def.num_rounds << ")." << endl;
  out << "  -s, --size <size>   Size of the (square) board (default: " << def.board_size.w << ")." << endl;
  out << "      --seed <n>      Random seed." << endl;
  out << "  -T, --trace-all     Print the game state after each move." << endl;
  out << "  -t, --trace         Print the game state each time the snake eats an apple." << endl;
  out << "      --no-color      Don't use ANSI color codes in trace output" << endl;
  out << "  -q, --quiet         Don't print extra output." << endl;
  out << "  -j, --json <file>   Write log of one run a json file." << endl;
  out << endl;
  list_agents(out);
}

void Config::parse_optional_args(int argc, const char** argv) {
  for (int i=0; i<argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-n" || arg == "--n") {
      if (i+1 >= argc) throw std::invalid_argument("Missing argument to " + arg);
      num_rounds = std::stoi(argv[++i]);
    } else if (arg == "-s" || arg == "--size") {
      if (i+1 >= argc) throw std::invalid_argument("Missing argument to " + arg);
      int size = std::stoi(argv[++i]);
      board_size = {size,size};
    } else if (arg == "-w" || arg == "--width") {
      if (i+1 >= argc) throw std::invalid_argument("Missing argument to " + arg);
      board_size.w = std::stoi(argv[++i]);
    } else if (arg == "-h" || arg == "--height") {
      if (i+1 >= argc) throw std::invalid_argument("Missing argument to " + arg);
      board_size.h = std::stoi(argv[++i]);
    } else if (arg == "--seed") {
      if (i+1 >= argc) throw std::invalid_argument("Missing argument to " + arg);
      int seed = std::stoi(argv[++i]);
      uint64_t s[] = {1234567891234567890u,9876543210987654321u+seed};
      rng = RNG(s);
    } else if (arg == "-j" || arg == "--json") {
      if (i+1 >= argc) throw std::invalid_argument("Missing argument to " + arg);
      json_file = argv[++i];
    } else if (arg == "-t" || arg == "--trace") {
      trace = Trace::eat;
    } else if (arg == "-T" || arg == "--trace-all") {
      trace = Trace::all;
    } else if (arg == "-q" || arg == "--quiet") {
      quiet = true;
    } else if (arg == "--no-color") {
      use_color = false;
    } else{
      throw std::invalid_argument("Unknown argument: " + arg);
    }
  }
}

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

void write_json(std::ostream& out, AgentFactory const& agent, LoggedGame const& game) {
  out << "game = ";
  out << "{" << std::endl;
  out << "  \"agent\": \"" << agent.name << "\"," << std::endl;
  out << "  \"agent_description\": \"" << agent.description << "\"," << std::endl;
  out << "  \"size\": "; write_json(out, game.dimensions()); out << "," << std::endl;
  out << "  \"snake_pos\": "; write_json(out, game.log.snake_pos); out << "," << std::endl;
  out << "  \"snake_size\": "; write_json(out, game.log.snake_size); out << "," << std::endl;
  out << "  \"apple_pos\": "; write_json(out, game.log.apple_pos); out << "," << std::endl;
  out << "  \"eat_turns\": "; write_json(out, game.log.eat_turns); out << "," << std::endl;
  out << "}" << std::endl;
  out << ";";
}

void write_json(std::string const& filename, AgentFactory const& agent, LoggedGame const& game) {
  std::ofstream out(filename);
  write_json(out, agent, game);
}

//------------------------------------------------------------------------------
// Playing full games
//------------------------------------------------------------------------------

enum class Visualize {
  no, eat, all
};

template <typename Game>
void play(Game& game, Agent& agent, Config const& config) {
  while (!game.done()) {
    if (config.trace == Trace::all) std::cout << game;
    auto event = game.move(agent(game));
    if (event == Game::Event::eat && config.trace == Trace::eat) std::cout << game;
  }
  if (config.trace == Trace::all) std::cout << game;
}


template <typename AgentGen>
Stats play_multiple(AgentGen make_agent, Config& config) {
  Stats stats;
  for (int i = 0; i < config.num_rounds; ++i) {
    Game game(config.board_size);
    auto agent = make_agent(config);
    play(game, *agent, config);
    stats.add(game);
    if (!config.quiet) {
      if (!game.win()) std::cout << game;
      std::cout << (i+1) << "/" << config.num_rounds << "  " << stats << "\033[1K\r" << std::flush;
    }
  }
  if (!config.quiet) std::cout << "\033[K\r";
  return stats;
}

void play_all_agents(Config& config, std::ostream& out = std::cout) {
  using namespace std;
  out << "agent, mean, stddev, min, q.25, median, q.75, max, lost" << endl;
  for (auto const& agent : agents) {
    out << left << setw(15) << agent.name << ", " << flush;
    auto stats = play_multiple(agent.make, config);
    out << right << fixed << setprecision(1) << setw(10);
    out << setw(8) << mean(stats.turns) << ", ";
    out << setw(8) << stddev(stats.turns) << ", ";
    out << setprecision(0);
    for (auto q : quantiles(stats.turns)) {
      out << setw(8) << q << ", ";
    }
    out << setprecision(1);
    out << setw(8) << ((1-mean(stats.wins))*100) << "%" << endl;
  }
}
      
//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

int main(int argc, const char** argv) {
  // Parse command line args
  std::string mode = argc >= 2 ? argv[1] : "help";
  
  try {
    if (mode == "help" || mode == "--help" || mode == "-h") {
      print_help(argv[0]);
    } else if (mode == "list") {
      list_agents();
    } else if (mode == "all") {
      Config config;
      config.quiet = true;
      config.parse_optional_args(argc-2, argv+2);
      
      play_all_agents(config);
    } else {
      auto agent = find_agent(mode);
      Config config;
      config.parse_optional_args(argc-2, argv+2);
      if (config.trace != Trace::no || !config.json_file.empty()) {
        LoggedGame game(config.board_size);
        auto a = agent.make(config);
        play(game, *a, config);
        if (!config.json_file.empty()) {
          write_json(config.json_file, agent, game);
        }
      } else {
        auto stats = play_multiple(agent.make, config);
        std::cout << stats << std::endl;
      }
    }
  } catch (std::exception const& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
  
  /*
  if (mode == "")
  for (int i=1; i<argc; ++i) {
    std::string arg = argv[i];
    if (arg
  }
  Usage: snake <MODE> [options]
  MODE can be:
   * all: play all agents against each other
   * list: list the agents
   * help: show help
   * <AGENT>: play games with agent
   * [play] <AGENT> [<N>] [<SIZE>]
   * step <AGENT>
   * json <AGENT> <FILE>
  Options:
   -n N:       number of games (default: 100)
   -s SIZE:    board size (default: 30)
   -seed SEED: random seed (default = timestamp)
  
  if (mode == "help") {
    print_help(argv[0]);
  } else if (mode == "list") {
    list_agents();
  } else if (mode == "trace" && argc > 2) {
  } else if (mode == "play") {
    play(argv+2, );
  }
  */
  /*
  //
  //auto agent = []{return FixedAgent{};};
  //auto agent = []{return FixedCycleAgent{random_hamiltonian_cycle(global_rng)};};
  //auto agent = []{return CutAgent{};};
  //auto agent = []{return CellTreeAgent{};};
  //auto agent = []{return PerturbedHamiltonianCycle(make_path(board_size));};
  //auto agent = []{return PerturbedHamiltonianCycle(random_hamiltonian_cycle(board_size, global_rng));};
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
  */
}

