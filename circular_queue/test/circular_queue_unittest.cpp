#include <CppUTest/CommandLineTestRunner.h>

#include "circular_queue.h"
#include "test_queue.h"

std::shared_ptr<Queue<uint16_t> > (*make_queue)(size_t depth);

std::shared_ptr<Queue<uint16_t> > make_test_queue(size_t depth) {
    return std::make_shared<TestQueue>(depth);
}

std::shared_ptr<Queue<uint16_t> > make_circular_queue(size_t depth) {
    return std::make_shared<CircularQueue>(depth);
}


int main(int argc, char * argv[]) {
    printf("TestQueue unit tests...\n");
    make_queue = make_test_queue;
    CommandLineTestRunner::RunAllTests(argc, argv);

    printf("CircularQueue unit tests...\n");
    make_queue = make_circular_queue;
    return CommandLineTestRunner::RunAllTests(argc, argv);
}