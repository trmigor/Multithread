#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <thread>
#include <vector>

#include "skip_list.hpp"

static unsigned int SEED = time(nullptr);
static size_t MANY = 1000000;
static size_t NUM_THR = 1;

void thr_find_job(skip_list::skiplist<int>* slist) {
    size_t many = MANY * 9 / 10 / NUM_THR;
    for (size_t i = 0; i < many; ++i) {
        slist->find(rand_r(&SEED) % MANY);
        // std::cout << "find " << i << std::endl;
    }
}

void thr_insert_job(skip_list::skiplist<int>* slist) {
    size_t many = MANY * 9 / 100 / NUM_THR;
    for (size_t i = 0; i < many; ++i) {
        slist->insert(rand_r(&SEED) % MANY);
        // std::cout << "insert " << i << std::endl;
    }
}

void thr_erase_job(skip_list::skiplist<int>* slist) {
    size_t many = MANY * 1 / 100 / NUM_THR;
    for (size_t i = 0; i < many; ++i) {
        slist->erase(rand_r(&SEED) % MANY);
        // std::cout << "erase " << i << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        NUM_THR = std::stoull(argv[1]);
    }

    skip_list::skiplist<int> slist;

    for (size_t i = 0; i < MANY; ++i) {
        slist.insert(i % 10);
    }

    /*std::cout << slist.size() << std::endl;
    for (auto it = slist.begin(); it != slist.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;*/

    auto start = std::chrono::system_clock::now();

    std::vector<std::thread> threads;
    for (size_t i = 0; i < 3 * NUM_THR; ++i) {
        if (i % 3 == 0) {
            threads.push_back(std::thread(&thr_find_job, &slist));
        }
        if (i % 3 == 1) {
            threads.push_back(std::thread(&thr_insert_job, &slist));
        }
        if (i % 3 == 2) {
            threads.push_back(std::thread(&thr_erase_job, &slist));
        }
    }

    for (size_t i = 0; i < threads.size(); ++i) {
        threads[i].join();
    }

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << NUM_THR * 3 << " " << elapsed.count() << std::endl;

    /*for (auto e : slist) {
        std::cout << e << " ";
    }
    std::cout << std::endl;*/
}
