FLAGS = -g -I include -fPIC -Wall -Wextra -pedantic -O1 -Wshadow -Wformat=2 -Wfloat-equal -Wconversion -Wcast-qual -Wcast-align -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=address,undefined -fno-sanitize-recover=all -fstack-protector
CPPFLAGS = $(FLAGS) -std=c++17

.PHONY: all
all: build build/main.o
	g++ $(CPPFLAGS) -o a.out build/main.o
	./a.out

build:
	mkdir build || echo "build: done"

build/main.o:
	g++ $(CPPFLAGS) -c -o build/main.o source/main.cpp

.PHONY: clean
clean:
	rm -rf build a.out
