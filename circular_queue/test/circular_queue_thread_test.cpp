#include <CppUTest/CommandLineTestRunner.h>


#include "i_queue.h"

#include <thread>

#include "test_queue.h"
#include "circular_queue.h"

#ifdef  WIN32
#include <Windows.h>
#endif  // WIN32


std::chrono::microseconds sleep_time;
std::chrono::seconds test_run_time = std::chrono::seconds(10);

std::shared_ptr<Queue<uint16_t> >(*make_queue)(size_t depth);

std::shared_ptr<Queue<uint16_t> > make_test_queue(size_t depth) {
    return std::make_shared<TestQueue>(depth);
}

std::shared_ptr<Queue<uint16_t> > make_circular_queue(size_t depth) {
    return std::make_shared<CircularQueue>(depth);
}

class Thread {
public:
    Thread() : _thread_should_stop(false), _cpu_affinity_mask(0xFF) {

    }
    ~Thread() {}
    void start(uint32_t cpu_affinity_mask = 0xFF) {
        _cpu_affinity_mask = cpu_affinity_mask;
        _thread = std::thread(run_thread, this);
    }
    void stop() {
        _thread_should_stop = true;
        _thread.join();
    }

protected:
    virtual void run() = 0;
    virtual bool should_stop() const {
        return _thread_should_stop;
    }

private:
    static void run_thread(Thread *_this) {
#ifdef  WIN32
        DWORD ret = SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)&_this->_cpu_affinity_mask);
        if (ret == 0) {
            printf("Failed to set thread affinity to %04X: Error: %u.\n", _this->_cpu_affinity_mask, GetLastError());
        }
        _this->run();
#endif  // WIN32
    }
    std::thread _thread;
    bool _thread_should_stop;
    uint32_t _cpu_affinity_mask;
};

class QueueThread : public Thread {
public:
    QueueThread(std::shared_ptr<Queue<uint16_t> > & queue, size_t average_queue_operation_count) : _queue(queue), _average_queue_operation_count(average_queue_operation_count) {

    }

protected:
    virtual void do_work(size_t queue_operation_count) = 0;
    virtual void on_exit() {

    }

    std::shared_ptr<Queue<uint16_t> > & _queue;

    void sleep() const {
        if (sleep_time > std::chrono::microseconds(0)) {
            std::this_thread::sleep_for(sleep_time);
        }
    }

private:
    virtual void run() {
        while (!should_stop()) {
            bool start = true;
            size_t queue_operation_count = rand() % (_average_queue_operation_count * 2);
            do_work(queue_operation_count);
            sleep();
        }

        on_exit();
    }

    size_t _average_queue_operation_count;
};

class QueueProducerThread : public QueueThread {
public:
    QueueProducerThread(std::shared_ptr<Queue<uint16_t> > & queue, size_t average_insertion_count) : QueueThread(queue, average_insertion_count), _insertion_count(0), _queue_full_count(0) {
    }

    uint64_t get_queue_full_count() const {
        return _queue_full_count;
    }

    uint64_t get_insertion_count() const {
        return _insertion_count;
    }

private:
    virtual void on_exit() {
        printf("Queue full count: %I64u\n", _queue_full_count);
        printf("Items inserted: %I64u\n", _insertion_count);
    }

    void do_work(size_t queue_operation_count) {
        for (size_t i = 0; i < queue_operation_count; i++) {
            while (_queue->is_full()) {
                _queue_full_count++;
                sleep();
            }
            _queue->push((uint16_t)(_insertion_count++));
        }
    }

    uint64_t _insertion_count;
    uint64_t _queue_full_count;
};

class QueueConsumerThread : public QueueThread {
public:
    QueueConsumerThread(std::shared_ptr<Queue<uint16_t> > & queue, size_t average_removal_count) : QueueThread(queue, average_removal_count), _removal_count(0) {
    }

    uint64_t get_removal_count() const {
        return _removal_count;
    }

private:
    virtual void on_exit() {
        printf("Items removed: %I64u\n", _removal_count);
    }

    void do_work(size_t queue_operation_count) {
        for (size_t i = 0; i < queue_operation_count; i++) {
            while (_queue->is_empty()) {
                if (should_stop()) {
                    return;
                }
                sleep();
            }
            uint16_t value;
            bool popped_value = _queue->pop(value);
            CHECK(popped_value);
            if (popped_value) {
                uint16_t expected_value = (uint16_t)_removal_count;
                CHECK_EQUAL(expected_value, value);
                _removal_count++;
            }
        }
    }

    virtual bool should_stop() const {
        return QueueThread::should_stop() && _queue->is_empty();
    }

    uint64_t _removal_count;
};


TEST_GROUP(QueueThreadTests) {
    void run_test(size_t queue_depth, size_t average_insertion_count, size_t average_removal_count) {
        printf("\n");
        std::shared_ptr<Queue<uint16_t> > q = make_queue(queue_depth);

        QueueProducerThread producer(q, average_insertion_count);
        QueueConsumerThread consumer(q, average_removal_count);

        consumer.start(0x0F);
        producer.start(0xF0);

        std::this_thread::sleep_for(test_run_time);

        // Always stop the producer first.
        producer.stop();
        consumer.stop();

        CHECK_EQUAL(producer.get_insertion_count(), consumer.get_removal_count());
    }
};

TEST(QueueThreadTests, balanced_producer_consumer_moderate_insertion_count) {
    run_test(100, 10, 10);
}

TEST(QueueThreadTests, balanced_producer_consumer_small_insertion_count) {
    run_test(100, 4, 4);
}

TEST(QueueThreadTests, balanced_producer_consumer_high_insertion_count) {
    run_test(100, 100, 100);
}

TEST(QueueThreadTests, heavy_producer_low_consumer) {
    run_test(100, 100, 4);
}

TEST(QueueThreadTests, low_producer_heavy_consumer) {
    run_test(100, 4, 100);
}

int run_tests(int argc, char * argv[]) {
    //printf("TestQueue thread tests...\n");
    //make_queue = make_test_queue;
    //CommandLineTestRunner::RunAllTests(argc, argv);

    printf("CircularQueue thread tests...\n");
    make_queue = make_circular_queue;
    return CommandLineTestRunner::RunAllTests(argc, argv);
}

int main(int argc, char * argv[]) {

    //DWORD bytes_returned = 0;
    //std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(1);
    //BOOL ret = GetLogicalProcessorInformation(NULL, &bytes_returned);
    //if (!ret) {
    //    DWORD err = GetLastError();
    //    if (err != ERROR_INSUFFICIENT_BUFFER) {
    //        printf("Error %u getting logical processor information\n", GetLastError());
    //        return GetLastError();
    //    }
    //}

    //size_t entries_needed = (bytes_returned + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) - 1 ) / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    //buffer.resize(entries_needed);
    //bytes_returned = entries_needed * sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    //ret = GetLogicalProcessorInformation(&buffer[0], &bytes_returned);
    //if (!ret) {
    //    printf("Error %u getting logical processor information\n", GetLastError());
    //    return GetLastError();
    //}

    srand((unsigned)time(NULL));

    //printf("Running tests with delay...\n");
    //sleep_time = std::chrono::microseconds(100);
    //run_tests(argc, argv);

    printf("Running tests without delay...\n");
    sleep_time = std::chrono::microseconds(0);
    return run_tests(argc, argv);
}