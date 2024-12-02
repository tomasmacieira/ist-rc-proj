CC = gcc

SRC = client/player.c

all:
	$(CC) $(SRC) -o player

# Clean up generated files
clean:
	rm -f player
