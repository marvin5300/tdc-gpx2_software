#ifndef THREADSAFE_QUEUE_H
#define THREADSAFE_QUEUE_H
#include <algorithm>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>


namespace thrdsf {
    class non_empty_queue : public std::exception {
        std::string what_;
    public:
        explicit non_empty_queue(std::string msg) { what_ = std::move(msg); }
        const char* what() const noexcept override { return what_.c_str(); }
    };

    template<typename T>
    class Queue {
        std::queue<T> m_queue;
        mutable std::mutex m_mutex;

        // Moved out of public interface to prevent races between this
        // and pop().
        [[nodiscard]] bool empty() const {
            return m_queue.empty();
        }

    public:
        Queue() = default;
        Queue(const Queue<T>&) = delete;
        Queue& operator=(const Queue<T>&) = delete;

        Queue(Queue<T>&& other) noexcept(false) {
            std::scoped_lock<std::mutex> lock(m_mutex);
            if (!empty()) {
                throw non_empty_queue("Moving into a non-empty queue");
            }
            m_queue = std::move(other.m_queue);
        }

        virtual ~Queue() noexcept(false) {
            std::scoped_lock<std::mutex> lock(m_mutex);
            if (!empty()) {
                throw non_empty_queue("Destroying a non-empty queue");
            }
        }

        [[nodiscard]] unsigned long size() const {
            std::scoped_lock<std::mutex> lock(m_mutex);
            return m_queue.size();
        }

        std::optional<T> pop() {
            std::scoped_lock<std::mutex> lock(m_mutex);
            if (m_queue.empty()) {
                return {};
            }
            T tmp = m_queue.front();
            m_queue.pop();
            return tmp;
        }

        [[nodiscard]] std::optional<T> front() {
            std::scoped_lock<std::mutex> lock(m_mutex);
            if (m_queue.empty()) {
                return {};
            }
            T tmp = m_queue.front();
            return tmp;
        }

        [[nodiscard]] std::optional<T> back() {
            std::scoped_lock<std::mutex> lock(m_mutex);
            if (m_queue.empty()) {
                return {};
            }
            T tmp = m_queue.back();
            return tmp;
        }

        void push(const T& item) {
            std::scoped_lock<std::mutex> lock(m_mutex);
            m_queue.push(item);
        }
    };
}

#endif // THREADSAFE_QUEUE_H
