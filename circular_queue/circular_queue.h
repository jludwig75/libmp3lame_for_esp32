#pragma once


#include "i_queue.h"

#include <stdint.h>


class CircularQueue : public Queue<uint16_t> {
public:
    CircularQueue(size_t depth);
    ~CircularQueue();

    virtual bool push(const uint16_t & value);
    virtual bool pop(uint16_t & value);
    virtual bool is_empty() const;
    virtual bool is_full() const;
    virtual size_t depth() const;

private:

    size_t next_index_value(size_t index_value) const;

    size_t _max_depth;
    size_t _queue_entries;
    size_t _remove_idx;
    size_t _insert_idx;
    uint16_t *_queue_buffer;
};
