CPPFLAGS = -Wall -Wextra -pedantic -std=c++17 -O3 -Wshadow -Wformat=2 -Wfloat-equal -Wconversion -Wcast-qual -Wcast-align -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize,undefined -fno-sanitize-recover=all -fstack-protector

out-64: main.o s_lock.o spin_init.o spin_lock.o spin_unlock.o
	g++ $(CPPFLAGS) -o out-64 main.o spin_init.o spin_lock.o spin_unlock.o
	make clean

main.o:
	g++ $(CPPFLAGS) -c -o main.o main.cpp

s_lock.o:
	g++ $(CPPFLAGS) -c -o s_lock.o s_lock.cpp

spin_init.o:
	g++ $(CPPFLAGS) -c -o spin_init.o spin_init.s

spin_lock.o:
	g++ $(CPPFLAGS) -c -o spin_lock.o spin_lock.s

spin_unlock.o:
	g++ $(CPPFLAGS) -c -o spin_unlock.o spin_unlock.s

clean:
	rm -rf *.o *.out
