CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs`
OBJ = main.o chip8.o

chip8: $(OBJ)
	$(CC) $(CFLAGS) -o chip8 $(OBJ) $(LDFLAGS)

main.o: src/main.c src/chip8.h
	$(CC) $(CFLAGS) -c src/main.c -o main.o

chip8.o: src/chip8.c src/chip8.h
	$(CC) $(CFLAGS) -c src/chip8.c -o chip8.o

clean:
	rm -f chip8 $(OBJ)
