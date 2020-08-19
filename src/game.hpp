#pragma once

#include "util.hpp"
#include "random.hpp"

//------------------------------------------------------------------------------
// Game state
//------------------------------------------------------------------------------

class Game {
private:
  RNG rng;
  
  Coord random_free_coord();

public:
  Grid<bool> grid;
  RingBuffer<Coord> snake; // coordinates that contain the snake, front = head of snake, back = tail of snake
  Coord apple_pos;
  int turn = 0;
  enum class State {
    playing, loss, win
  } state = State::playing;
  
  inline bool win()  const { return state == State::win; }
  inline bool loss() const { return state == State::loss; }
  inline bool done() const { return state != State::playing; }
  
  inline Coord snake_pos() const { return snake.front(); }
  
  Game(RNG const& rng = global_rng.next_rng());
  Game(Game const&) = delete;
  void move(Dir dir);
};

std::ostream& operator << (std::ostream& out, Game const& game);

//------------------------------------------------------------------------------
// Game
//------------------------------------------------------------------------------

#include <algorithm>

Game::Game(RNG const& base_rng)
  : rng(base_rng)
  , grid(false)
  , snake(w*h+1)
{
  Coord start = Coord{rng.random(w), rng.random(h)};
  snake.push_front(start);
  grid[start] = true;
  apple_pos = random_free_coord();
}

Coord Game::random_free_coord() {
  int n = w*h - snake.size();
  int pos = rng.random(n);
  for (auto c : coords) {
    if (!grid[c]) {
      if (pos == 0) return c;
      else pos--;
    }
  }
  throw "no free coord";
}

void Game::move(Dir dir) {
  if (state != State::playing) return;
  turn++;
  Coord next = snake.front() + dir;
  if (!valid(next) || grid[next]) {
    state = State::loss;
    return;
  }
  snake.push_front(next);
  grid[next] = true;
  if (next == apple_pos) {
    // grow
    if (snake.size() == grid.size()) {
      state = State::win;
    } else {
      apple_pos = random_free_coord();
    }
    //std::cout << *this;
  } else {
    // remove tail
    grid[snake.back()] = false;
    snake.pop_back();
  }
}


std::ostream& operator << (std::ostream& out, Grid<const char*> const& grid) {
  for (int y=0; y<h; ++y) {
    for (int x=0; x<w; ++x) {
      out << grid[{x,y}];
    }
    out << std::endl;
  }
  return out;
}

Grid<const char*> box_draw_grid(Game const& game) {
  Grid<const char*> grid("·");
  if (true) {
    #define GRAY(x) "\033[30;1m" x "\033[0m"
    for (int y=0; y<h; y+=2) {
      for (int x=0; x<w; x+=2) {
        grid[{x,  y  }] = GRAY("╭");
        grid[{x+1,y  }] = GRAY("╮");
        grid[{x,  y+1}] = GRAY("╰");
        grid[{x+1,y+1}] = GRAY("╯");
      }
    }
  }
  grid[game.apple_pos] = "\033[31;1m●\033[0m";
  for (int i=0; i < game.snake.size() ; ++i) {
    Coord c = game.snake[i];
    //#define SNAKE_COLOR(x) "\033[32;1m" x "\033[0m"
    #define SNAKE_COLOR(x) x
    grid[c] = SNAKE_COLOR("#");
    if (i==0) {
      grid[c] = SNAKE_COLOR("■");
    } else if (i == game.snake.size()-1) {
      Dir d = game.snake[i-1] - c;
      if (d == Dir::up)    grid[c] = SNAKE_COLOR("╵");
      if (d == Dir::down)  grid[c] = SNAKE_COLOR("╷");
      if (d == Dir::left)  grid[c] = SNAKE_COLOR("╴");
      if (d == Dir::right) grid[c] = SNAKE_COLOR("╶");
    } else {
      Dir d = c - game.snake[i+1];
      Dir e = game.snake[i-1] - c;
      if (d == Dir::up) {
        if (e == Dir::up)    grid[c] = SNAKE_COLOR("│");
        if (e == Dir::down)  grid[c] = SNAKE_COLOR("│");
        if (e == Dir::left)  grid[c] = SNAKE_COLOR("┐");
        if (e == Dir::right) grid[c] = SNAKE_COLOR("┌");
      } else if (d == Dir::down) {
        if (e == Dir::up)    grid[c] = SNAKE_COLOR("│");
        if (e == Dir::down)  grid[c] = SNAKE_COLOR("│");
        if (e == Dir::left)  grid[c] = SNAKE_COLOR("┘");
        if (e == Dir::right) grid[c] = SNAKE_COLOR("└");
      } else if (d == Dir::left) {
        if (e == Dir::up)    grid[c] = SNAKE_COLOR("└");
        if (e == Dir::down)  grid[c] = SNAKE_COLOR("┌");
        if (e == Dir::left)  grid[c] = SNAKE_COLOR("─");
        if (e == Dir::right) grid[c] = SNAKE_COLOR("─");
      } else if (d == Dir::right) {
        if (e == Dir::up)    grid[c] = SNAKE_COLOR("┘");
        if (e == Dir::down)  grid[c] = SNAKE_COLOR("┐");
        if (e == Dir::left)  grid[c] = SNAKE_COLOR("─");
        if (e == Dir::right) grid[c] = SNAKE_COLOR("─");
      }
    }
  }
  return grid;
}

std::ostream& operator << (std::ostream& out, Game const& game) {
  out << "turn " << game.turn << ", size " << game.snake.size() << (game.win() ? " WIN!" : game.loss() ? " LOSS" : "") << std::endl;
  if (true) {
    out << box_draw_grid(game);
  } else {
    for (int y=0; y<h; ++y) {
      for (int x=0; x<w; ++x) {
        Coord c = {x,y};
        if (game.grid[c]) {
          if (c == game.snake.front()) {
            out << '@';
          } else {
            out << '#';
          }
        } else if (c == game.apple_pos) {
          out << 'a';
        } else {
          out << '.';
        }
      }
      out << std::endl;
    }
  }
  return out;
}

//------------------------------------------------------------------------------
// Agents
//------------------------------------------------------------------------------

struct Agent {
  virtual ~Agent() {}
  virtual Dir operator () (Game const& game) = 0;
};

