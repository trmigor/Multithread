#ifndef SKIP_LIST_INCLUDE_SKIP_LIST_HPP_
#define SKIP_LIST_INCLUDE_SKIP_LIST_HPP_

#include <atomic>
#include <cstdint>
#include <functional>
#include <iostream>
#include <stdexcept>

namespace skip_list {

#define MAX_LAYER 64
#define MOR std::memory_order_relaxed

#ifndef __get_entry
#define __get_entry(ELEM, STRUCT, MEMBER) \
    (reinterpret_cast<STRUCT*> \
    (reinterpret_cast<uint8_t *>(ELEM) - offsetof(STRUCT, MEMBER)))
#endif

struct node;

typedef std::atomic<node*>     atm_node_ptr;
typedef std::atomic<bool>      atm_bool;
typedef std::atomic<uint8_t>   atm_uint8_t;
typedef std::atomic<uint16_t>  atm_uint16_t;
typedef std::atomic<uint32_t>  atm_uint32_t;

struct node {
    atm_node_ptr*   next;
    atm_bool        is_fully_linked;
    atm_bool        being_modified;
    atm_bool        removed;
    uint8_t         top_layer;
    atm_uint16_t    ref_count;
    atm_uint32_t    accessing_next;
};

typedef int cmp_t(node* a, node* b, void* aux);

struct raw_config {
    size_t  fanout;
    size_t  maxLayer;
    void*   aux;
};

struct raw {
    node            head;
    node            tail;
    cmp_t*          cmp_func;
    void*           aux;
    atm_uint32_t    num_entries;
    atm_uint32_t*   layer_entries;
    atm_uint8_t     top_layer;
    uint8_t         fanout;
    uint8_t         max_layer;
};

void skiplist_init(raw* slist, cmp_t* cmp_func);
void skiplist_free(raw* slist);

void init_node(node* node);
void free_node(node* node);

size_t get_size(raw* slist);

raw_config get_default_config();
raw_config get_config(raw* slist);

void set_config(raw* slist, raw_config config);

int skiplist_insert(raw* slist, node* node);
int skiplist_insert_nodup(raw* slist, node* node);

node* skiplist_find(raw *slist, node* query);
node* skiplist_find_smaller(raw* slist, node* query);
node* skiplist_find_smaller_or_equal(raw* slist, node* query);
node* skiplist_find_greater(raw* slist, node* query);
node* skiplist_find_greater_or_equal(raw* slist, node* query);

int skiplist_erase_node_passive(raw* slist, node* node);
int skiplist_erase_node(raw* slist, node* node);
int skiplist_erase(raw* slist, node* query);

int is_valid_node(node* node);
int is_safe_to_free(node* node);
void wait_for_free(node* node);

void grab_node(node* node);
void release_node(node* node);

node* skiplist_next(raw* slist, node* node);
node* skiplist_prev(raw* slist, node* node);
node* skiplist_begin(raw* slist);
node* skiplist_end(raw* slist);

const std::out_of_range ExceptIterator(
    "list: Trying to get data by invalid iterator"
);

template<class T>
class skiplist {
    public:
        typedef T value_type;

        typedef value_type&         reference;
        typedef const value_type&   const_reference;

        typedef value_type*         pointer;
        typedef const value_type*   const_pointer;

        typedef size_t              size_type;
        typedef ptrdiff_t           difference_type;

        struct __node {
            node snode;
            size_t key;
            value_type value;
        };

        class iterator {
            public:
                iterator(void) = delete;

                explicit iterator(raw* slist, __node* cur) :
                    slist_(slist), cur_(cur) {}

                ~iterator() {
                    if (cur_ != nullptr) {
                        cur_->snode.ref_count.fetch_sub(1, MOR);
                    }
                }

                iterator operator++() {
                    if (cur_ == nullptr) {
                        return *this;
                    }
                    node* tmp = skiplist_next(slist_, &cur_->snode);
                    if (cur_->snode.ref_count != 0) {
                        cur_->snode.ref_count.fetch_sub(1, MOR);
                    }
                    cur_ = __get_entry(tmp, __node, snode);
                    return *this;
                }

                bool operator==(const iterator& other) {
                    return cur_ == other.cur_;
                }

                bool operator!=(const iterator& other) {
                    return !(cur_ == other.cur_);
                }

                reference operator*() const {
                    if (cur_ == nullptr) {
                        throw ExceptIterator;
                    }
                    return cur_->value;
                }

                __node* __get_node() {
                    return cur_;
                }

            private:
                raw* slist_;
                __node* cur_;
        };

        class const_iterator {
            public:
                const_iterator(void) = delete;

                explicit const_iterator(raw* slist, __node* cur) :
                    slist_(slist), cur_(cur) {}

                ~const_iterator() {
                    if (cur_ != nullptr) {
                        cur_->snode.ref_count.fetch_sub(1, MOR);
                    }
                }

