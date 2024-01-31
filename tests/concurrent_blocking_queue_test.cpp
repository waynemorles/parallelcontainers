
#include <atomic>
#include <iostream>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include "queue/concurrent_blocking_queue.hpp"

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

namespace morles {
namespace concurrent {

TEST(ConcurrentBlockingQueue, test) {
    ConcurrentBlockingQueue<int> queue(1024, 10);
    int counter = 1000000;
    std::thread producer([&](){
                         while (true) {
                         for (int i=0; i<counter; i++) {
                            queue.Push(i);
                         }
                         std::this_thread::sleep_for(std::chrono::seconds(10));
                         }
                         });

    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::thread consumer([&](){
                         while (true) {
                         for (int i=0; i<counter; i++) {
                            int j;
                            queue.Pop(j);
                            std::cout << "Pop " << j << std::endl;
                         }
                         std::this_thread::sleep_for(std::chrono::seconds(10));
                         }
                         });

    producer.join();
    consumer.join();
}

} // namespace concurrent
} // namespace morles

