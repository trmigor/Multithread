FLAGS = -I include -fPIC -Wall -Wextra -pedantic -O3 -Wshadow -Wformat=2 -Wfloat-equal -Wconversion -Wcast-qual -Wcast-align #-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=address,undefined -fno-sanitize-recover=all -fstack-protector
CFLAGS = $(FLAGS) -std=c99
CPPFLAGS = $(FLAGS) -std=c++17

all: build build/s_lock.o build/spin_lock.o build/spin_unlock.o

build:
	mkdir build

build/s_lock.o: source/s_lock.cpp
	g++ $(CPPFLAGS) -c -o build/s_lock.o source/s_lock.cpp

build/spin_lock.o: source/spin_lock.s
	gcc $(CFLAGS) -c -o build/spin_lock.o source/spin_lock.s

build/spin_unlock.o: source/spin_unlock.s
	gcc $(CFLAGS) -c -o build/spin_unlock.o source/spin_unlock.s

lib: build build/s_lock.o build/spin_lock.o build/spin_unlock.o
	mkdir lib
	ar rc lib/libspinlock.a build/s_lock.o build/spin_lock.o build/spin_unlock.o
	ranlib lib/libspinlock.a
	g++ -shared -o lib/libspinlock.so build/s_lock.o build/spin_lock.o build/spin_unlock.o

clean:
	rm -rf build/ lib/
