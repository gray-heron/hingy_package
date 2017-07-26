CC=g++
CFLAGS=-I. -std=gnu++11 -pedantic -O3
LIBS = -lSDL2 -lSDL2_net -pthread
MODULES = build/main.o build/utils.o

build/%.o: evolutionary_trainer/%.cpp
	mkdir -p build
	$(CC) -c -o $@ $< $(CFLAGS)

default: $(MODULES)
	$(CC) -o evolutionary_trainer/trainer $(LIBS) $(MODULES)
