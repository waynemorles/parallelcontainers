
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
    bool TryPush(V&& v) noexcept {
        return this->TryPush(v);        
    }

    template <typename V,
              typename = typename std::enable_if<
                std::is_nothrow_constructible<T, V&&>::value>::type>
    void Push(V&& v) noexcept {
        int tries = tries_;
        while (true) {
            tries --;
            if (this->TryPush(v)) {
                if (block_consumer) pop_cond_.notify_all();
                return;
            }
            if (!tries) {
                {
                    std::unique_lock<std::mutex> lock(push_mutex_);
                    block_producer ++;
                    push_cond_.wait(lock, [&]{ 
                         return !this->IsFull(); 
                                    });
                    block_producer --;
                }
                tries = tries_;
            }
        }
    }

    bool TryPop(T& v) noexcept {
        return this->TryPop(v);
    }

    void Pop(T& v) noexcept {
        int tries = tries_;
        while (true) {
            tries --;
            if (this->TryPop(v)) {
                if (block_producer) push_cond_.notify_all();
                return;
            }
            if (!tries) {
                {
                    std::unique_lock<std::mutex> lock(pop_mutex_);
                    block_consumer ++;
                    pop_cond_.wait(lock, [&]{ 
                        return !this->Empty(); 
                                   });
                    block_consumer --;
                }
                tries = tries_;
            }
        }
    }

private:
    std::mutex push_mutex_, pop_mutex_;
    std::condition_variable push_cond_, pop_cond_;
    int tries_;
    int block_consumer = 0;
    int block_producer = 0;
};

}
}

