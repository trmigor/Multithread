CPPFLAGS = -I include -Wall -Wextra -pedantic -std=c++17 -O3 -Wshadow -Wformat=2 -Wfloat-equal -Wconversion -Wcast-qual -Wcast-align -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=address,undefined -fno-sanitize-recover=all -fstack-protector

out-64: build build/main.o build/s_lock.o build/spin_lock.o build/spin_unlock.o
	g++ $(CPPFLAGS) -o build/out-64 build/main.o build/s_lock.o build/spin_lock.o build/spin_unlock.o
	make clean

build/main.o:
	g++ $(CPPFLAGS) -c -o build/main.o source/main.cpp

build/s_lock.o:
	g++ $(CPPFLAGS) -c -o build/s_lock.o source/s_lock.cpp

build/spin_lock.o:
	g++ $(CPPFLAGS) -c -o build/spin_lock.o source/spin_lock.s

build/spin_unlock.o:
	g++ $(CPPFLAGS) -c -o build/spin_unlock.o source/spin_unlock.s

build:
	mkdir build

clean:
	rm -rf build/*.o build/*.out
