CC=gcc
CFLAGS=-Wall -g

milestone1:
	$(CC) $(CFLAGS) main.c -o dijkstra

milestone2:
	$(CC) $(CFLAGS) main.c -o sim

milestone3:
	$(CC) $(CFLAGS) main.c -o sim

clean:
	del dijkstra.exe sim.exe