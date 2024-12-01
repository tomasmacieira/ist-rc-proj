CC = gcc

SRC = playerAPP/player.c

all:
	$(CC) $(SRC) -o player

# Clean up generated files
clean:
	rm -f player