                iterator operator++() {
                    if (cur_ == nullptr) {
                        return *this;
                    }
                    node* tmp = skiplist_next(slist_, &cur_->snode);
                    if (cur_->snode.ref_count != 0) {
                        cur_->snode.ref_count.fetch_sub(1, MOR);
                    }
                    cur_ = __get_entry(tmp, __node, snode);
                    return *this;
                }

                bool operator==(const iterator& other) {
                    return cur_ == other.cur_;
                }

                bool operator!=(const iterator& other) {
                    return !(cur_ == other.cur_);
                }

                const_reference operator*() const {
                    if (cur_ == nullptr) {
                        throw ExceptIterator;
                    }
                    return cur_->value;
                }

                const __node* __get_node() {
                    return cur_;
                }

            private:
                raw* slist_;
                __node* cur_;
        };

        skiplist(void) {
            slist_ = new raw;
            skiplist_init(slist_, __cmp);
        }

        skiplist(const skiplist& other) = delete;

        ~skiplist(void) {
            node* cursor = skiplist_begin(slist_);
            while (cursor != nullptr) {
                __node* entry = __get_entry(cursor, __node, snode);
                cursor = skiplist_next(slist_, cursor);

                skiplist_erase_node(slist_, &entry->snode);
                release_node(&entry->snode);
                wait_for_free(&entry->snode);
                free_node(&entry->snode);

                delete entry;
            }

            skiplist_free(slist_);
            delete slist_;
        }

        size_type size() {
            return get_size(slist_);
        }

        bool empty() {
            return (size() == 0);
        }

        int insert(const_reference x) {
            __node* to_ins = new __node;
            init_node(&to_ins->snode);
            to_ins->key = hash_(x);
            to_ins->value = x;
            return skiplist_insert_nodup(slist_, &to_ins->snode);
        }

        int erase(const_reference x) {
            iterator found = find(x);
            if (found == end()) {
                return -1;
            }

            __node* to_erase = found.__get_node();
            int done = skiplist_erase_node(slist_, &to_erase->snode);
            if (done != 0) {
                return done;
            }

            release_node(&to_erase->snode);
            wait_for_free(&to_erase->snode);
            free_node(&to_erase->snode);

            return 0;
        }

        iterator find(const_reference x) {
            size_t key = hash_(x);
            __node query;
            query.key = key;
            bool done = false;
            node* cursor;
            __node* found;
            while (!done) {
                cursor = skiplist_find_greater_or_equal(slist_, &query.snode);
                if (cursor == nullptr || cursor == &slist_->tail) {
                    return end();
                }
                found = __get_entry(cursor, __node, snode);
                // if (found->value == x) {
                    done = true;
                // }
            }
            return iterator(slist_, found);
        }

        const_iterator find(const value_type& x) const {
            size_t key = hash_(x);
            __node query;
            query.key = key;
            bool done = false;
            node* cursor;
            __node* found;
            while (!done) {
                cursor = skiplist_find_greater_or_equal(slist_, &query.snode);
                if (cursor == nullptr || cursor == &slist_->tail) {
                    return cend();
                }
                found = __get_entry(cursor, __node, snode);
                if (found->value == x) {
                    done = true;
                }
            }
            return const_iterator(slist_, found);
        }

        iterator begin() {
            node* begin = skiplist_begin(slist_);
            __node* tmp = __get_entry(begin, __node, snode);
            return iterator(slist_, tmp);
        }

        const_iterator begin() const {
            node* begin = skiplist_begin(slist_);
            __node* tmp = __get_entry(begin, __node, snode);
            return const_iterator(slist_, tmp);
        }

        const_iterator cbegin() const {
            node* begin = skiplist_begin(slist_);
            __node* tmp = __get_entry(begin, __node, snode);
            return const_iterator(slist_, tmp);
        }

        iterator end() {
            node* end = skiplist_end(slist_);
            __node* tmp = __get_entry(end, __node, snode);
            iterator res(slist_, tmp);
            ++res;
            return res;
        }

        const_iterator end() const {
            node* end = skiplist_end(slist_);
            __node* tmp = __get_entry(end, __node, snode);
            const_iterator res(slist_, tmp);
            ++res;
            return res;
        }

        const_iterator cend() const {
            node* end = skiplist_end(slist_);
            __node* tmp = __get_entry(end, __node, snode);
            const_iterator res(slist_, tmp);
            res++;
            return res;
        }

    private:
        raw* slist_;
        std::hash<value_type> hash_;

        static int __cmp(node* a, node* b, void* aux) {
            __node* aa;
            __node* bb;
            aa = __get_entry(a, __node, snode);
            bb = __get_entry(b, __node, snode);

            if (aa->key < bb->key) {
                return -1;
            }
            if (aa->key > bb->key) {
                return 1;
            }
            return 0;
        }
};

}  // namespace skip_list

#endif  // SKIP_LIST_INCLUDE_SKIP_LIST_HPP_
