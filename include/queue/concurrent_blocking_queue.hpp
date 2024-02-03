
#include <algorithm>
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
        return this->TryPush(std::forward<V>(v));        
    }

    template <typename V,
              typename = typename std::enable_if<
                std::is_nothrow_constructible<T, V&&>::value>::type>
    bool Push(V&& v) noexcept {
        int tries = tries_;
        while (true) {
            tries --;
            if (this->TryPush(std::forward<V>(v))) {
                if (block_consumer_) pop_cond_.notify_all();
                return true;
            } else if (wait_up_exited_) return false;
            if (!tries) {
                {
                    std::unique_lock<std::mutex> lock(push_mutex_);
                    block_producer_ ++;
                    push_cond_.wait(lock, [&]{ 
                         return !this->IsFull() || wait_up_exited_; 
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
            } else if (wait_up_exited_) return false;
            if (!tries) {
                {
                    std::unique_lock<std::mutex> lock(pop_mutex_);
                    block_consumer_ ++;
                    pop_cond_.wait(lock, [&]{ 
                        return !this->Empty() || wait_up_exited_; 
                                   });
                    block_consumer_ --;
                }
                tries = tries_;
            }
        }
    }

    void Wait() noexcept {
        wait_up_exited_ = true;
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
            if (consumer == 0) break;
        }
    }

private:
    std::mutex push_mutex_, pop_mutex_;
    std::condition_variable push_cond_, pop_cond_;
    int tries_;
    int block_consumer_ = 0;
    int block_producer_ = 0;
    volatile bool wait_up_exited_ = false;
};

} // namespace concurrent
} // namespace morles

