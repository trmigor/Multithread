#include "t_lock.hpp"
#include <cstdlib>

t_lock* ticket_init(void) {
    t_lock *res = reinterpret_cast<t_lock*>(malloc(sizeof(t_lock)));
    res->next_ticket = 0;
    res->now_serving = 0;
    return res;
}

void ticket_delete(t_lock *lock) {
    free(lock);
}
