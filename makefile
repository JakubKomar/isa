#ISA 2021-pop3 client
#author: Jakub Komárek (xkomar33)

CFLAGS= -Wall -g

BIN=popcl
ZIP=xkomar33.zip
CC=g++ 
RM=rm -f
SRC=$(wildcard src/*.cpp)
.PHONY: all build run pack clean

all: build

build: 
	$(CC) $(CFLAGS) -o $(BIN) $(SRC) -lssl -lcrypto 

pack: clean
	cd src && \
	zip -r $(ZIP) *  && \
	mv $(ZIP) ../

run:
	test -f $(BIN) && ./$(BIN)

valgrind:
	test -f $(BIN) && valgrind ./$(BIN)

clean:
	rm -rf $(BIN) $(ZIP)
