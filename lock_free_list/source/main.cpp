#include <unistd.h>
#include <exception>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include "list.hpp"

void thr_job_even(size_t num, list::list<int>* numbers) {
    numbers->thread_attach();
    int left = 100 / 10 * static_cast<int>(num);
    int right = 100 / 10 * (static_cast<int>(num) + 1);
    for (int i = left; i < right; ++i) {
        numbers->push_front(i + 1);
    }
}

void thr_job_odd(list::list<int>* numbers) {
    numbers->thread_attach();
    for (size_t i = 0; i < 10; ++i) {
        try {
            numbers->pop_front();
        } catch (std::out_of_range e) {
            std::stringstream s;
            s << e.what() << std::endl;
            std::cout << s.str();
        }
    }
}

void thr_job_swap(list::list<int>* numbers) {
    numbers->thread_attach();
    numbers->swap_first();
}

void thr_job_ABA(list::list<int>* numbers) {
    numbers->thread_attach();
    sleep(1);
    numbers->pop_front();
    numbers->pop_front();
    numbers->pop_front();
    numbers->pop_front();
    numbers->push_front(1);
}

int main(void) {
    list::list<int> numbers(static_cast<size_t>(1));

    std::vector<std::thread> threads;
    for (size_t i = 0; i < 10; ++i) {
        if (i % 2 == 0) {
            threads.push_back(std::thread(&thr_job_even, i, &numbers));
        } else {
            threads.push_back(std::thread(&thr_job_odd, &numbers));
        }
    }

    for (size_t i = 0; i < 10; ++i) {
        threads[i].join();
    }

    try {
        std::cout << numbers.front() << " " << numbers.size() << std::endl;
    } catch (std::out_of_range e) {
        std::cout << "Empty" << std::endl;
    }

    for (auto e : numbers) {
        std::cout << e << " ";
    }
    std::cout << std::endl;

    numbers.clear();
    numbers.reset_num_threads();
    std::cout << numbers.size() << std::endl;
    for (auto e : numbers) {
        std::cout << e << " ";
    }
    std::cout << std::endl;

    for (int i = 0; i < 10; ++i) {
        numbers.push_front(10 - i);
    }

    std::thread sw(&thr_job_swap, &numbers);
    std::thread aba(&thr_job_ABA, &numbers);

    sw.join();
    aba.join();

    for (auto e : numbers) {
        std::cout << e << " ";
    }
    std::cout << std::endl;
}
