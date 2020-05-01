#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifndef TICKET_LOCK_INCLUDE_T_LOCK_HPP_
#define TICKET_LOCK_INCLUDE_T_LOCK_HPP_

typedef struct {
    int64_t volatile now_serving;
    char volatile padding[56];
    int64_t volatile next_ticket;
} t_lock;

#ifdef __cplusplus
extern "C" {
#endif

    t_lock* ticket_init(void);
    void ticket_delete(t_lock *lock);
    void ticket_lock(t_lock *lock) __asm__("ticket_lock");
    void ticket_unlock(t_lock *lock) __asm__("ticket_unlock");

#ifdef __cplusplus
}
#endif

#endif  // TICKET_LOCK_INCLUDE_T_LOCK_HPP_
