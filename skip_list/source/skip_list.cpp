#include "skip_list.hpp"

#include <cstdlib>
#include <ctime>
#include <thread>

namespace skip_list {

unsigned int SEED = time(nullptr);

static inline void __sl_node_init(node* node, size_t top_layer) {
    if (top_layer > UINT8_MAX) {
        top_layer = UINT8_MAX;
    }

    assert(node->is_fully_linked == false);
    assert(node->being_modified == false);

    node->is_fully_linked.store(false, MOR);
    node->being_modified.store(false, MOR);
    node->removed.store(false, MOR);

    if (node->top_layer != top_layer || node->next == nullptr) {
        node->top_layer = top_layer;

        if (node->next != nullptr) {
            delete[] node->next;
        }
        node->next = new atm_node_ptr[top_layer + 1];
    }
}

void skiplist_init(raw* slist, cmp_t* cmp_func) {
    slist->cmp_func = nullptr;
    slist->aux = nullptr;

    slist->fanout = 4;
    slist->max_layer = 12;
    slist->num_entries = 0;

    slist->layer_entries = new atm_uint32_t[slist->max_layer];
    slist->top_layer = 0;

    init_node(&slist->head);
    init_node(&slist->tail);

    __sl_node_init(&slist->head, slist->max_layer);
    __sl_node_init(&slist->tail, slist->max_layer);

    for (size_t layer = 0; layer < slist->max_layer; ++layer) {
        slist->head.next[layer] = &slist->tail;
        slist->tail.next[layer] = nullptr;
    }

    slist->head.is_fully_linked.store(true, MOR);
    slist->tail.is_fully_linked.store(true, MOR);
    slist->cmp_func = cmp_func;
}

void skiplist_free(raw* slist) {
    free_node(&slist->head);
    free_node(&slist->tail);
    delete[] slist->layer_entries;
    slist->layer_entries = nullptr;

    slist->aux = nullptr;
    slist->cmp_func = nullptr;
}

void init_node(node* node) {
    node->next = nullptr;

    node->is_fully_linked.store(false, MOR);
    node->being_modified.store(false, MOR);
    node->removed.store(false, MOR);

    node->accessing_next = 0;
    node->top_layer = 0;
    node->ref_count = 0;
}

void free_node(node* node) {
    delete[] node->next;
    node->next = nullptr;
}

size_t get_size(raw* slist) {
    return slist->num_entries.load(MOR);
}

raw_config get_default_config() {
    raw_config res;
    res.fanout = 4;
    res.maxLayer = 12;
    res.aux = nullptr;
    return res;
}

raw_config get_config(raw* slist) {
    raw_config res;
    res.fanout = slist->fanout;
    res.maxLayer = slist->max_layer;
    res.aux = slist->aux;
    return res;
}

void set_config(raw* slist, raw_config config) {
    slist->fanout = config.fanout;

    slist->max_layer = config.maxLayer;
    if (slist-> layer_entries != nullptr) {
        delete[] slist->layer_entries;
    }
    slist->layer_entries = new atm_uint32_t[slist->max_layer];

    slist->aux = config.aux;
}

static inline int __sl_cmp(raw* slist, node* a, node* b) {
    if (a == b) {
        return 0;
    }
    if (a == &slist->head || b == &slist->tail) {
        return -1;
    }
    if (a == &slist->tail || b == &slist->head) {
        return 1;
    }
    return slist->cmp_func(a, b, slist->aux);
}

static inline bool __sl_valid_node(node* node) {
    return node->is_fully_linked.load(MOR);
}

static inline void __sl_read_lock_an(node* node) {
    // RW lock
    while (true) {
        // Wait for active writer to release the lock
        uint32_t accessing_next = node->accessing_next.load(MOR);
        while (accessing_next & 0xfff00000) {
            std::this_thread::yield();
            accessing_next = node->accessing_next.load(MOR);
        }

        node->accessing_next.fetch_add(0x1, MOR);
        accessing_next = node->accessing_next.load(MOR);
        if ((accessing_next & 0xfff00000) == 0) {
            return;
        }

        node->accessing_next.fetch_sub(0x1, MOR);
    }
}

static inline void __sl_read_unlock_an(node* node) {
    node->accessing_next.fetch_sub(0x1, MOR);
}

static inline void __sl_write_lock_an(node* node) {
    // RW lock
    while (true) {
        // Wait for active writer to release the lock
        uint32_t accessing_next = node->accessing_next.load(MOR);
        while (accessing_next & 0xfff00000) {
            std::this_thread::yield();
            accessing_next = node->accessing_next.load(MOR);
        }

        node->accessing_next.fetch_add(0x100000, MOR);
        accessing_next = node->accessing_next.load(MOR);
        if ((accessing_next & 0xfff00000) == 0x100000) {
            while (accessing_next & 0x000fffff) {
                std::this_thread::yield();
                accessing_next = node->accessing_next.load(MOR);
            }
            return;
        }

        node->accessing_next.fetch_sub(0x100000, MOR);
    }
}

static inline void __sl_write_unlock_an(node* node) {
    node->accessing_next.fetch_sub(0x100000, MOR);
}

// __sl_next increases the `ref_count` of returned node as a result of working
// with it
// Caller is responsible to decrease it.
static inline node* __sl_next(raw* slist, node* cur_node, int layer,
        node* node_to_find, bool* found) {
    node* next_node = nullptr;

    // Turn on `accessing_next`:
    // now `cur_node` is not removable, so
    // `cur_node->next` will be consistent until clearing `accessing_next`.
    __sl_read_lock_an(cur_node); {
        if (!__sl_valid_node(cur_node)) {
            __sl_read_unlock_an(cur_node);
            return nullptr;
        }
        next_node = cur_node->next[layer].load(MOR);
        // Increase ref count of `next_node`:
        // now it can't be destroyed.
        assert(next_node != nullptr);
        next_node->ref_count.fetch_add(1, MOR);
        assert(next_node->top_layer >= layer);
    } __sl_read_unlock_an(cur_node);

    size_t num_nodes = 0;
    node* nodes[256];

    while ((next_node != nullptr && !__sl_valid_node(next_node)) ||
            next_node == node_to_find) {
        if (found != nullptr && node_to_find == next_node) {
            *found = true;
        }

        node* temp = next_node;
        __sl_read_lock_an(temp); {
            assert(next_node != nullptr);
            if (!__sl_valid_node(temp)) {
                __sl_read_unlock_an(temp);
                temp->ref_count.fetch_sub(1, MOR);
                next_node = nullptr;
                break;
            }
            next_node = temp->next[layer].load(MOR);
            next_node->ref_count.fetch_add(1, MOR);
            nodes[num_nodes++] = temp;
            assert(next_node->top_layer >= layer);
        } __sl_read_unlock_an(temp);
    }

    for (size_t ii = 0; ii < num_nodes; ++ii) {
        nodes[ii]->ref_count.fetch_sub(1, MOR);
    }

    return next_node;
}

static inline size_t __sl_decide_top_layer(raw* slist) {
    size_t layer = 0;
    while (layer + 1 < slist->max_layer) {
        if (rand_r(&SEED) % slist->fanout == 0) {
            // Flip a coin
            layer++;
        } else {
            // stop with probability (fanout - 1)/fanout
            break;
        }
    }
    return layer;
}

static inline void __sl_clr_flags(node** node_arr,
        size_t start_layer, size_t top_layer) {
    for (size_t layer = start_layer; layer <= top_layer; ++layer) {
        if (layer == top_layer || node_arr[layer] != node_arr[layer + 1]) {
            bool exp = true;
            bool done = node_arr[layer]->being_modified.compare_exchange_weak(
                exp, false);
            assert(done);
        }
    }
}

static inline bool __sl_valid_prev_next(node* prev, node* next) {
    return __sl_valid_node(prev) && __sl_valid_node(next);
}

static inline int __sl_insert(raw* slist, node* x, bool no_dup) {
    size_t top_layer = __sl_decide_top_layer(slist);

    __sl_node_init(x, top_layer);
    __sl_write_lock_an(x);

    node* prevs[MAX_LAYER];
    node* nexts[MAX_LAYER];

insert_retry:
    int cmp = 0;
    int cur_layer = 0;
    int layer;
    node* cur_node = &slist->head;
    cur_node->ref_count.fetch_add(1, MOR);

    size_t sl_top_layer = slist->top_layer;
    if (top_layer > sl_top_layer) {
        sl_top_layer = top_layer;
    }
    for (cur_layer = sl_top_layer; cur_layer <= sl_top_layer; --cur_layer) {
        do {
            node* next_node = __sl_next(slist, cur_node, cur_layer,
                nullptr, nullptr);
            if (next_node == nullptr) {
                __sl_clr_flags(prevs, cur_layer + 1, top_layer);
                cur_node->ref_count.fetch_sub(1, MOR);
                std::this_thread::yield();
                goto insert_retry;
            }
            cmp = __sl_cmp(slist, x, next_node);
            if (cmp > 0) {
                // if (cur_node < next_node < node) then move to next node
                node* temp = cur_node;
                cur_node = next_node;
                temp->ref_count.fetch_sub(1, MOR);
                continue;
            } else {
                // otherwise cur_node < node <= next_node
                next_node->ref_count.fetch_sub(1, MOR);
            }

            if (no_dup && cmp == 0) {
                // Not allow to duplicate the key.
                __sl_clr_flags(prevs, cur_layer + 1, top_layer);
                cur_node->ref_count.fetch_sub(1, MOR);
                return -1;
            }

            if (cur_layer <= top_layer) {
                // Before insertions 'prev' and 'next' must be fully linked
                // No other thread could modify 'prev' at the same time.
                prevs[cur_layer] = cur_node;
                nexts[cur_layer] = next_node;

                int error_code = 0;
                size_t locked_layer = cur_layer + 1;

                // Check prev node for the duplication in the upper layer.
                if (cur_layer < top_layer &&
                        prevs[cur_layer] == prevs[cur_layer + 1]) {
                    // It is already duplicated,
                    // so 'being_modified' flag is true
                    // do nothing.
                } else {
                    bool exp = false;
                    if (prevs[cur_layer]->being_modified.compare_exchange_weak(
                            exp, true)) {
                        locked_layer = cur_layer;
                    } else {
                        error_code = -1;
                    }
                }

                if (error_code == 0 && !__sl_valid_prev_next(prevs[cur_layer],
                        nexts[cur_layer])) {
                    error_code = -2;
                }

                if (error_code != 0) {
                    __sl_clr_flags(prevs, locked_layer, top_layer);
                    cur_node->ref_count.fetch_sub(1, MOR);
                    std::this_thread::yield();
                    goto insert_retry;
                }

                // Set current node's pointers.
                x->next[cur_layer].store(nexts[cur_layer], MOR);

                // Check if `cur_node->next` has been changed from `next_node`.
                node* next_node_again = __sl_next(slist, cur_node, cur_layer,
                    nullptr, nullptr);
                next_node_again->ref_count.fetch_sub(1, MOR);
                if (next_node_again != next_node) {
                    // Set all flags from current layer to top layer as default.
                    __sl_clr_flags(prevs, cur_layer, top_layer);
                    cur_node->ref_count.fetch_sub(1, MOR);
                    std::this_thread::yield();
                    goto insert_retry;
                }
            }

            if (cur_layer != 0) {
                // It is non-bottom layer^, so go down.
                break;
            }

            // It is bottom layer, so insertion succeeded
            // Need to change prev/next nodes' prev/next pointers
            // from 0 to top_layer.
            for (layer = 0; layer <= top_layer; ++layer) {
                __sl_write_lock_an(prevs[layer]);
                node* exp = nexts[layer];
                if (!prevs[layer]->next[layer].compare_exchange_weak(exp, x)) {
                    assert(false);
                }
                __sl_write_unlock_an(prevs[layer]);
            }

            // Node is fully linked.
            x->is_fully_linked.store(true, MOR);

            // Removing next nodes allowed.
            __sl_write_unlock_an(x);

            slist->num_entries.fetch_add(1, MOR);
            slist->layer_entries[x->top_layer].fetch_add(1, MOR);
            for (size_t ii = slist->max_layer - 1;
                    ii <= slist->max_layer - 1; --ii) {
                if (slist->layer_entries[ii] > 0) {
                    slist->top_layer = ii;
                    break;
                }
            }

            // Modification has been done for all layers
            __sl_clr_flags(prevs, 0, top_layer);
            cur_node->ref_count.fetch_sub(1, MOR);

            return 0;
        } while (cur_node != &slist->tail);
    }
    return 0;
}

int skiplist_insert(raw* slist, node* node) {
    return __sl_insert(slist, node, false);
}

int skiplist_insert_nodup(raw* slist, node* node) {
    return __sl_insert(slist, node, true);
}

typedef enum {
    SM   = -2,
    SMEQ = -1,
    EQ   =  0,
    GTEQ =  1,
    GT   =  2
} __sl_find_mode;

// __sl_find increases the `ref_count` of returned node as a result of working
// with it
// Caller is responsible to decrease it.
static inline node* __sl_find(raw* slist, node* query, __sl_find_mode mode) {
find_retry:
    int cmp = 0;
    size_t cur_layer = 0;
    node* cur_node = &slist->head;
    cur_node->ref_count.fetch_add(1, MOR);

    uint8_t sl_top_layer = slist->top_layer;
    for (cur_layer = sl_top_layer; cur_layer <= sl_top_layer; --cur_layer) {
        do {
            node* next_node = __sl_next(slist, cur_node, cur_layer,
                nullptr, nullptr);
            if (next_node == nullptr) {
                cur_node->ref_count.fetch_sub(1, MOR);
                std::this_thread::yield();
                goto find_retry;
            }
            cmp = __sl_cmp(slist, query, next_node);
            if (cmp > 0) {
                // if (cur_node < next_node < node) then move to next node.
                node* temp = cur_node;
                cur_node = next_node;
                temp->ref_count.fetch_sub(1, MOR);
                continue;
            } else {
                if (-1 <= mode && mode <= 1 && cmp == 0) {
                    // if (cur_node < query == next_node) return.
                    cur_node->ref_count.fetch_sub(1, MOR);
                    return next_node;
                }
            }

            // Otherwise (cur_node < query < next_node)
            if (cur_layer != 0) {
                // It is non-bottom layer then go down
                next_node->ref_count.fetch_sub(1, MOR);
                break;
            }

            // It is bottom layer
            if (mode < 0 && cur_node != &slist->head) {
                // Set smaller mode
                next_node->ref_count.fetch_sub(1, MOR);
                return cur_node;
            } else {
                if (mode > 0 && next_node != &slist->tail) {
                    // Set greater mode
                    cur_node->ref_count.fetch_sub(1, MOR);
                    return next_node;
                }
            }

            // Otherwise sets exact match mode or it is not found
            cur_node->ref_count.fetch_sub(1, MOR);
            next_node->ref_count.fetch_sub(1, MOR);
            return nullptr;
        } while (cur_node != &slist->tail);
    }

    return nullptr;
}

node* skiplist_find(raw* slist, node* query) {
    return __sl_find(slist, query, EQ);
}

node* skiplist_find_smaller(raw* slist, node* query) {
    return __sl_find(slist, query, SM);
}

node* skiplist_find_smaller_or_equal(raw* slist, node* query) {
    return __sl_find(slist, query, SMEQ);
}

node* skiplist_find_greater(raw* slist, node* query) {
    return __sl_find(slist, query, GT);
}

node* skiplist_find_greater_or_equal(raw* slist, node* query) {
    return __sl_find(slist, query, GTEQ);
}

int skiplist_erase_node_passive(raw* slist, node* x) {
    size_t top_layer = x->top_layer;
    bool removed = false;
    bool is_fully_linked = false;

    removed = x->removed.load(MOR);
    if (removed) {
        // It is already removed.
        return -1;
    }

    node* prevs[MAX_LAYER];
    node* nexts[MAX_LAYER];

    bool exp = false;
    if (!x->being_modified.compare_exchange_weak(exp, true)) {
        // It is already modified and we cannot work on this node for now.
        return -2;
    }

    // Removed flag is set first, so reader cannot read this node.
    x->removed.store(true, MOR);

erase_node_retry:
    is_fully_linked = x->is_fully_linked.load(MOR);
    if (!is_fully_linked) {
        // It is already unlinked then remove is done by other thread.
        x->removed.store(false, MOR);
        x->being_modified.store(false, MOR);
        return -3;
    }

    int cmp = 0;
    bool found_node_to_erase = false;
    node* cur_node = &slist->head;
    cur_node->ref_count.fetch_add(1, MOR);

    size_t cur_layer = slist->top_layer;
    for (; cur_layer <= slist->top_layer; --cur_layer) {
        do {
            bool node_found = false;
            node* next_node = __sl_next(slist, cur_node, cur_layer,
                x, &node_found);
            if (next_node == nullptr) {
                __sl_clr_flags(prevs, cur_layer + 1, top_layer);
                cur_node->ref_count.fetch_sub(1, MOR);
                std::this_thread::yield();
                goto erase_node_retry;
            }

            // Should find exact position of the `node` unlike the insert()
            cmp = __sl_cmp(slist, x, next_node);
            if (cmp > 0 || (cur_layer <= top_layer && !node_found)) {
                node* temp = cur_node;
                cur_node = next_node;
                if (cmp > 0) {
                    // if (cur_node < next_node < node) then move to next node.
                    int cmp2 = __sl_cmp(slist, cur_node, x);
                    if (cmp2 > 0) {
                        // Otherwise (node < cur_node <= next_node),
                        // so it is not found.
                        __sl_clr_flags(prevs, cur_layer + 1, top_layer);
                        temp->ref_count.fetch_sub(1, MOR);
                        next_node->ref_count.fetch_sub(1, MOR);
                        assert(false);
                    }
                }
                temp->ref_count.fetch_sub(1, MOR);
                continue;
            } else {
                // Otherwise (cur_node <= node <= next_node)
                next_node->ref_count.fetch_sub(1, MOR);
            }

            if (cur_layer <= top_layer) {
                prevs[cur_layer] = cur_node;
                // 'next_node' and 'node' could not be the same,
                //  as 'removed' flag is already set.
                assert(next_node != x);
                nexts[cur_layer] = next_node;

                // Check for the duplication of prev node with upper layer
                int error_code = 0;
                size_t locked_layer = cur_layer + 1;
                if (cur_layer < top_layer &&
                        prevs[cur_layer] == prevs[cur_layer + 1]) {
                    // It is already duplicated with upper layer,
                    // so 'being_modified' flag is true
                    // then do nothing.
                } else {
                    exp = false;
                    if (prevs[cur_layer]->being_modified.compare_exchange_weak(
                            exp, true)) {
                        locked_layer = cur_layer;
                    } else {
                        error_code = -1;
                    }
                }

                if (error_code == 0 && !__sl_valid_prev_next(prevs[cur_layer],
                        nexts[cur_layer])) {
                    error_code = -2;
                }

                if (error_code != 0) {
                    __sl_clr_flags(prevs, locked_layer, top_layer);
                    cur_node->ref_count.fetch_sub(1, MOR);
                    std::this_thread::yield();
                    goto erase_node_retry;
                }

                node* next_node_again = __sl_next(slist, cur_node, cur_layer,
                    x, nullptr);
                next_node_again->ref_count.fetch_sub(1, MOR);
                if (next_node_again != nexts[cur_layer]) {
                    // `next` pointer has been already changed,
                    // so must retry.
                    __sl_clr_flags(prevs, cur_layer, top_layer);
                    cur_node->ref_count.fetch_sub(1, MOR);
                    std::this_thread::yield();
                    goto erase_node_retry;
                }
            }
            if (cur_layer == 0) {
                found_node_to_erase = true;
            }
            break;
        } while (cur_node != &slist->tail);
    }

    // It is not exist in the skiplist, it should not happen.
    assert(found_node_to_erase);

    // It is bottom layer, so removal succeeded
    // Mark this node unlinked.
    __sl_write_lock_an(x); {
        x->is_fully_linked.store(false, MOR);
    } __sl_write_unlock_an(x);

    // Need to change prev nodes' next pointers from 0 to top_layer.
    for (cur_layer = 0; cur_layer <= top_layer; ++cur_layer) {
        __sl_write_lock_an(prevs[cur_layer]);
        node* exp = x;
        assert(exp != nexts[cur_layer]);
        assert(nexts[cur_layer]->is_fully_linked);
        if (!prevs[cur_layer]->next[cur_layer].compare_exchange_weak(
                exp, nexts[cur_layer])) {
            assert(false);
        }
        assert(nexts[cur_layer]->top_layer >= cur_layer);
        __sl_write_unlock_an(prevs[cur_layer]);
    }

    slist->num_entries.fetch_sub(1, MOR);
    slist->layer_entries[x->top_layer].fetch_sub(1, MOR);
    for (size_t ii = slist->max_layer - 1; ii <= slist->max_layer - 1; --ii) {
        if (slist->layer_entries[ii] > 0) {
            slist->top_layer = ii;
            break;
        }
    }

    // Modification has been done for all layers.
    __sl_clr_flags(prevs, 0, top_layer);
    cur_node->ref_count.fetch_sub(1, MOR);

    x->being_modified.store(false, MOR);
    return 0;
}

int skiplist_erase_node(raw* slist, node* node) {
    int res = 0;
    do {
        res = skiplist_erase_node_passive(slist, node);
        // if ret == -2, so other thread is accessing the same node
        // at the same time
        // Need to try again.
    } while (res == -2);
    return res;
}

int skiplist_erase(raw* slist, node* query) {
    node* found = skiplist_find(slist, query);
    if (found == nullptr) {
        // Key is not found
        return -4;
    }

    int res = 0;
    do {
        res = skiplist_erase_node_passive(slist, found);
        // if ret == -2, so other thread is accessing the same node
        // at the same time
        // Need to try again.
    } while (res == -2);

    found->ref_count.fetch_sub(1, MOR);
    return res;
}

int is_valid_node(node* node) {
    return __sl_valid_node(node);
}

int is_safe_to_free(node* node) {
    if (node->accessing_next != 0) {
        return 0;
    }
    if (node->being_modified) {
        return 0;
    }
    if (!node->removed) {
        return 0;
    }

    if (node->ref_count.load(MOR) != 0) {
        return 0;
    }
    return 1;
}

void wait_for_free(node* node) {
    while (!is_safe_to_free(node)) {
        std::this_thread::yield();
    }
}

void grab_node(node* node) {
    node->ref_count.fetch_add(1, MOR);
}

void release_node(node* node) {
    assert(node->ref_count != 0);
    node->ref_count.fetch_sub(1, MOR);
}

node* skiplist_next(raw* slist, node* x) {
    node* next = __sl_next(slist, x, 0, nullptr, nullptr);
    if (next == nullptr) {
        next = __sl_find(slist, x, GT);
    }

    if (next == &slist->tail) {
        return nullptr;
    }
    return next;
}

node* skiplist_prev(raw* slist, node* x) {
    node* prev = __sl_find(slist, x, SM);
    if (prev == &slist->head) {
        return nullptr;
    }
    return prev;
}

node* skiplist_begin(raw* slist) {
    node* next = nullptr;
    while (next == nullptr) {
        next = __sl_next(slist, &slist->head, 0, nullptr, nullptr);
    }
    if (next == &slist->tail) {
        return nullptr;
    }
    return next;
}

node* skiplist_end(raw* slist) {
    return skiplist_prev(slist, &slist->tail);
}

}  // namespace skip_list

// TODO(trmigor): compare_exchange_weak: is MOR needed?
// TODO(puchkovki): RESOLVE WARNINGS!!!
