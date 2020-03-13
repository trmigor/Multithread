#ifndef S_LOCK
#define S_LOCK

typedef long s_lock;

extern "C" {
    s_lock* spin_init(s_lock *lock);
    void spin_lock(s_lock *lock);
    void spin_unlock(s_lock *lock);
}

#endif
