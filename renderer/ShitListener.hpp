/**
 * @file ShitListener.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <set>

#include "ShitUtility.hpp"

namespace Shit {
template <typename Fn, std::enable_if_t<std::is_function_v<Fn>, bool> = true>
class Listener : public std::list<std::shared_ptr<std::function<Fn>>> {
public:
    Listener() {}
    Listener(const Listener &other) = delete;

    template <typename... Args, std::enable_if_t<std::is_invocable_v<Fn, Args...>, bool> = true>
    void notify(Args... args) {
        for (auto &&it = this->begin(); it != this->end(); ++it) {
            (**it)(args...);
            // if (it->expired())
            //	it = this->erase(it);
            // else
            //{
            //	(*(it->lock()))(args...);
            //	++it;
            // }
        }
    }
};
//===============================================
template <typename T, std::enable_if_t<!std::is_void_v<T>, bool> = true>
struct all_values {
    typedef std::vector<std::decay_t<T>> result_type;

    template <typename InputIter>
    result_type operator()(InputIter first, InputIter last) const {
        result_type res;
        while (first != last) {
            res.emplace_back(*first);
            // try
            //{
            //	res.emplace_back(*first);
            // }
            // catch (const std::exception &e)
            //{
            //	// std::cout << e.what() << std::endl;
            //	ST_THROW("signal error ", e.what())
            // };
            ++first;
        }
        return res;
    }
};
//===============================================
template <typename T>
struct last_value {
    typedef std::optional<T> result_type;

    template <typename InputIter>
    result_type operator()(InputIter first, InputIter last) const {
        std::optional<T> value;
        while (first != last) {
            try {
                value = *first;
            } catch (const std::exception &e) {
                // std::cout << e.what() << std::endl;
                ST_THROW("signal error ", e.what())
            };
            ++first;
        }
        return value;
    }
};
template <>
struct last_value<void> {
    typedef void result_type;

    template <typename InputIter>
    void operator()(InputIter first, InputIter last) const {
        while (first != last) {
            try {
                *first;
            } catch (const std::exception &e) {
                // std::cout << e.what() << std::endl;
                ST_THROW("signal error ", e.what())
            }
            ++first;
        }
    }
};

class SlotBase;
class SignalBase {
    friend class SlotBase;

public:
    virtual ~SignalBase() {}

protected:
    virtual std::shared_ptr<void> lock_pimpl() const = 0;
};

using tracked_container_type = std::vector<std::variant<std::weak_ptr<void>>>;
using locked_container_type = std::vector<std::variant<std::shared_ptr<void>>>;

class SlotBase {
protected:
    tracked_container_type _trackedObjects;
    locked_container_type _lockedObjects;

public:
    virtual ~SlotBase() {}

    const tracked_container_type &tracked_objects() const { return _trackedObjects; }

    // if one of tracked objects is expired, return ture, otherwise return false
    bool expired() const {
        for (auto &&e : _trackedObjects) {
            if (std::visit(overloaded{[](const std::weak_ptr<void> &arg) { return arg.expired(); }}, e)) return true;
        }
        return false;
    }

    //
    locked_container_type lock() const {
        locked_container_type ret;
        for (auto &&e : _trackedObjects) {
            ret.emplace_back(std::visit(overloaded{[](const std::weak_ptr<void> &arg) {
                                            if (arg.expired()) throw std::runtime_error("expired slot");
                                            return arg.lock();
                                        }},
                                        e));
        }
        return ret;
    }

protected:
    void track_signal(const SignalBase &signal) { _trackedObjects.emplace_back(signal.lock_pimpl()); }
};

template <typename _Signature, std::enable_if_t<std::is_function_v<_Signature>, bool> = true>
class Slot : public SlotBase {
public:
    using function_type = typename std::function<_Signature>;
    using result_type = typename function_type::result_type;

    template <typename T>
    Slot(const T &t) : _slotFunction(t) {}
    // check if empty
    explicit operator bool() const noexcept { return bool(_slotFunction); }
    // invocation
    template <typename... Args>
    result_type operator()(Args &&...args) const {
        ST_MAYBE_UNUSED auto &&a = lock();
        return _slotFunction(args...);
    }

    Slot &Track(const std::weak_ptr<void> &v) {
        _trackedObjects.emplace_back(v);
        return *this;
    }
    Slot &Track(const SignalBase &v) {
        track_signal(v);
        return *this;
    }
    Slot &Track(const SlotBase &v) {
        for (auto &&e : v.tracked_objects()) _trackedObjects.emplace_back(e);
        return *this;
    }

    // slot function access
    constexpr const function_type &SlotFunction() const { return _slotFunction; }
    constexpr function_type &SlotFunction() { return _slotFunction; }

    bool operator==(const Slot &other) const {
        return _slotFunction.target<_Signature>() == other._slotFunction.target<_Signature>();
    }

private:
    function_type _slotFunction;
};

// called by combiner, wrap slot iter and arguments
template <typename _ConnectionBody, typename _ConnectionBodyIter, typename... _Args>
class SlotCallIterator {
public:
    using result_type = typename _ConnectionBody::slot_type::result_type;

    SlotCallIterator(_ConnectionBodyIter it, std::shared_ptr<std::tuple<_Args...>> args) : _iter(it), _args(args) {}
    result_type operator*() { return invokeFunc(std::make_index_sequence<sizeof...(_Args)>{}); }
    bool operator==(const SlotCallIterator &other) { return _iter == other._iter; }
    bool operator!=(const SlotCallIterator &other) { return _iter != other._iter; }
    SlotCallIterator &operator++() {
        ++_iter;
        return *this;
    };
    SlotCallIterator &operator--() {
        --_iter;
        return *this;
    };
    SlotCallIterator operator++(int) {
        auto ret = *this;
        _iter++;
        return ret;
    };
    SlotCallIterator operator--(int) {
        auto ret = *this;
        _iter++;
        return ret;
    };

private:
    _ConnectionBodyIter _iter;
    std::shared_ptr<std::tuple<_Args...>> _args;

    template <size_t... I>
    result_type invokeFunc(std::index_sequence<I...>) {
        return (*(*_iter)->GetSlot())(std::get<I>(*_args)...);
    }
};

// class noncopyable
//{
// protected:
//	constexpr noncopyable() = default;
//	~noncopyable() = default;

//	noncopyable(const noncopyable &) = delete;
//	noncopyable &operator=(const noncopyable &) = delete;
//};

// template <typename Mutex>
// class garbage_collecting_lock : public noncopyable
//{
// public:
//	garbage_collecting_lock(Mutex &m) : lock(m)
//	{
//	}
//	void add_trash(const shared_ptr<void> &piece_of_trash)
//	{
//		garbage.emplace_back(piece_of_trash);
//	}

// private:
//	// garbage must be declared before lock
//	// to insure it is destroyed after lock is
//	// destroyed.
//	std::vector<std::shared_ptr<void>> garbage;
//	std::unique_lock<Mutex> lock;
// };

class ConnectionBodyBase {
    mutable bool _connected{true};

public:
    virtual ~ConnectionBodyBase() {}

    void Disconnect() {
        if (_connected) {
            std::unique_lock<ConnectionBodyBase> lk(*this);
            ReleaseSlot();
            _connected = false;
        }
    }
    bool Connected() const {
        if (SlotExpired()) _connected = false;
        return _connected;
    }

    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual std::shared_ptr<void> ReleaseSlot() = 0;
    virtual bool SlotExpired() const = 0;
};

template <typename _SlotType, typename _GroupKey>
class ConnectionBody : public ConnectionBodyBase {
    std::shared_ptr<_SlotType> _slot;
    std::shared_ptr<std::mutex> _mutex;  // from  signal
    _GroupKey _group{};

public:
    using slot_type = _SlotType;

    ConnectionBody(const _SlotType &slotIn, const std::shared_ptr<std::mutex> &signalMutex) noexcept
        : _slot(new _SlotType(slotIn)), _mutex(signalMutex) {}
    ConnectionBody(_GroupKey group, const _SlotType &slotIn, const std::shared_ptr<std::mutex> &signalMutex) noexcept
        : _group(group), _slot(new _SlotType(slotIn)), _mutex(signalMutex) {}
    ~ConnectionBody() { Disconnect(); }

    void DisconnectExpiredSlot() {
        if (_slot && _slot->Expired()) {
            Disconnect();
        }
    }

    void lock() override { _mutex->lock(); }
    void unlock() override { _mutex->unlock(); }

    constexpr std::shared_ptr<_SlotType> GetSlot() const { return _slot; }

    std::shared_ptr<void> ReleaseSlot() override {
        std::shared_ptr<_SlotType> ret;
        ret.swap(_slot);
        return ret;
    }
    bool SlotExpired() const override { return _slot->expired(); }
    const _GroupKey &GroupKey() const { return _group; }
    void SetGroupKey(const _GroupKey &group) { _group = group; }

    // void DisconnectExpiredSlot()
    //{
    // }
};

class Connection {
    std::weak_ptr<ConnectionBodyBase> _weakConnectionBody;

public:
    Connection(const std::weak_ptr<ConnectionBodyBase> &connectionBody) noexcept
        : _weakConnectionBody(connectionBody) {}

    // connection management
    void Disconnect() const {
        if (!_weakConnectionBody.expired()) _weakConnectionBody.lock()->Disconnect();
    }
    bool Connected() const noexcept {
        if (!_weakConnectionBody.expired()) return _weakConnectionBody.lock()->Connected();
        return false;
    }

    void Swap(Connection &other) noexcept { std::swap(_weakConnectionBody, other._weakConnectionBody); }

    // blocking
    // bool Blocked() const noexcept {}

    // bool operator==(const Connection &other) const noexcept {}
    // bool operator!=(const Connection &other) const noexcept { return !(*this ==
    // other); } bool operator<(const Connection &other) const noexcept {}
};

template <typename _Signature, typename _Combiner = last_value<typename std::function<_Signature>::result_type>,
          typename _Group = int, std::enable_if_t<std::is_function_v<_Signature>, bool> = true>
class Signal : public SignalBase {
public:
    using signature_type = _Signature;
    using group_type = _Group;
    using slot_function_type = std::function<signature_type>;
    using slot_type = Slot<signature_type>;
    using combiner_type = _Combiner;
    using result_type = typename combiner_type::result_type;
    using connection_body_type = ConnectionBody<slot_type, group_type>;

    struct ConnectionCompare {
        bool operator()(const std::shared_ptr<connection_body_type> &lhs,
                        const std::shared_ptr<connection_body_type> &rhs) const {
            return lhs->GroupKey() < rhs->GroupKey();
        };
    };
    using connection_container_type = std::multiset<std::shared_ptr<connection_body_type>, ConnectionCompare>;
    using connection_container_iterator_type = connection_container_type::iterator;

    Signal() : _mutex(std::make_shared<std::mutex>()), _handle(std::make_shared<int>()) {}

    Connection Connect(slot_type const &slot) {
        const std::lock_guard<std::mutex> lock(*_mutex);
        return Connection(*_connections.emplace(std::make_shared<connection_body_type>(slot, _mutex)));
    }
    Connection Connect(group_type group, slot_type const &slot) {
        const std::lock_guard<std::mutex> lock(*_mutex);
        return Connection(*_connections.emplace(std::make_shared<connection_body_type>(group, slot, _mutex)));
    }

    bool Contains(const slot_type &slot) {
        return std::find_if(_connections.cbegin(), _connections.cend(),
                            [&slot](auto &&e) { return *e->GetSlot() == slot; }) != _connections.cend();
    }
    void Disconnect(slot_type const &slot) {
        auto it = std::find_if(_connections.cbegin(), _connections.cend(),
                               [&slot](auto &&e) { return *e->GetSlot() == slot; });
        if (it != _connections.cend()) {
            _connections.erase(it);
        }
    }
    void DisconnectAllConnections() { _connections.clear(); }
    constexpr bool Empty() const { return _connections.empty(); }
    constexpr size_t ConnectionNum() const { return _connections.size(); }

    void ClearExpiredConnections() const {
        while (true) {
            auto it = std::find_if(_connections.begin(), _connections.end(), [](auto &&e) {
                auto a = e->GetSlot();
                return !a || e->SlotExpired();
            });

            if (it == _connections.end())
                break;
            else
                _connections.erase(it);
        }
    };

    template <typename... Args>
    result_type operator()(Args &&...args) const {
        ClearExpiredConnections();
        auto a = std::make_shared<std::tuple<Args...>>(args...);
        return _combiner(
            SlotCallIterator<connection_body_type, connection_container_iterator_type, Args...>(_connections.begin(),
                                                                                                a),
            SlotCallIterator<connection_body_type, connection_container_iterator_type, Args...>(_connections.end(), a));
    }

private:
    mutable connection_container_type _connections{};

    std::shared_ptr<std::mutex> _mutex;
    std::shared_ptr<int> _handle;

    combiner_type _combiner{};

    constexpr std::shared_ptr<int> getHandle() const { return _handle; }

    std::shared_ptr<void> lock_pimpl() const override { return _handle; }
};
}  // namespace Shit