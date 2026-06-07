CC = gcc

CFLAGS = -Wall -Wextra -g

RAYLIB = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# =========================
# Milestones
# =========================

milestone1:
	$(CC) $(CFLAGS) dijkstra.c -o dijkstra

milestone2:
	$(CC) $(CFLAGS) -DSIMULATION_MODE sim.c dijkstra.c -o sim $(RAYLIB)

milestone3:
	$(CC) $(CFLAGS) -DSIMULATION_MODE sim.c dijkstra.c -o sim $(RAYLIB)

milestone4:
	$(CC) $(CFLAGS) -DSIMULATION_MODE milestone4.c dijkstra.c -o sim $(RAYLIB)

milestone5:
	$(CC) $(CFLAGS) -DSIMULATION_MODE milestone5.c dijkstra.c -o sim $(RAYLIB)

clean:
	rm -f dijkstra sim milestone4 milestone5 *.o
