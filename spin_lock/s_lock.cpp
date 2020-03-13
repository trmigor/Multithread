#include "s_lock.hpp"
#include <stdlib.h>

s_lock* spin_init(void) {
    s_lock *lock = (s_lock*) malloc(sizeof(s_lock));
    *lock = 0;
    return lock;
}

void spin_delete(s_lock *lock) {
    free(lock);
}
