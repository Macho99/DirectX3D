#pragma once
#include <functional>
#include <vector>
#include <memory>
#include <algorithm>
#include <cstdint>
#include <utility>

template<typename... Args>
class Event
{
public:
    using Handler = function<void(Args...)>;
    using ListenerId = uint64;

private:
    struct Entry
    {
        ListenerId id;
        Handler handler;
    };

    struct State
    {
        vector<Entry> handlers;
        ListenerId nextId = 0;
    };

public:
    class ScopedListener
    {
    public:
        ScopedListener() = default;

        ScopedListener(weak_ptr<State> state, ListenerId id)
            : _state(std::move(state)), _id(id)
        {
        }

        ~ScopedListener()
        {
            Reset();
        }

        ScopedListener(const ScopedListener&) = delete;
        ScopedListener& operator=(const ScopedListener&) = delete;

        ScopedListener(ScopedListener&& other) noexcept
            : _state(std::move(other._state)), _id(other._id)
        {
            other._id = 0;
        }

        ScopedListener& operator=(ScopedListener&& other) noexcept
        {
            if (this == &other)
                return *this;

            Reset();

            _state = std::move(other._state);
            _id = other._id;
            other._id = 0;

            return *this;
        }

        void Reset()
        {
            if (_id == 0)
                return;

            if (auto state = _state.lock())
            {
                auto& handlers = state->handlers;
                handlers.erase(
                    std::remove_if(handlers.begin(), handlers.end(),
                        [id = _id](const Entry& e)
                        {
                            return e.id == id;
                        }),
                    handlers.end());
            }

            _state.reset();
            _id = 0;
        }

        bool IsValid() const
        {
            return _id != 0 && !_state.expired();
        }

    private:
        weak_ptr<State> _state;
        ListenerId _id = 0;
    };

public:
    Event()
        : _state(make_shared<State>())
    {
    }

    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
    Event(Event&&) = default;
    Event& operator=(Event&&) = default;

    ScopedListener AddListener(Handler handler)
    {
        ListenerId id = ++_state->nextId;
        _state->handlers.push_back({ id, std::move(handler) });
        return ScopedListener(_state, id);
    }

    void Invoke(Args... args)
    {
        auto snapshot = _state->handlers;

        for (auto& entry : snapshot)
        {
            if (entry.handler)
                entry.handler(args...);
        }
    }

    //void Clear()
    //{
    //    _state->handlers.clear();
    //}

    size_t Count() const
    {
        return _state->handlers.size();
    }

private:
    shared_ptr<State> _state;
};