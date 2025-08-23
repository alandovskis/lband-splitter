CXX=g++
CXXFLAGS=-std=c++17 -O2 -Wall
SRC=$(wildcard src/*.cpp)
OBJ=$(SRC:.cpp=.o)
BIN=splitter

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean
