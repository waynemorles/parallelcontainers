
#include <atomic>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>

namespace morles {
namespace concurrent {

class SpinWait {
public:
    static constexpr std::int32_t yield_limit = 16;
    SpinWait() {}
    void Wait() {
        if (counter_ <= yield_limit) {
            int delay = counter_;
            while (delay-- > 0) { 
                asm("yield" ::: "memory");
                //_mm_pause(); 
            }    
            counter_ *= 2;
        } else {
            std::this_thread::yield();
        }
    }
    
private:
    int counter_ = 1;
};

template<typename Allocator>
class queue_allocator_traits : public std::allocator_traits<Allocator> {
public:
    using base_type = std::allocator_traits<Allocator>;
};

template<typename T, typename Allocator = std::allocator<T>>
class ConcurrentRingQueue {

    struct Item {
        Item() {}
        ~Item() {}
        std::atomic<uint64_t> position_;
        T data_;
    };

    static constexpr int32_t CACHELINE_SIZE = 64;
    using data_traits = queue_allocator_traits<Allocator>;
    using item_allocator_type = typename data_traits::template rebind_alloc<Item>;
    using item_allocator_traits = queue_allocator_traits<item_allocator_type>;

public:
    ConcurrentRingQueue(size_t capacity) : capacity_(capacity), alloc_(Allocator()) {
        if (capacity_>=2 && (capacity_&(capacity_-1))) {
            throw std::invalid_argument("queue size must be a power of 2");
        }
        base_ = new Item[capacity_];
        for (int i=0; i<capacity_; i++) {
            base_[i].position_.exchange(i);
        }
    }

    virtual ~ConcurrentRingQueue() { if (base_) delete[] base_; }
    ConcurrentRingQueue(const ConcurrentRingQueue&) = delete;
    ConcurrentRingQueue& operator=(const ConcurrentRingQueue&) = delete;

    template <typename... Args>
    bool TryPush(Args&& ... args) {
        uint64_t head = head_.load(std::memory_order_relaxed);
        SpinWait wait;
        while (true) {
            uint64_t slot = head & (capacity_ - 1);
            Item* item = &base_[slot];
            uint64_t pos = item->position_.load(std::memory_order_acquire);
            // head point to the newest allocated item, and allocate new item
            if (head == pos) {
                if (head_.compare_exchange_weak(head, head+1)) {
                    data_traits::construct(alloc_, &item->data_, args...);
                    item->position_.store(head + 1, std::memory_order_release);
                    return true;
                } else {
                    // conflict and wait for a while
                    wait.Wait();
                }
            }
            if (head > pos) return false;
            if (head < pos) head = head_.load(std::memory_order_acquire);
        }
    }

    template <typename V,
              typename = typename std::enable_if<
                std::is_nothrow_constructible<T, V&&>::value>::type>
    void Push(V&& v) noexcept {
       SpinWait wait;
       while(!TryPush(std::forward<V>(v))) { wait.Wait(); }
    }

    bool TryPop(T& v) {
        uint64_t tail = tail_.load(std::memory_order_relaxed);
        SpinWait wait;
        while (true) {
            uint64_t slot = tail & (capacity_ - 1);
            Item* item = &base_[slot];
            uint64_t pos = item->position_.load(std::memory_order_acquire);
            if (pos == tail + 1) {
                if (tail_.compare_exchange_weak(tail, tail + 1)) {
                    v = std::move(item->data_);
                    item->position_.store(tail + capacity_, std::memory_order_release);
                    return true;
                } else {
                    // conflict and wait for a while
                    wait.Wait();
                }
            }
            if (pos < tail + 1) return false;
            if (pos > tail + 1) tail = tail_.load(std::memory_order_acquire);
        }
    }
    
    void Pop(T& v) noexcept {
       SpinWait wait;
       while(!TryPop(v)) { wait.Wait(); }
    } 

    int Capacity() const { return capacity_; }
    bool Empty() const {
        uint64_t h = head_.load(std::memory_order_acquire);
        uint64_t t = tail_.load(std::memory_order_acquire);
        return h == t;
    }
    bool IsFull() const {
        uint64_t h = head_.load(std::memory_order_acquire);
        uint64_t t = tail_.load(std::memory_order_acquire);
        return (int)(h - t) == capacity_;
    }

private:
    int capacity_;
    Allocator alloc_;
    Item* base_;
    alignas(CACHELINE_SIZE) std::atomic<uint64_t> head_{0};
    alignas(CACHELINE_SIZE) std::atomic<uint64_t> tail_{0};
};

} // namespace concurrent
} // namespace morles
