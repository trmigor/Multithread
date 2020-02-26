#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <unistd.h>

using namespace std;

int fd[2];

void Func(char num) {
    if (num <= '4') {
        for (size_t i = 0; i < 100; ++i) {
            write(fd[1], &num, 1);
        }
    } else {
        ofstream out(to_string(num - '4') + ".txt");
        for (size_t i = 0; i < 100; ++i) {
            char res;
            read(fd[0], &res, 1);
            out << res << endl;
        }
    }
}

int main(void) {
    pipe(fd);

    vector<thread> threads;
    for (char i = '1'; i < '9'; ++i) {
        threads.push_back(thread(Func, i));
    }

    for (size_t i = 0; i < 8; ++ i) {
        threads[i].join();
    }

    return EXIT_SUCCESS;
}