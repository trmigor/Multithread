# Multithread
Solutions for concurrent programs

Created for Multithread programming class in MIPT

## Spin lock
Low-level synchronization primitive with busy-waiting. Performed using Test-Test-And-Set paradigm.

Source code is written in x86-64 inline assembly and has C++ wrapper.

## Ticket lock
Fair synchronization algorithm.

Source code is written in x86-64 inline assembly and has C++ wrapper.

## Multithread matrix multiplication
Cache-friendly and fast.

Performed as C++ class.

## Lock-free list
Realization of lock-free list data structure. No for locks, yes for hazard pointers.

Now has only stack-way operations with head, but not with tail.

Performed as C++ template class.

## Lock-free skiplist
Realization of lock-free skiplist. No for locks, yes for reference count. Random-based, no-duplicate.

Performed as C++ template class.
