#pragma once


template<typename _T>
class Queue {
public:
    virtual ~Queue() {}
    virtual bool push(const _T & value) = 0;
    virtual bool pop(_T & value) = 0;
    virtual bool is_empty() const = 0;
    virtual bool is_full() const = 0;
    virtual size_t depth() const = 0;
};
