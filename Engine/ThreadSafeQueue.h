#pragma once

template <class T>
class ThreadSafeQueue
{
public:
    void Push(T v)
    {
        std::lock_guard<std::mutex> lk(_mtx);
        _q.emplace_back(std::move(v));
    }

    // 현재 쌓인 것을 전부 꺼내서 out에 담음
    void PopAll(std::vector<T>& out)
    {
        std::lock_guard<std::mutex> lk(_mtx);
        while (!_q.empty())
        {
            out.emplace_back(std::move(_q.front()));
            _q.pop_front();
        }
    }

private:
    mutex _mtx;
    deque<T> _q;
};
