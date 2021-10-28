#isa 2021-pop3 client
#author: Jakub Komárek (xkomar33)

CFLAGS= -Wall -g

BIN=popcl
ZIP=xkomar33.zip
CC=g++ 
RM=rm -f
SRC=$(wildcard src/**/*.cpp) $(wildcard src/*.cpp)
PATHS=$(addprefix ../, $(SRC))
.PHONY: all build doxygen run pack clean

all: build

build: 
	if [ -d "build" ]; then rm -r build; fi && \
	mkdir build && \
	cd build && \
	$(CC) $(CFLAGS) -o $(BIN) $(PATHS) -lssl -lcrypto && \
	cp $(BIN) ../

pack: clean
	cd src && \
	zip -r $(ZIP) * README.txt && \
	mv $(ZIP) ../

run:
	test -f $(BIN) && ./$(BIN)
valgrind:
	test -f $(BIN) && valgrind ./$(BIN)

doxygen:
	if [ -d "doc" ]; then rm -r doc; fi && \
	mkdir doxygen && \
	doxygen Doxyfile

clean:
	rm -rf $(BIN) build/ $(ZIP)
