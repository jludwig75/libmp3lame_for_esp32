#include "test_queue.h"

using namespace std;
#include <assert.h>


TestQueue::TestQueue(size_t depth) : 
    _max_depth(depth),
    _queue(),
    _queue_lock()
{
}

bool TestQueue::push(const uint16_t & value) {
    unique_lock<mutex> lock(_queue_lock);

    assert(_queue.size() <= _max_depth);

    if (_queue.size() >= _max_depth) {
        return false;
    }

    _queue.push(value);
    assert(_queue.size() <= _max_depth);

    return true;
}

bool TestQueue::pop(uint16_t & value) {
    unique_lock<mutex> lock(_queue_lock);

    if (_queue.empty()) {
        return false;
    }

    value = _queue.front();
    _queue.pop();
    return true;
}

bool TestQueue::is_empty() const {
    unique_lock<mutex> lock(_queue_lock);

    return _queue.empty();
}

bool TestQueue::is_full() const {
    unique_lock<mutex> lock(_queue_lock);

    assert(_queue.size() <= _max_depth);

    return _queue.size() == _max_depth;
}

size_t TestQueue::depth() const {
    return _max_depth;
}
