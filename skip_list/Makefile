FLAGS = -g -I include -fPIC -Wall -Wextra -pedantic -O1 -Wshadow -Wformat=2 -Wfloat-equal -Wconversion -Wcast-qual -Wcast-align -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=address,undefined -fno-sanitize-recover=all -fstack-protector
CPPFLAGS = $(FLAGS) -std=c++17

.PHONY: all
all: build build/main.o build/skip_list.o
	g++ $(CPPFLAGS) -o a.out build/main.o build/skip_list.o

build:
	mkdir build || echo "build: done"

build/main.o: source/main.cpp
	g++ $(CPPFLAGS) -c -o build/main.o source/main.cpp

build/skip_list.o: source/skip_list.cpp
	g++ $(CPPFLAGS) -c -o build/skip_list.o source/skip_list.cpp

.PHONY: clean
clean:
	rm -rf build a.out

.PHONY: bench res
bench: all
	for i in $$(seq 1 100); do ./a.out $$i >> res/time.txt; done
