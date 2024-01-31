
#include <atomic>
#include <iostream>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include "queue/cocurrent_ring_queue.hpp"

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

namespace morles {
namespace concurrent {

TEST(ConcurrentRingQueue, PushSyncConsumeSync) {
    ConcurrentRingQueue<int> queue(1024);
    for (int i=0; i<1024; ) {
        if (queue.TryPush(i)) {
        } else {
            queue.Push(i);
        }
        i++;
    }

    for (int i=0; i<1024; i++) {
        int j;
        queue.Pop(j);
        ASSERT_EQ(j, i);
    }
}

TEST(ConcurrentRingQueue, pushParllelConsumeSync) {
    ConcurrentRingQueue<int> queue(256);
    static constexpr int TZ = 8;
    std::thread threads[TZ];
    for (int i=0; i<TZ; i++) {
        threads[i] = std::thread([&, i](){
                                 for (int j=0; j<1024; j++) {
                                    queue.Push(j*i);
                                 }
                                 });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    int counter = 0;
    std::thread consume([&]() {
                        for (int i=0; i<TZ*1024; i++) {
                            int val;
                            queue.Pop(val);
                            counter ++;
                        }    
                        });
    for (int i=0; i<TZ; i++) {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }
    if (consume.joinable()) consume.join();
    ASSERT_TRUE(queue.Empty());
    ASSERT_EQ(counter, TZ*1024);
}

TEST(ConcurrentRingQueue, pushSyncConsumeParllel) {
    ConcurrentRingQueue<int> queue(256);
    static constexpr int TZ = 4;
    static constexpr int COUNTER_PER_THREAD = 10000000;
    std::thread producer([&](){
                             for (int j=0; j<COUNTER_PER_THREAD*TZ; j++) {
                                queue.Push(j);
                             }
                             });

    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    std::atomic_int counter = COUNTER_PER_THREAD*TZ;

    std::thread consumer[TZ];
    for (int i=0; i<TZ; i++) {
        consumer[i] = std::thread([&]() {
                            while(counter.load(std::memory_order_relaxed)) {
                                int val;
                                if(queue.TryPop(val)) {
                                    counter --;
                                }
                            }    
                            });
    }
    if (producer.joinable()) producer.join();
    for (int i=0; i<TZ; i++) {
        if (consumer[i].joinable()) {
            consumer[i].join();
        }
    }
    ASSERT_TRUE(queue.Empty());
}

TEST(ConcurrentRingQueue, pushParllelConsumeParrllel) {
    ConcurrentRingQueue<int> queue(2048);
    static constexpr int TZ = 4;
    static constexpr int COUNTER_PER_THREAD = 10000000;
    std::thread producer[TZ];
    for (int i=0; i<TZ; i++) {
        producer[i] = std::thread([&, i](){
                                 for (int j=0; j<COUNTER_PER_THREAD; j++) {
                                    queue.Push(j*i);
                                 }
                                 });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    std::atomic_int counter = COUNTER_PER_THREAD*TZ;

    std::thread consumer[TZ];
    for (int i=0; i<TZ; i++) {
        consumer[i] = std::thread([&]() {
                            while(counter.load(std::memory_order_relaxed)) {
                                int val;
                                if(queue.TryPop(val)) {
                                    counter --;
                                }
                            }    
                            });
    }
    for (int i=0; i<TZ; i++) {
        if (producer[i].joinable()) {
            producer[i].join();
        }
        if (consumer[i].joinable()) {
            consumer[i].join();
        }
    }
    ASSERT_TRUE(queue.Empty());
}

} // namespace concurrent
} // namespace morles

