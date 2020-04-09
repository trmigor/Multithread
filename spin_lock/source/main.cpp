#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <unistd.h>
#include "s_lock.hpp"

s_lock *a;
size_t res = 0;

void F(size_t num_thr, size_t many) {
    usleep(1000);
    for (size_t i = 0; i < many / num_thr; ++i) {
        spin_lock(a);
        ++res;
        spin_unlock(a);
    }
}

int main(int argc, char *argv[]) {
    size_t num_thr, many, more;
    if (argc < 2) {
        num_thr = 0;
    } else {
        num_thr = (size_t) std::stol(argv[1]);
    }
    if (argc < 3) {
        many = 1;
    } else {
        many = (size_t) std::stol(argv[2]);
    }
    if (argc < 4) {
        more = 1;
    } else {
        more = (size_t) std::stol(argv[3]);
    }

    a = spin_init();

    double sum = 0.0;
    for (size_t k = 0; k < more; ++k) {
        res = 0;
        auto start = std::chrono::system_clock::now();
        
        std::vector<std::thread> threads;
        for (size_t i = 0; i < num_thr; ++i) {
            threads.push_back(std::thread(F, num_thr, many));
        }

        for (size_t i = 0; i < threads.size(); ++i) {
            threads[i].join();
        }

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        sum += elapsed.count();
    }

    std::cout.precision(7);
    std::cout << num_thr << " " << sum / more << " " << res << std::endl;
    
    spin_delete(a);
}
