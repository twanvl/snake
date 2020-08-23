#pragma once

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "random.hpp"

//------------------------------------------------------------------------------
// Directions
//------------------------------------------------------------------------------

enum class Dir {
  up, down, left, right
};

const Dir dirs[] = {Dir::up, Dir::down, Dir::left, Dir::right};

inline std::ostream& operator << (std::ostream& out, Dir dir) {
  switch (dir) {
    case Dir::up:    return out << "u";
    case Dir::down:  return out << "d";
    case Dir::left:  return out << "l";
    case Dir::right:
    default:         return out << "r";
  }
}

inline Dir operator - (Dir dir) {
  switch (dir) {
    case Dir::up:    return Dir::down;
    case Dir::down:  return Dir::up;
    case Dir::left:  return Dir::right;
    case Dir::right:
    default:         return Dir::left;
  }
}

inline Dir rotate_clockwise(Dir dir) {
  switch (dir) {
    case Dir::up:    return Dir::right;
    case Dir::down:  return Dir::left;
    case Dir::left:  return Dir::up;
    case Dir::right:
    default:         return Dir::down;
  }
}

inline Dir rotate_counter_clockwise(Dir dir) {
  switch (dir) {
    case Dir::up:    return Dir::left;
    case Dir::down:  return Dir::right;
    case Dir::left:  return Dir::down;
    case Dir::right:
    default:         return Dir::up;
  }
}

//------------------------------------------------------------------------------
// Coordinates
//------------------------------------------------------------------------------

struct Coord {
  int x,y;
};

inline bool operator == (Coord a, Coord b) {
  return a.x == b.x && a.y == b.y;
}
inline bool operator != (Coord a, Coord b) {
  return !(a == b);
}

inline Coord operator + (Coord a, Dir dir) {
  switch (dir) {
    case Dir::up:    return Coord{a.x, a.y-1};
    case Dir::down:  return Coord{a.x, a.y+1};
    case Dir::left:  return Coord{a.x-1, a.y};
    case Dir::right:
    default:         return Coord{a.x+1, a.y};
  }
}

inline std::ostream& operator << (std::ostream& out, Coord a) {
  return out << "(" << a.x << "," << a.y << ")";
}

inline Dir operator - (Coord a, Coord b) {
  if (a.x == b.x) {
    if (a.y == b.y-1) return Dir::up;
    if (a.y == b.y+1) return Dir::down;
  } else if (a.y == b.y) {
    if (a.x == b.x-1) return Dir::left;
    if (a.x == b.x+1) return Dir::right;
  }
  std::cout << "Not neighbors: " << a << " and " << b << std::endl;
  throw std::logic_error("Not a dir");
}

