FLAGS = -I include -fPIC -Wall -Wextra -pedantic -O3 -Wshadow -Wformat=2 -Wfloat-equal -Wconversion -Wcast-qual -Wcast-align #-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=address,undefined -fno-sanitize-recover=all -fstack-protector
CFLAGS = $(FLAGS) -std=c99
CPPFLAGS = $(FLAGS) -std=c++17

all: build build/t_lock.o build/ticket_lock.o build/ticket_unlock.o

build:
	mkdir build

build/t_lock.o: source/t_lock.cpp
	g++ $(CPPFLAGS) -c -o build/t_lock.o source/t_lock.cpp

build/ticket_lock.o: source/ticket_lock.s
	gcc $(CFLAGS) -c -o build/ticket_lock.o source/ticket_lock.s

build/ticket_unlock.o: source/ticket_unlock.s
	gcc $(CFLAGS) -c -o build/ticket_unlock.o source/ticket_unlock.s

lib: build build/t_lock.o build/ticket_lock.o build/ticket_unlock.o
	mkdir lib
	ar rc lib/libticketlock.a build/t_lock.o build/ticket_lock.o build/ticket_unlock.o
	ranlib lib/libticketlock.a
	g++ -shared -o lib/libticketlock.so build/t_lock.o build/ticket_lock.o build/ticket_unlock.o

clean:
	rm -rf build/ lib/
