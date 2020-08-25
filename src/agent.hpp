#pragma once

#include "game.hpp"
#include "game_util.hpp"

//------------------------------------------------------------------------------
// Logging
//------------------------------------------------------------------------------

// Log of what an agent is thinking
// At each turn, we have a map from keys to a grid or a path (or nothing)
struct AgentLog {
  struct NoEntry {};
  struct CopyEntry {};
  using LogEntry = std::variant<NoEntry, CopyEntry, std::vector<Coord>, Grid<bool>>;
  
  enum Key {
    cycle,
    plan,
    unreachable,
    MAX_KEY
  };
  
  std::vector<LogEntry> logs[(int)Key::MAX_KEY];
  
  template <typename T>
  void add(int turn, Key key, T&& value) {
    while ((int)logs[key].size() < turn) {
      logs[key].emplace_back(NoEntry{});
    }
    logs[key].emplace_back(std::forward<T>(value));
  }
  
  static std::string key_name(Key key) {
    if (key == cycle) return "cycles";
    if (key == plan) return "plans";
    if (key == unreachable) return "unreachables";
    else throw std::logic_error("key_name");
  }
};

//------------------------------------------------------------------------------
// Agents
//------------------------------------------------------------------------------

struct Agent {
  virtual ~Agent() {}
  virtual Dir operator () (Game const& game, AgentLog* log = nullptr) = 0;
};

