FLAGS = -Wall -Werror -Wextra -std=gnu++11
GXX = g++

all: example.cc BTree.h
	$(GXX) $(FLAGS) example.cc -o example

clean:
	rm example
