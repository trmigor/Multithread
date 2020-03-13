out-64: main.o s_lock.o spin_init.o spin_lock.o spin_unlock.o
	g++ -o out-64 main.o spin_init.o spin_lock.o spin_unlock.o
	make clean

main.o:
	g++ -c -o main.o main.cpp

s_lock.o:
	g++ -c -o s_lock.o s_lock.cpp

spin_init.o:
	g++ -c -o spin_init.o spin_init.s

spin_lock.o:
	g++ -c -o spin_lock.o spin_lock.s

spin_unlock.o:
	g++ -c -o spin_unlock.o spin_unlock.s

clean:
	rm -rf *.o *.out
