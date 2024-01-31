
include_directories(${GTEST_INCLUDE_DIR})
set(DEPS_LIBS ${GTEST_LIBRARY} pthread)

add_executable(concurrent_ring_queue_test concurrent_ring_queue_test.cpp)
target_link_libraries(concurrent_ring_queue_test ${DEPS_LIBS})
