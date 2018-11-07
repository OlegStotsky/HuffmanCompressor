# Basic make file.

all: smoke

huffman: main.cpp huffman.cpp huffman.hpp
	clang++ -g -Wall -Wextra -std=c++17 -o huffman main.cpp huffman.cpp

smoke: huffman
	cd smoke_test && ./smoke_test.sh ../huffman
