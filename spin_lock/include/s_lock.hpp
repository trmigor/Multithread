#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifndef SPIN_LOCK_INCLUDE_S_LOCK_HPP_
#define SPIN_LOCK_INCLUDE_S_LOCK_HPP_

typedef int64_t s_lock;

#ifdef __cplusplus
extern "C" {
#endif

    s_lock* spin_init(void);
    void spin_delete(s_lock *lock);
    void spin_lock(s_lock *lock) __asm__("spin_lock");
    void spin_unlock(s_lock *lock) __asm__("spin_unlock");

#ifdef __cplusplus
}
#endif

#endif  // SPIN_LOCK_INCLUDE_S_LOCK_HPP_
