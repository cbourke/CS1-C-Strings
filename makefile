#
# makefile for a horse game
#

CC = gcc
CCFLAGS = -Iinclude -std=c99

all: horse

horse: main.o game_functions.o
	$(CC) $(CCFLAGS) obj/main.o obj/game_functions.o -o horse -lm

main.o: src/main.c
	@mkdir -p obj
	$(CC) $(CCFLAGS) -c src/main.c -o obj/$@

gameFunctions.o: src/game_functions.c include/game_functions.h
	@mkdir -p obj
	$(CC) $(CCFLAGS) -c src/game_functions.c -o obj/$@ -lm

clean:
	rm -rf obj/*o
