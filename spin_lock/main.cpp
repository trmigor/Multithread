#include <iostream>
#include "s_lock.hpp"

int main(void) {
    s_lock a;
    std::cout << a << std::endl;
    spin_init(&a);
    std::cout << a << std::endl;
    spin_lock(&a);
    std::cout << a << std::endl;
    spin_unlock(&a);
    std::cout << a << std::endl;
    std::cout << "Hi!" << std::endl;
}
