#include <iostream>

typedef long s_lock;

extern "C" {
    s_lock* spin_init(s_lock *lock);
    void spin_lock(s_lock *lock);
    void spin_unlock(s_lock *lock);
}

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
