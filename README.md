# Graph Simulation and Pathfinding Project

This project is a comprehensive C-based application that implements graph representation, Dijkstra's shortest path algorithm, graphical visualization with animation using the raylib library, multiple processes, inter-process communication, synchronization, and scheduling algorithms.

## Authors

* **Jinan Rasas**
* **Afnan Rabiah**
* **Mohammad Smoum**
* **Cleopatra Kajjo**

---

# Project Overview

The project is divided into seven main milestones:

1. **Milestone 1: Algorithmic Foundation** - Implementation of a weighted directed graph and Dijkstra's algorithm to find the shortest path between two nodes.
2. **Milestone 2: Graphical Representation** - Visualizing the graph nodes and edges in a window using raylib.
3. **Milestone 3: Path Animation** - Animating an entity moving along the calculated shortest path with speed proportional to edge weights.
4. **Milestone 4: Multiple Processes** - The parent process manages the GUI, creates child processes using fork(), and animates multiple travelers simultaneously.
5. **Milestone 5: IPC** - Each child process calculates its own path and sends updates to the parent process using a pipe.
6. **Milestone 6: Synchronization** - Graph nodes are protected using semaphores to ensure mutual exclusion.
7. **Milestone 7: Scheduling Algorithms** - FCFS and SJF scheduling policies determine which traveler may enter a node when multiple travelers are waiting.

---

# Milestone 5 - IPC Explanation

For Milestone 5, we used a pipe as the IPC mechanism.

Each traveler is represented by a child process created using fork().

Each child process calculates its own shortest path using Dijkstra's algorithm.

Whenever the child reaches a node, it sends a message to the parent through the pipe.

The parent process receives the messages, updates the GUI, and prints status information.

We chose pipes because communication is primarily one-way: travelers report their progress to the parent process.

---

# Milestone 6 - Synchronization Explanation

For Milestone 6, we used POSIX semaphores to synchronize access to graph nodes.

Each node is protected by a semaphore, ensuring that only one traveler may occupy a node at any given time.

Before entering a node, a traveler attempts to acquire the node semaphore. If the node is occupied, the traveler must wait until the semaphore becomes available.

After occupying a node for one second, the traveler releases the semaphore and allows another waiting traveler to enter.

The GUI clearly displays waiting travelers, making synchronization behavior visible during execution.

We chose semaphores because they provide efficient mutual exclusion and prevent collisions between travelers.

---

# Milestone 7 - Scheduling Algorithms

## Overview

Milestone 7 extends the synchronization mechanism by replacing random node access ordering with scheduling algorithms.

When multiple travelers request entry to the same node, the parent process acts as a scheduler and decides which traveler is allowed to enter next.

Two scheduling algorithms are implemented:

### FCFS (First Come First Served)

Travelers are served according to their arrival order.

Example:

Traveler A arrives first

Traveler B arrives second

Traveler C arrives third

Entry order:

A → B → C

### SJF (Shortest Job First)

Travelers are selected according to the shortest upcoming path segment weight.

If two travelers have equal segment weights, arrival time is used as a tie-breaker.

This algorithm may reduce average waiting times compared to FCFS.

## Implementation Details

The parent process maintains a separate waiting queue for each graph node.

When a traveler requests entry:

1. The traveler sends a request message to the parent.
2. If the node is free, access is granted immediately.
3. If the node is occupied, the traveler is inserted into the node's waiting queue.
4. When the current traveler leaves, the parent selects the next traveler according to the active scheduling algorithm.
5. The selected traveler receives permission to enter the node.

The scheduling algorithm is selected at runtime using command-line arguments.

## Scheduling Commands

Run with FCFS:

```bash
./sim -schd fcfs input.txt
```

Run with SJF:

```bash
./sim -schd sjf input.txt
```

## GUI Support

The GUI displays the currently active scheduling algorithm during execution.

## Comparison Between Algorithms

### FCFS

Advantages:

* Fair and predictable.
* Preserves arrival order.
* Prevents starvation.

Disadvantages:

* Can lead to longer average waiting times.

### SJF

Advantages:

* Often reduces average waiting time.
* Improves throughput when short jobs are present.

Disadvantages:

* Less fair than FCFS.
* Long jobs may experience longer waits.

In our experiments, SJF generally reduced waiting times when several travelers competed for the same node, while FCFS preserved strict arrival order.

---

# File Structure

* `dijkstra.c` - Graph implementation and Dijkstra's shortest path algorithm.
* `dijkstra.h` - Header file for graph and Dijkstra functionality.
* `sim.c` - Main simulation program.
* `milestone4.c` - Multiple traveler implementation using fork().
* `milestone5.c` - IPC implementation using pipes.
* `milestone6.c` - Synchronization implementation using semaphores.
* `milestone7.c` - Scheduling implementation using FCFS and SJF algorithms.
* `Makefile` - Build script for all milestones.
* `input.txt` - Sample graph input file.

---

# Prerequisites

To run the graphical simulation, raylib must be installed.

## macOS

```bash
brew install raylib
```

## Ubuntu / Debian

```bash
sudo apt install libraylib-dev
```

---

# How to Build and Run

## Compilation

Compile Milestone 1:

```bash
make milestone1
```

Compile Milestone 2:

```bash
make milestone2
```

Compile Milestone 3:

```bash
make milestone3
```

Compile Milestone 4:

```bash
make milestone4
```

Compile Milestone 5:

```bash
make milestone5
```

Compile Milestone 6:

```bash
make milestone6
```

Compile Milestone 7:

```bash
make milestone7
```

---

## Cleaning

```bash
make clean
```

---

## Running the Programs

### Milestone 1

```bash
./dijkstra input.txt
```

### Milestones 2–6

```bash
./sim input.txt
```

### Milestone 7

FCFS:

```bash
./sim -schd fcfs input.txt
```

SJF:

```bash
./sim -schd sjf input.txt
```

---

# Input File Format

## Milestone 1

Input format:

```text
N M
src dst weight
...
start end
```

Example:

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

---

## Milestones 4–7

Input format:

```text
N M
src dst weight
...
T
source destination
source destination
...
```

Where:

* N = Number of nodes
* M = Number of edges
* T = Number of travelers

Example:

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
```

---

# Features

* Weighted directed graph implementation.
* Dijkstra shortest-path algorithm.
* Dynamic node positioning.
* Interactive graphical interface.
* Animated traveler movement.
* Multiple concurrent processes using fork().
* Inter-process communication using pipes.
* Synchronization using POSIX semaphores.
* Waiting queue management.
* FCFS scheduling algorithm.
* SJF scheduling algorithm.
* Runtime scheduler selection.
* Visual display of active scheduling policy.
* Traveler waiting-state visualization.
* Terminal event logging.
* Path highlighting and movement animation.

---

# Final Submission

The final submission contains all seven milestones.

The repository includes:

* Source code for all milestones.
* Working Makefile with milestone targets.
* This README file.
* Sample input files.
* Demonstration video showing FCFS and SJF executions on the same graph and comparing their behavior.
