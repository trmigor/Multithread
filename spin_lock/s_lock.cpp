#include "s_lock.hpp"
#include <stdlib.h>

s_lock* spin_init(void) {
    return (s_lock*) malloc(sizeof(s_lock));
}

void spin_delete(s_lock *lock) {
    free(lock);
}
