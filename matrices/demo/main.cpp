#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <chrono>
#include <fstream>

#include "Matrix.hpp"

#define forn(i, n) for (size_t i = 0; i < size_t(n); ++i)

int main(int argc, char *argv[]) {
    size_t num_threads;
    size_t m_size;
    size_t many;
    if (argc < 2) {
        num_threads = 1;
    } else {
        num_threads = std::stoull(argv[1]);
    }
    if (argc < 3) {
        m_size = 1;
    } else {
        m_size = std::stoull(argv[2]);
    }
    if (argc < 4) {
        many = 1;
    } else {
        many = std::stoull(argv[3]);
    }

    matrix::Matrix a(m_size, m_size, num_threads);
    forn(i, a.Rows()) {
        forn(j, a.Cols()) {
            if (i == j) {
                a[i][j] = 1;
            }
        }
    }

    double res = 0.0;
    forn(i, many) {
        auto start = std::chrono::system_clock::now();
        a = a * a;
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        res += elapsed.count();
    }

    std::cout << num_threads << " " << m_size << " " << res / many << std::endl;

    return 0;
}
