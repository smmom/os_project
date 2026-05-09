# Graph Simulation and Pathfinding Project

This project is a comprehensive C-based application that implements graph representation, Dijkstra's shortest path algorithm, and a graphical visualization with animation using the `raylib` library.

## Authors
* **Jinan Rasas**
* **Afnan Rabiah**
* **Mohammad Smoum**

---

## Project Overview
The project is divided into three main milestones:
1. **Milestone 1: Algorithmic Foundation** - Implementation of a weighted directed graph and Dijkstra's algorithm to find the shortest path between two nodes.
2. **Milestone 2: Graphical Representation** - Visualizing the graph nodes and edges in a window using `raylib`.
3. **Milestone 3: Path Animation** - Animating an entity moving along the calculated shortest path with speed proportional to edge weights.

---

## File Structure
* `dijkstra.c`: Implementation of the graph data structure and Dijkstra's algorithm.
* `sim.c`: Main simulation file containing the GUI logic, graph visualization, and path animation.
* `Makefile`: Build script to compile the project milestones.
* `input.txt`: Sample input file containing graph data (nodes, edges, and query).

---

## Prerequisites
To run the graphical simulation, you need to have `raylib` installed on your system.

### macOS (M1/M2/M3/M4)
Install via Homebrew:
```bash
brew install raylib
```

### Linux (Ubuntu/Debian)
```bash
sudo apt install libraylib-dev
```

---

## How to Build and Run

### 1. Compilation
Use the provided `Makefile` to compile the project:

* **To compile Milestone 1 (Dijkstra CLI):**
  ```bash
  make milestone1
  ```
* **To compile Milestone 2 & 3 (GUI Simulation):**
  ```bash
  make milestone2
  ```
* **To compile everything:**
  ```bash
  make all
  ```

### 2. Running the Programs
Both programs require an input file as a command-line argument.

* **Run Dijkstra CLI:**
  ```bash
  ./dijkstra input.txt
  ```
* **Run GUI Simulation:**
  ```bash
  ./sim input.txt
  ```

---

## Input File Format
The input file should follow this structure:
1. First line: `N M` (Number of nodes and number of edges).
2. Next `M` lines: `src dst weight` (Edge definition).
3. Last line: `start end` (Shortest path query).

Example `input.txt`:
```text
10 15

0 1 2

1 2 3

2 3 1

3 4 4

4 5 2

5 6 3

6 7 1

7 8 5

8 9 2

0 3 8

2 5 10

4 7 6

6 9 4

1 8 15

0 9 25

0 9
```

---

## Features
* **Dynamic Node Positioning:** Nodes are automatically distributed in a circular layout for clarity.
* **Interactive GUI:** Includes a "PLAY/STOP" button to control the animation.
* **Realistic Animation:** The entity waits at each node and moves at a speed inversely proportional to the edge weight.
* **Visual Feedback:** The shortest path is highlighted in blue, and arrival is confirmed with a message.
