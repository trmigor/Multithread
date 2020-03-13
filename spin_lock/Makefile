out-64: main.o spin_lock_TTAS_lock.o spin_lock_TTAS_unlock.o
	g++ -o out-64 main.o spin_lock_TTAS_lock.o spin_lock_TTAS_unlock.o
	make clean

main.o:
	g++ -c -o main.o main.cpp

spin_lock_TTAS_lock.o:
	g++ -c -o spin_lock_TTAS_lock.o spin_lock_TTAS_lock.s

spin_lock_TTAS_unlock.o:
	g++ -c -o spin_lock_TTAS_unlock.o spin_lock_TTAS_unlock.s

clean:
	rm -rf *.o *.out
