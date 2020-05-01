FLAGS = -I include -fPIC -Wall -Wextra -pedantic -O3 -Wshadow -Wformat=2 -Wfloat-equal -Wconversion -Wcast-qual -Wcast-align #-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=address,undefined -fno-sanitize-recover=all -fstack-protector
CPPFLAGS = $(FLAGS) -std=c++17

all: build build/Matrix.o build/main.o
	g++ $(CPPFLAGS) -o a.out build/Matrix.o build/main.o

build:
	mkdir build

build/main.o: demo/main.cpp
	g++ $(CPPFLAGS) -c -o build/main.o demo/main.cpp

build/Matrix.o: source/Matrix.cpp
	g++ $(CPPFLAGS) -c -o build/Matrix.o source/Matrix.cpp

lib: build/Matrix.o
	rm -rf lib/
	mkdir lib/
	ar rc lib/libmatrix.a build/Matrix.o
	ranlib lib/libmatrix.a
	g++ -shared -o lib/libmatrix.so build/Matrix.o

clean:
	rm -rf build/ lib/ a.out
