CC=gcc
CFLAGS=-Wall -g

milestone1:
	$(CC) $(CFLAGS) dijkstra.c -o dijkstra
milestone2:
	$(CC) $(CFLAGS) main.c -o sim

milestone3:
	$(CC) $(CFLAGS) main.c -o sim

clean:
	rm -f dijkstra sim dijkstra.exe sim.exe