#ifndef S_LOCK
#define S_LOCK

typedef int64_t s_lock;

s_lock* spin_init(void);

extern "C" {
    void spin_lock(s_lock *lock);
    void spin_unlock(s_lock *lock);
}

#endif
