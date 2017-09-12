#include <CppUTest/CommandLineTestRunner.h>

#include "make_queue.h"

TEST_GROUP(Queue) {
};


TEST(Queue, empty_queue_is_full) {
    std::shared_ptr<Queue<uint16_t> > q = make_queue(0);
    CHECK_EQUAL(0, q->depth());

    CHECK(q->is_full());
    CHECK(q->is_empty());
    CHECK_FALSE(q->push(0));
    uint16_t val;
    CHECK_FALSE(q->pop(val));
}

TEST(Queue, qd1) {
    std::shared_ptr<Queue<uint16_t> > q = make_queue(1);
    CHECK_EQUAL(1, q->depth());

    CHECK_FALSE(q->is_full());
    CHECK(q->is_empty());

    CHECK(q->push(0));
    CHECK_FALSE(q->push(2));
    CHECK(q->is_full());
    CHECK_FALSE(q->is_empty());

    uint16_t val;
    CHECK(q->pop(val));
    CHECK_FALSE(q->is_full());
    CHECK(q->is_empty());

    CHECK_FALSE(q->pop(val));
}

TEST(Queue, fill_and_empty_queue) {
    size_t queue_depth = 4;
    std::shared_ptr<Queue<uint16_t> > q = make_queue(queue_depth);
    CHECK_EQUAL(queue_depth, q->depth());

    CHECK_FALSE(q->is_full());
    CHECK(q->is_empty());

    for (size_t i = 0; i < queue_depth; i++) {
        CHECK(q->push((uint16_t)i));
        CHECK_FALSE(q->is_empty());
        if (i < queue_depth - 1) {
            CHECK_FALSE(q->is_full());
        }
    }

    CHECK(q->is_full());
    CHECK_FALSE(q->is_empty());
    CHECK_FALSE(q->push(9));

    for (size_t i = 0; i < queue_depth; i++) {
        uint16_t val;
        CHECK(q->pop(val));
        CHECK_EQUAL((uint16_t)i, val);
        if (i < queue_depth - 1) {
            CHECK_FALSE(q->is_empty());
        }
        CHECK_FALSE(q->is_full());
    }

    CHECK_FALSE(q->is_full());
    CHECK(q->is_empty());
}

TEST(Queue, fill_half__empty__fill__empty_half__fill_half__empty) {
    uint16_t value;

    size_t queue_depth = 4;
    std::shared_ptr<Queue<uint16_t> > q = make_queue(queue_depth);

    // Fill half
    CHECK(q->push(0));
    CHECK(q->push(1));


    // Empty
    CHECK(q->pop(value));
    CHECK_EQUAL(0, value);

    CHECK(q->pop(value));
    CHECK_EQUAL(1, value);

    // Can't pop more
    CHECK_FALSE(q->pop(value));


    // Fill all the way
    CHECK(q->push(2));
    CHECK(q->push(3));
    CHECK(q->push(4));
    CHECK(q->push(5));
    // Can't push more
    CHECK_FALSE(q->push(6));


    // Empty half
    CHECK(q->pop(value));
    CHECK_EQUAL(2, value);

    CHECK(q->pop(value));
    CHECK_EQUAL(3, value);


    // Fill half
    CHECK(q->push(6));
    CHECK(q->push(7));


    // Empty
    CHECK(q->pop(value));
    CHECK_EQUAL(4, value);

    CHECK(q->pop(value));
    CHECK_EQUAL(5, value);

    CHECK(q->pop(value));
    CHECK_EQUAL(6, value);

    CHECK(q->pop(value));
    CHECK_EQUAL(7, value);

    // Can't pop more
    CHECK_FALSE(q->pop(value));
}

TEST(Queue, queue_full_with_insert_and_remove_at_boundaries) {
    uint16_t value;

    size_t queue_depth = 4;
    std::shared_ptr<Queue<uint16_t> > q = make_queue(queue_depth);

    // Fill half
    CHECK(q->push(0));
    CHECK(q->push(1));
    CHECK(q->push(2));
    CHECK(q->push(3));

    CHECK(q->pop(value));
    CHECK_EQUAL(0, value);
    CHECK(q->push(3));
    CHECK(q->is_full());
    CHECK_FALSE(q->push(4));
}