inline int manhattan_distance(Coord a, Coord b) {
  return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

inline bool is_neighbor(Coord a, Coord b) {
  return manhattan_distance(a,b) == 1;
}

const Coord INVALID = {-1,-1};
const Coord NOT_VISITED = {-1,-1};
const Coord ROOT = {-2,-2};

//------------------------------------------------------------------------------
// Coordinate Grid
//------------------------------------------------------------------------------

struct CoordRange {
  int w, h;
  
  // Iterate over all coordinates
  struct iterator {
  private:
    Coord coord;
    int w;
  public:
    inline iterator(Coord coord, int w) : coord(coord), w(w) {}
    inline Coord operator * () const {
      return coord;
    }
    inline void operator ++ () {
      coord.x++;
      if (coord.x == w) {
        coord.x = 0;
        coord.y++;
      }
    }
    inline bool operator == (iterator const& that) const {
      return coord == that.coord;
    }
    inline bool operator != (iterator const& that) const {
      return coord != that.coord;
    }
  };
  
  inline iterator begin() const {
    return iterator{{0,0},w};
  }
  inline iterator end() const {
    return iterator{{0,h},w};
  }
  inline bool valid(Coord a) const {
    return a.x >= 0 && a.x < w && a.y >= 0 && a.y < h;
  }
  inline int size() const {
    return w * h;
  }
  inline Coord random(RNG& rng) const {
    return Coord{rng.random(w), rng.random(h)};
  }
};

//------------------------------------------------------------------------------
// Grid
//------------------------------------------------------------------------------

template <typename T> struct ClearValue{};
template <> struct ClearValue<bool> {
  inline static bool value() { return false; }
};


// A grid data structure, storing values of type T
// The grid has size w*h
template <typename T>
class Grid {
private:
  T* data;
public:
  const int w,h;
  Grid(CoordRange range, T const& init = T())
    : Grid(range.w, range.h, init)
  {}
  Grid(int w, int h, T const& init = T())
    : data(new T[w*h]), w(w), h(h)
  {
    std::fill(begin(), end(), init);
  }
  Grid(Grid const& that)
    : data(new T[that.w*that.h]), w(that.w), h(that.h)
  {
    std::copy(that.begin(), that.end(), data);
  }
  Grid(Grid&& that)
    : data(that.data), w(that.w), h(that.h)
  {
    that.data = nullptr;
  }
  ~Grid() {
    delete[] data;
  }
  inline int size() const {
    return w*h;
  }
  inline T& operator [] (Coord a) {
    return data[a.x + w*a.y];
  }
  inline T const& operator [] (Coord a) const {
    return data[a.x + w*a.y];
  }
  
  inline CoordRange coords() const {
    return {w,h};
  }
  inline CoordRange dimensions() const {
    return coords();
  }
  inline bool valid(Coord a) const {
    return coords().valid(a);
  }
  
  // is the grid clear, i.e. inside the range and equal to ClearValue<T>::value()
  inline bool is_clear(Coord a) const {
    return valid(a) && (*this)[a] == ClearValue<T>::value();
  }

  using iterator = T*;
  inline iterator begin() { return data; }
  inline iterator end() { return &data[w*h]; }
  
  using const_iterator = const T*;
  inline const_iterator begin() const { return data; }
  inline const_iterator end() const { return &data[w*h]; }
};

//------------------------------------------------------------------------------
// Ring Buffer
//------------------------------------------------------------------------------

// A ring buffer: elements can be added/removed to the front and back in constant time,
// up to a maximum capacity.
template <typename T>
class RingBuffer {
private:
  T* data;
  int capacity_;
  int begin_, end_;
public:
  RingBuffer(int capacity_)
    : data(new T[capacity_])
    , capacity_(capacity_)
    , begin_(0)
    , end_(0)
  {}
  RingBuffer(RingBuffer const& that)
    : data(new T[that.capacity_])
    , capacity_(that.capacity_)
    , begin_(that.begin_)
    , end_(that.end_)
  {
    std::copy(that.data, that.data+capacity_, data);
  }
  ~RingBuffer() {
    delete [] data;
  }

  inline int capacity() const {
    return capacity_;
  }
  inline int size() const {
    return end_ - begin_ + (end_ < begin_) * capacity();
  }
  bool empty() const {
    return begin_ == end_;
  }

  T& front() {
    return data[begin_];
  }
  T const& front() const {
    return data[begin_];
  }
  void push_front(T const& x) {
    begin_--;
    if (begin_ < 0) begin_ = capacity() - 1;
    data[begin_] = x;
  }
  void pop_front() {
    begin_++;
    if (begin_ >= capacity()) begin_ = 0;
  }

  T& back() {
    return end_ == 0 ? data[capacity() - 1] : data[end_ - 1];
  }
  T const& back() const {
    return end_ == 0 ? data[capacity() - 1] : data[end_ - 1];
  }
  void push_back(T const& x) {
    data[end_] = x;
    end_++;
    if (end_ >= capacity()) end_ = 0;
  }
  void pop_back() {
    end_--;
    if (end_ < 0) end_ = capacity() - 1;
  }
  
  T const& operator[] (int i) const {
    return data[(begin_+i) % capacity_];
  }
  
  template <typename Ptr, typename Ref>
  struct iterator_base {
  private:
    Ptr begin, end, data;
  public:
    inline iterator_base(Ptr begin, Ptr end, Ptr data) : begin(begin), end(end), data(data) {}
    inline iterator_base& operator ++ () {
      ++data;
      if (data == end) data = begin;
      return *this;
    }
    inline iterator_base& operator -- () {
      if (data == begin) data = end;
      --data;
      return *this;
    }
    inline Ref operator * () const {
      return *data;
    }
    inline bool operator == (iterator_base const& that) const {
      return data == that.data;
    }
    inline bool operator != (iterator_base const& that) const {
      return data != that.data;
    }
  };
  using iterator = iterator_base<T*,T&>;
  using const_iterator = iterator_base<const T*,const T&>;
  iterator begin() { return iterator(data, data+capacity_, data+begin_); }
  iterator end()   { return iterator(data, data+capacity_, data+end_); }
  const_iterator begin() const { return const_iterator(data, data+capacity_, data+begin_); }
  const_iterator end()   const { return const_iterator(data, data+capacity_, data+end_); }
};

//------------------------------------------------------------------------------
// Statistics utilities
//------------------------------------------------------------------------------

template <typename T>
double mean(std::vector<T> const& xs) {
  // work around emscripten bug (missing accumulate function)
  //return std::accumulate(xs.begin(), xs.end(), 0.) / xs.size();
  double sum = 0.;
  for(auto x : xs) sum += x;
  return sum / std::max(1, (int)xs.size());
}

template <typename T>
double variance(std::vector<T> const& xs) {
  double m = mean(xs);
  double sum = 0.;
  for(auto x : xs) sum += (x-m)*(x-m);
  return sum / std::max(1, (int)xs.size()-1);
}

template <typename T>
double stddev(std::vector<T> const& xs) {
  return std::sqrt(variance(xs));
}

double lerp(double a, double b, double t) {
  return t*a + (1-t)*b;
}

template <typename T>
std::vector<double> quantiles(std::vector<T> const& xs) {
  std::vector<T> sorted = xs;
  std::sort(sorted.begin(), sorted.end());
  std::vector<double> quantiles;
  for (size_t i=0 ; i<5 ; ++i) {
    size_t j = i * (xs.size() - 1);
    if (j % 4 == 0) {
      quantiles.push_back(sorted[j/4]);
    } else {
      quantiles.push_back(lerp(sorted[j/4], sorted[(j+3)/4], (j%4)*0.25));
    }
  }
  return quantiles;
}

std::ostream& operator << (std::ostream& out, std::vector<double> xs) {
  out << "[";
  bool first = true;
  for (auto x : xs) {
    if (!first) {
      out << ", ";
    }
    first = false;
    out << x;
  }
  return out << "]";
}

