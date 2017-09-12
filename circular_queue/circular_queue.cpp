#include "circular_queue.h"

#include <assert.h>


CircularQueue::CircularQueue(size_t depth) :
    _max_depth(depth),
    _queue_entries(_max_depth + 1),
    _remove_idx(0),
    _insert_idx(0),
    _queue_buffer(NULL)
{
    _queue_buffer = new uint16_t[_queue_entries];
}

CircularQueue::~CircularQueue() {
    delete [] _queue_buffer;
}

bool CircularQueue::push(const uint16_t & value) {
    if (is_full()) {
        return false;
    }

    _queue_buffer[_insert_idx] = value;
    _insert_idx = next_index_value(_insert_idx);
    return true;
}

bool CircularQueue::pop(uint16_t & value) {
    if (is_empty()) {
        return false;
    }

    value = _queue_buffer[_remove_idx];
    _remove_idx = next_index_value(_remove_idx);
    return true;
}

bool CircularQueue::is_empty() const {
    return _remove_idx == _insert_idx;
}

bool CircularQueue::is_full() const {
    return next_index_value(_insert_idx) == _remove_idx;
}

size_t CircularQueue::depth() const {
    return _max_depth;
}

size_t CircularQueue::next_index_value(size_t index_value) const {
    assert(index_value < _queue_entries);

    return (index_value + 1) % _queue_entries;
}
