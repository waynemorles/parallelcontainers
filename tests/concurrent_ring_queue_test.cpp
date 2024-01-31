
#include <gtest/gtest.h>
#include "queue/cocurrent_ring_queue.hpp"

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

namespace morles {
namespace concurrent {

TEST(ConcurrentRingQueue, push) {
    ConcurrentRingQueue<int> queue(64);
    queue.push(100);
    int j;
    queue.pop(j);
    ASSERT_EQ(j, 100);
}

} // namespace concurrent
} // namespace morles

