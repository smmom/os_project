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
	$(CC) $(CFLAGS) milestone4_5.c -o milestone4_5 $(INCLUDES) $(LDFLAGS)

milestone5:
	$(CC) $(CFLAGS) milestone4_5.c -o milestone4_5 $(INCLUDES) $(LDFLAGS)


# =========================
# Clean
# =========================

clean:
	rm -f dijkstra sim *.o
