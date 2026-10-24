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

game_functions.o: src/game_functions.c include/game_functions.h
	@mkdir -p obj
	$(CC) $(CCFLAGS) -c src/game_functions.c -o obj/$@ -lm

game_function_tests: game_functions.o test/game_function_tests.c
	$(CC) $(CCFLAGS) obj/game_functions.o test/game_function_tests.c -o game_function_tests -lm -lcmocka

clean:
	rm -rf obj/*o
