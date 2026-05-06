# ===== Compiler =====
CC = gcc
CFLAGS = -Wall -Wextra -g

# ===== Raylib =====
RAYLIB = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# ===== Files =====
DIJKSTRA = dijkstra.c
SIM = sim.c

# ===== Outputs =====
CLI_OUT = dijkstra
SIM_OUT = sim

# =========================
# Milestone 1 (CLI only)
# =========================
milestone1:
	$(CC) $(CFLAGS) $(DIJKSTRA) -o $(CLI_OUT)

# =========================
# Milestone 2–6 (GUI)
# =========================
milestone2 milestone3 milestone4 milestone5 milestone6:
	$(CC) $(CFLAGS) -DNO_MAIN $(SIM) $(DIJKSTRA) -o $(SIM_OUT) $(RAYLIB)



# =========================
# Clean
# =========================
clean:
	rm -f $(CLI_OUT) $(SIM_OUT)