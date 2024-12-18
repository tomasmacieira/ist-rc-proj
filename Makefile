CC = gcc

CLT = client/player.c
SV = server/gs.c

all:
	$(CC) $(CLT) -o player
	$(CC) $(SV) -o gs

clean:
	rm -f player
	rm -f gs
	rm -rf server/games/*
	rm -rf server/scores/*