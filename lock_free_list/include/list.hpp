#ifndef LOCK_FREE_LIST_INCLUDE_LIST_HPP_
#define LOCK_FREE_LIST_INCLUDE_LIST_HPP_

#include <unistd.h>
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace list {

const std::out_of_range ExceptEmptyList(
    "list: List is empty"
);

const std::out_of_range ExceptIterator(
    "list: Trying to get data by invalid iterator"
);

const std::length_error ExceptHazard(
    "list: Too much hazard pointers for this thread"
);

const std::invalid_argument ExceptNullPtr(
    "list: Null pointer argument"
);

template <class T>
struct __list_node {
    T value;
    std::atomic<__list_node*> next;
};

template <class T>
class __list_iterator {
    public:
        typedef T value_type;

        typedef value_type& reference;

        __list_iterator() = delete;

        __list_iterator(__list_node<T>* node, __list_node<T>* tail) :
                node_(node), tail_(tail) {}

        __list_iterator operator++() {
            node_ = node_->next;
            return *this;
        }

        bool operator==(const __list_iterator& other) const {
            return node_ == other.node_;
        }

        bool operator!=(const __list_iterator& other) const {
            return !(node_ == other.node_);
        }

        reference operator*() const {
            if (node_ == tail_) {
                throw ExceptIterator;
            }
            return node_->value;
        }

        __list_node<T>* getNode() {
            return node_;
        }

    private:
        __list_node<T>* node_;
        __list_node<T>* tail_;
};

template <class T>
class __list_const_iterator {
    public:
        typedef T value_type;

        typedef const value_type& const_reference;

        __list_const_iterator() = delete;

        __list_const_iterator(__list_node<T>* node, __list_node<T>* tail) :
                node_(node), tail_(tail) {}

        __list_const_iterator operator++() {
            node_ = node_->next;
            return *this;
        }

        bool operator==(const __list_const_iterator& other) const {
            return node_ == other.node_;
        }

        bool operator!=(const __list_const_iterator& other) const {
            return !(node_ == other.node_);
        }

        const_reference operator*() const {
            if (node_ == tail_) {
                throw ExceptIterator;
            }
            return node_->value;
        }

        const __list_node<T>* getNode() {
            return node_;
        }

    private:
        __list_node<T>* node_;
        __list_node<T>* tail_;
};

template <class T>
class list {
    public:
        typedef T value_type;

        typedef value_type&         reference;
        typedef const value_type&   const_reference;

        typedef value_type*         pointer;
        typedef const value_type*   const_pointer;

        typedef size_t              size_type;
        typedef ptrdiff_t           difference_type;

        typedef __list_iterator<value_type>             iterator;
        typedef __list_const_iterator<value_type>       const_iterator;

    private:
        typedef __list_node<value_type> node;

    public:
        //
        // Constructors
        //

        explicit list(size_t hazard_ptr_allowed = 1) :
                size_(0), num_threads_(1),
                hazard_ptr_allowed_(hazard_ptr_allowed) {
            init_();
        }

        explicit list(size_type n, const value_type& val,
                size_t hazard_ptr_allowed = 1) :
                    size_(n), num_threads_(1),
                    hazard_ptr_allowed_(hazard_ptr_allowed) {
            init_();
            for (size_type i = 0; i < n; ++i) {
                node* curr = new node;
                curr->next = tail_;
                curr->value = val;
            }
        }

        template <class InputIterator>
        list(InputIterator first, InputIterator last,
                size_t hazard_ptr_allowed = 1,
                typename std::enable_if
                <std::__is_input_iterator<InputIterator>::value>::type* = 0) :
                    num_threads_(1),
                    hazard_ptr_allowed_(hazard_ptr_allowed) {
            init_();
            size_t i = 0;
            for (InputIterator it = first; it != last; ++it) {
                node* curr = new node;
                curr->next = tail_;
                curr->value = *it;
                ++i;
            }
            size_ = i;
        }

        list(const list& x) {
            init_();
            size_t i = 0;
            node* start = x.head_->next;
            for (node* next = start; next != x.tail_; next = next->next) {
                node* curr = new node;
                curr->next = tail_;
                curr->value = next->value;
                ++i;
            }
            size_ = i;
            num_threads_ = x.num_threads_;
            hazard_ptr_allowed_ = x.hazard_ptr_allowed_;
            hazard_ptrs_ = x.hazard_ptrs_;
            retire_ = x.retire_;
        }

        list(list&& x) {
            head_ = x.head_;
            tail_ = x.tail_;
            size_.store(x.size_);
            num_threads_ = x.num_threads_;
            hazard_ptr_allowed_ = x.hazard_ptr_allowed_;
            hazard_ptrs_ = x.hazard_ptrs_;
            retire_ = x.retire_;
        }

        list(std::initializer_list<value_type> il,
                size_t num_threads = 1, size_t hazard_ptr_allowed = 1) :
                    num_threads_(num_threads),
                    hazard_ptr_allowed_(hazard_ptr_allowed) {
            init_();
            auto first = il.begin();
            auto last = il.end();
            size_t i = 0;
            for (auto it = first; it != last; ++it) {
                node* curr = new node;
                curr->next = tail_;
                curr->value = *it;
                ++i;
            }
            size_ = i;
        }

        //
        // Assignment
        //

        list& operator=(const list& x) {
            clear();

            size_t i = 0;
            node* start = x.head_->next;
            for (node* next = start; next != x.tail_; next = next->next) {
                node* curr = new node;
                curr->next = tail_;
                curr->value = next->value;
                ++i;
            }
            size_ = i;
            num_threads_ = x.num_threads_;
            hazard_ptr_allowed_ = x.hazard_ptr_allowed_;

            return *this;
        }

        list& operator=(list&& x) {
            clear();

            head_ = x.head_;
            tail_ = x.tail_;
            size_ = x.size_;
            num_threads_ = x.num_threads_;
            hazard_ptr_allowed_ = x.hazard_ptr_allowed_;
            hazard_ptrs_ = x.hazard_ptrs_;
            retire_ = x.retire_;

            return *this;
        }

        list& operator=(std::initializer_list<value_type> il) {
            clear();

            auto first = il.begin();
            auto last = il.end();
            size_t i = 0;
            for (auto it = first; it != last; ++it) {
                node* curr = new node;
                curr->next = tail_;
                curr->value = *it;
                ++i;
            }
            size_ = i;

            return *this;
        }

        //
        // Destructor
        //

        ~list() {
            node* prev = head_;
            for (node* next = head_; next != nullptr; next = next->next) {
                if (next != prev) {
                    delete prev;
                }
                prev = next;
            }
        }

        //
        // Iterators
        //

        void print() {
            std::stringstream s;
            for (node* curr = head_; curr != nullptr; curr = curr->next) {
                if (curr == head_) {
                    s << "head: (" << curr->value << ") ";
                    continue;
                }
                if (curr == tail_) {
                    s << "tail: (" << curr->value << ") ";
                    continue;
                }
                s << curr->value << " ";
            }
            s << std::endl;
            std::cout << s.str();
        }

        iterator begin() {
            return iterator(head_->next, tail_);
        }

        const_iterator begin() const {
            return const_iterator(head_->next, tail_);
        }

        const_iterator cbegin() const noexcept {
            return const_iterator(head_->next, tail_);
        }

        iterator end() {
            return iterator(tail_, tail_);
        }
        const_iterator end() const {
            return const_iterator(tail_, tail_);
        }

        const_iterator cend() const noexcept {
            return const_iterator(tail_, tail_);
        }

        //
        // Capacity
        //

        inline bool empty() const {
            return head_->next == tail_;
        }

        inline size_type size() const {
            return size_;
        }

        //
        // Element access
        //

        inline reference front() {
            if (empty()) {
                throw ExceptEmptyList;
            }
            return head_->next.load(std::memory_order_relaxed)->value;
        }

        inline const_reference front() const {
            if (empty()) {
                throw ExceptEmptyList;
            }
            return head_->next.load(std::memory_order_relaxed)->value;
        }

        //
        // Modifiers
        //

        void push_front(const value_type& val) {
            std::stringstream s;
            s << "Push(" << val << ")"<< std::endl;
            std::cout << s.str();

            node* new_node = new node;
            new_node->value = val;

            bool done = false;
            while (!done) {
                node* next = head_->next;
                new_node->next = next;
                done = head_->next.compare_exchange_weak(next, new_node,
                    std::memory_order_relaxed);
            }

            size_.fetch_add(1, std::memory_order_relaxed);
        }

        void push_front(value_type&& val) {
            std::stringstream s;
            s << "Push(" << val << ")"<< std::endl;
            std::cout << s.str();

            node* new_node = new node;
            new_node->value = val;

            bool done = false;
            while (!done) {
                node* next = head_->next;
                new_node->next = next;
                done = head_->next.compare_exchange_weak(next, new_node,
                    std::memory_order_relaxed);
            }

            size_.fetch_add(1, std::memory_order_relaxed);
        }

        void pop_front() {
            if (empty()) {
                throw ExceptEmptyList;
                return;
            }
            node* curr = head_->next.load(std::memory_order_relaxed);
            bool done = false;
            while (!done) {
                curr = head_->next.load(std::memory_order_relaxed);
                if (curr == tail_) {
                    throw ExceptEmptyList;
                    return;
                }
                node* next = curr->next;
                done = head_->next.compare_exchange_weak(curr, next,
                    std::memory_order_relaxed);
            }
            std::stringstream s;
            s << "Pop(" << curr->value << ")"<< std::endl;
            std::cout << s.str();
            size_.fetch_sub(1, std::memory_order_relaxed);
            retire_node_(curr);
        }

        void swap_first() {
            node* second = head_->next;
            set_hazard(second);
            sleep(5);
            std::swap(head_->value, second->value);
            unset_hazard(second);
        }

        void clear() noexcept {
            node* prev = head_->next;
            for (node* next = prev; next != tail_; next = next->next) {
                if (next != prev) {
                    retire_node_(prev);
                }
                prev = next;
            }
            head_->next = tail_;
            size_.store(0, std::memory_order_relaxed);
        }

        //
        // Thread-safety
        //

        void thread_attach() {
            attach_lock.lock();
            std::thread::id my_id = std::this_thread::get_id();
            hazard_ptrs_.insert(std::pair<std::thread::id, std::vector<node*>>(
                my_id, std::vector<node*>(0)));
            retire_.insert(std::pair<std::thread::id, std::vector<node*>>(
                my_id, std::vector<node*>(0)));
            ++num_threads_;
            attach_lock.unlock();
        }

        void set_hazard(node* which) {
            if (which == nullptr) {
                throw ExceptNullPtr;
                return;
            }
            std::thread::id my_id = std::this_thread::get_id();
            if (hazard_ptrs_[my_id].size() < hazard_ptr_allowed_) {
                hazard_ptrs_[my_id].push_back(which);
            } else {
                throw ExceptHazard;
            }
        }

        void unset_hazard(node* which) {
            if (which == nullptr) {
                throw ExceptNullPtr;
                return;
            }
            std::thread::id my_id = std::this_thread::get_id();
            hazard_ptrs_[my_id].pop_back();
        }

        void reset_num_threads() {
            num_threads_ = 1;
            hazard_ptrs_.clear();
            retire_.clear();
        }

    private:
        node* head_;
        node* tail_;
        std::atomic<size_type> size_;

        size_t num_threads_;
        size_t hazard_ptr_allowed_;
        std::map<std::thread::id, std::vector<node*>> hazard_ptrs_;
        std::map<std::thread::id, std::vector<node*>> retire_;
        std::mutex attach_lock;

        void init_() {
            head_ = new node;
            tail_ = new node;
            head_->next = tail_;
            tail_->next = nullptr;
            head_->value = value_type();
            tail_->value = value_type();
        }

        void scan_() {
            std::vector<node*> all_hazard;
            for (auto e : hazard_ptrs_) {
                std::cout << e.first << std::endl;
                for (auto curr : e.second) {
                    all_hazard.push_back(curr);
                }
            }
            std::sort(all_hazard.begin(), all_hazard.end());

            std::thread::id my_id = std::this_thread::get_id();
            std::vector<node*> my_new_retire;
            for (auto curr : (*retire_.find(my_id)).second) {
                if (std::binary_search(all_hazard.begin(), all_hazard.end(),
                        curr)) {
                    my_new_retire.push_back(curr);
                } else {
                    delete curr;
                }
            }
            retire_[my_id] = my_new_retire;
        }

        void retire_node_(node* x) {
            std::thread::id my_id = std::this_thread::get_id();
            retire_[my_id].push_back(x);

            if (retire_[my_id].size() >=
                    2 * num_threads_ * hazard_ptr_allowed_) {
                scan_();
            }
        }
};

}  // namespace list

#endif  // LOCK_FREE_LIST_INCLUDE_LIST_HPP_
