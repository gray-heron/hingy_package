CC=g++
CFLAGS=-I. -std=gnu++11 -pedantic -O3
LIBS = -lSDL2 -lSDL2_net
MODULES = build/driver.o build/hingy_math.o build/hingy_track.o build/main.o build/torcs_integration.o build/utils.o

build/%.o: hingybot/%.cpp
	mkdir -p build
	$(CC) -c -o $@ $< $(CFLAGS)

default: $(MODULES)
	$(CC) -o hingybot/hingybot $(LIBS) $(MODULES)

clean:
	rm -r build
	rm hingybot/hingybot
