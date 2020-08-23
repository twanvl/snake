# Snake playing AI

A bunch of different algorithms that play the game of snake.

    ┌──┐┌────────────┐··
    ■··│└───────┐····╵··
    ···└─┐┌┐┌───┘·······
    ·┌──┐││││···········
    ·│┌┐│└┘││···········
    ·│││└──┘│···········
    ·└┘└───┐│···········
    ·······│└───┐···●···
    ·······└────┘·······
    ····················

## Usage

The program runs on the command line, and comes with help.

To build use `cmake`:

    mkdir build
    cmake -B build
    build/snake


## Algorithms

### Cell tree algorithm

The idea here is to limit the moves the snake can make. In each 2×2 cell it is always 'driving' on the right. So at any point there are only two possible moves. If the snake moves from one cell to another then the cells are connected.
If the connected cells form a tree, then the path of the snake must be a Hamiltonian cycle.

So, we enforce the following conditions
* Only make moves consistent with right-hand-driving in the 2×2 cell
* Only move to parents or unvisited nodes in the tree (this prevents cycles)
* Make sure that nodes don't become inaccessible (that would prevent a full spanning tree/Hamiltonian cycle from existing)

### Simple fixed agents
Agents that follow a hardcoded Hamiltonian Cycle.

### Perturbed Hamiltonian Cycle

See https://johnflux.com/2015/05/02/nokia-6110-part-3-algorithms/

### Dynamic Hamiltonian Cycle Repair

A re-implementation of Brian Haidet's algorithm, see
https://github.com/BrianHaidet/AlphaPhoenix/tree/master/Snake_AI_(2020a)_DHCR_with_strategy

The basic idea is to maintain a Hamiltonian cycle over the grid, and then to try to take shortcuts along the shortest path to the goal if the Hamiltonian cycle can be repaired.

This version uses less hacks. For example instead of tweaking the A* algorithm, I updated the edge costs to take the existing cycle into account.

Currently does not include "nascar" or "applefear" modes.


## Results

Number of steps to finish the game on a 30×30 grid (over 100 runs)

|agent          |mean     |stddev   |min      |q.25     |median   |q.75     |max      |lost      |
|---------------|---------|---------|---------|---------|---------|---------|---------|----------|
|zig-zag        | 202529.7|   4652.9|   193677|   198820|   202236|   205846|   215937|      0.0%|
|fixed          | 202490.4|   4418.0|   190491|   199010|   202494|   205993|   212761|      0.0%|
|zig-zag-cut    | 105039.8|   3149.7|    98162|   102595|   104694|   107240|   113850|      0.0%|
|cell           |  49178.4|   2187.1|    44217|    47628|    48966|    50592|    56266|      0.0%|
|cell1          |  57393.5|   2528.5|    52207|    55668|    57332|    59260|    63365|      0.0%|
|cell-fixed     |  52035.8|   2400.6|    47367|    50604|    51721|    53440|    58447|      0.0%|
|phc            | 103496.7|   2197.7|    98052|   101700|   103457|   105180|   109541|      0.0%|
|dhcr           |  60690.3|   2571.3|    55241|    59097|    60627|    62400|    67208|      3.0%|

Can be reproduced with `snake all`

## Visualization

There is javascript code to visualize a run in ./visualize.

TODO: Here are some visualizations of runs of the different agents


