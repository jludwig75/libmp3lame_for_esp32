#pragma once


#include "i_queue.h"

#include <stdint.h>

#include <queue>
#include <mutex>


class TestQueue : public Queue<uint16_t> {
public:
    TestQueue(size_t depth);

    virtual bool push(const uint16_t & value);
    virtual bool pop(uint16_t & value);
    virtual bool is_empty() const;
    virtual bool is_full() const;
    virtual size_t depth() const;

private:
    size_t _max_depth;
    std::queue<uint16_t> _queue;
    mutable std::mutex _queue_lock;
};
