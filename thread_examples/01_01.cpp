#include <iostream>
#include <vector>
#include <sstream>
#include <thread>

using namespace std;

void Func(int num) {
    stringstream out; 
    out << "Number = " << num << "; id = " << this_thread::get_id() << "\n";
    cout << out.str();
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return EXIT_FAILURE;
    }

    size_t N;
    N = stoi(argv[1]);
    vector<thread> threads;
    for (size_t i = 0; i < N; ++i) {
        threads.push_back(thread(Func, i));
    }

    for (size_t i = 0; i < N; ++i) {
        threads[i].join();
    }

    return EXIT_SUCCESS;
}