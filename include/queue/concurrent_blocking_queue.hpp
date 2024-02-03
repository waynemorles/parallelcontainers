
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <type_traits>
#include "queue/concurrent_ring_queue.hpp"

namespace morles {
namespace concurrent {

template <typename T, typename Allocator = std::allocator<T>>
class ConcurrentBlockingQueue : protected ConcurrentRingQueue<T, Allocator> {

public:
    ConcurrentBlockingQueue(int capacity, int tries) 
        : ConcurrentRingQueue<T,Allocator>(capacity),tries_(tries) {}
    virtual ~ConcurrentBlockingQueue() {}
    
    template <typename V,
              typename = typename std::enable_if<
                std::is_nothrow_constructible<T, V&&>::value>::type>
    bool TryPushQ(V&& v) noexcept {
        if (wait_up_exited_.load(std::memory_order_acquire)) return false;
        return this->TryPush(std::forward<V>(v));        
    }

    template <typename V,
              typename = typename std::enable_if<
                std::is_nothrow_constructible<T, V&&>::value>::type>
    bool Push(V&& v) noexcept {
        int tries = tries_;
        while (true) {
            tries --;
            if (wait_up_exited_.load(std::memory_order_acquire)) return false;
            if (this->TryPush(std::forward<V>(v))) {
                if (block_consumer_) pop_cond_.notify_all();
                return true;
            } 
            if (!tries) {
                {
                    std::unique_lock<std::mutex> lock(push_mutex_);
                    block_producer_ ++;
                    push_cond_.wait(lock, [&]{ 
                         return !this->IsFull() || wait_up_exited_.load(std::memory_order_acquire); 
                                    });
                    block_producer_ --;
                }
                tries = tries_;
            }
        }
    }

    bool TryPopQ(T& v) noexcept {
        return this->TryPop(v);
    }

    bool Pop(T& v) noexcept {
        int tries = tries_;
        while (true) {
            tries --;
            if (this->TryPop(v)) {
                if (block_producer_) push_cond_.notify_all();
                return true;
            } else if (wait_up_exited_.load(std::memory_order_relaxed)) return false;
            if (!tries) {
                {
                    std::unique_lock<std::mutex> lock(pop_mutex_);
                    block_consumer_ ++;
                    pop_cond_.wait(lock, [&]{ 
                        return !this->Empty() || wait_up_exited_.load(std::memory_order_relaxed); 
                                   });
                    block_consumer_ --;
                }
                tries = tries_;
            }
        }
    }

    // Wait until the queue is empty and exit
    void Wait() noexcept {
        wait_up_exited_.store(true, std::memory_order_release);
        int producer, consumer;
        while (true) {
            {
                std::unique_lock<std::mutex> lock(push_mutex_);
                producer = block_producer_;
            }
            if (producer > 0) push_cond_.notify_all();
            if (producer == 0) break;
        }
        while (true) {
            {
                std::unique_lock<std::mutex> lock(pop_mutex_);
                consumer = block_consumer_;
            }
            if (consumer > 0) pop_cond_.notify_all();
            if (consumer == 0) {
                if (this->Empty()) break;
            }
        }
    }

private:
    std::mutex push_mutex_, pop_mutex_;
    std::condition_variable push_cond_, pop_cond_;
    int tries_;
    int block_consumer_ = 0;
    int block_producer_ = 0;
    std::atomic<bool> wait_up_exited_{false};
};

} // namespace concurrent
} // namespace morles

