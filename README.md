# Graph Simulation and Pathfinding Project

This project is a comprehensive C-based application that implements graph representation, Dijkstra's shortest path algorithm, graphical visualization with animation using the raylib library, multiple processes, inter-process communication, and synchronization.

## Authors
* **Jinan Rasas**
* **Afnan Rabiah**
* **Mohammad Smoum**
* **Cleopatra Kajjo**

---

## Project Overview
The project is divided into six main milestones:
1. **Milestone 1: Algorithmic Foundation** - Implementation of a weighted directed graph and Dijkstra's algorithm to find the shortest path between two nodes.
2. **Milestone 2: Graphical Representation** - Visualizing the graph nodes and edges in a window using `raylib`.
3. **Milestone 3: Path Animation** - Animating an entity moving along the calculated shortest path with speed proportional to edge weights.
4. **Milestone 4: Multiple Processes** - The parent process manages the GUI, creates child processes using fork(), and animates multiple travelers at the same time.
5. **Milestone 5: IPC** - Each child process calculates its own path and sends updates to the parent process using a pipe.
6. **Milestone 6: Synchronization** - Each graph node is protected by a semaphore so that only one traveler can stay inside a node at a time.

---

## Milestone 5 - IPC Explanation

For Milestone 5, we used a pipe as the IPC mechanism.

Each traveler is represented by a child process created using fork().
Each child process calculates its own shortest path using Dijkstra.
Whenever the child reaches a node, it sends a message to the parent through the pipe.

The parent process receives the messages, updates the GUI, and prints the log to the terminal.

We chose pipe because the communication is one-way: the children only need to report their progress to the parent.

---

## Milestone 6 - Synchronization Explanation

For Milestone 6, we used POSIX semaphores as the synchronization mechanism.

Each node in the graph has its own semaphore. The semaphores are stored in shared memory using mmap(), so all child processes can access the same locks.

Before a traveler enters a node, it tries to acquire the semaphore of that node using sem_trywait(). If the node is already occupied, the traveler sends a waiting message to the parent process and then waits using sem_wait() until the node becomes free.

When a traveler enters a node, it stays there for one full second using sleep(1). After that, it releases the node using sem_post(), allowing another waiting traveler to enter.

The GUI displays waiting travelers in a different color, so it is clear when a traveler is waiting outside a node.

We chose semaphores because they provide mutual exclusion and ensure that no more than one traveler can be inside the same node at the same time.

---


## File Structure
* `dijkstra.c`: Implementation of the graph data structure and Dijkstra's algorithm.
* `dijkstra.h`: Header file for the graph and Dijkstra functions.
* `sim.c`: Main simulation file containing the GUI logic, graph visualization, and path animation.
* `milestone4.c`: Implementation of multiple travelers using child processes.
* `milestone5.c`: Implementation of IPC using pipes.
* `milestone6.c`: Implementation of synchronization using semaphores.
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
* **To compile Milestone 4:**
  ```bash
  make milestone4
  ```
* **To compile Milestone 5:**
  ```bash
  make milestone5
  ```
* **To compile Milestone 6:**
  ```bash
  make milestone6
  ```
### 2. Cleaning Up
To remove all compiled executables and start fresh:
  ```bash
  make clean 
  ```

### 3. Running the Programs
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
## Milestone 1 Input Format
The input file should follow this structure:
1. First line: `N M` (Number of nodes and number of edges).
2. Next `M` lines: `src dst weight` (Edge definition).
3. Last line: `start end` (Shortest path query).

Example `input.txt`:
```text
6 8
0 1 4
0 2 2
1 3 5
2 1 1
2 3 8
3 4 2
4 5 3
2 5 10
0 5
```

## Milestones 4-6 Input Format
The input file should follow this structure:

First line: `N M` (number of nodes and number of edges).
Next `M` lines: `src dst weight` (edge definition).
Next line: number of travelers.
Next lines: `source destination` for each traveler.

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
2
0 9
4 8

---

## Features
* **Dynamic Node Positioning:** Nodes are automatically distributed in a circular layout for clarity.
* **Interactive GUI:** Includes a "PLAY/PAUSE" button to control the animation.
* **Realistic Animation:** Travelers move according to edge weights.
* **Multiple Processes:** Each traveler is represented by a child process created using fork().
* **IPC:** Child processes communicate their progress to the parent process through a pipe.
* **Synchronization:** Each node is protected by a semaphore to prevent more than one traveler from occupying it at the same time.
* **Waiting Visualization:** Travelers waiting outside occupied nodes are displayed in a different color.
* **Terminal Logs:** The parent process prints traveler arrivals, waiting states, and finish events.
* **Visual Feedback:** The shortest path is highlighted in blue, and arrival is confirmed with a message.
