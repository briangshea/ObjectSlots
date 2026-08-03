#ifndef _OBJECTSLOTS_V2_HPP_
#define _OBJECTSLOTS_V2_HPP_

#include <ObjectSlots/objectslots_config.hpp>
#ifdef ENABLE_OBJECTSLOTS_V2
#include <list>
#include <memory>
#include <functional>
#include <algorithm>
#include <type_traits>
#include <shared_mutex>
#include <mutex>
#include <atomic>
#include <vector>

namespace Signals {
inline namespace v2 {

template <typename Signature> class Signal;
template <typename Signature> class _ConnectionToken;

class SignalBase {
protected:
    SignalBase() = default;
    ~SignalBase() = default;

public:
    class _UniversalPasskey {
    private:
        friend class SignalBase;
        constexpr _UniversalPasskey() = default;
    };

protected:
    [[nodiscard]] constexpr inline _UniversalPasskey emit_key() const noexcept {
        return _UniversalPasskey{};
    }
};

template <typename ReturnType, typename... Args>
class Signal<ReturnType(Args...)> {
public:
    using SignatureType = ReturnType(Args...);
    using Connection = _ConnectionToken<SignatureType>;
    
    // Made public so _ConnectionToken can access this type definition
    struct ConnectionState {
        std::function<SignatureType> callback;
        std::weak_ptr<void> trackingObject;
        bool hasTracker = false;
        
        std::atomic<bool> active{true};
        std::atomic<bool> blocked{false};
        int priority = 0;
    };

private:
    using _AccessKey = SignalBase::_UniversalPasskey;
    std::list<std::shared_ptr<ConnectionState>> connections;
    mutable std::shared_mutex listMutex; 

    void insert_prioritized(const std::shared_ptr<ConnectionState>& newState) {
        std::unique_lock<std::shared_mutex> lock(listMutex);
        auto it = std::find_if(connections.begin(), connections.end(),
            [prio = newState->priority](const auto& existingConn) { return existingConn->priority < prio; });
        connections.insert(it, newState);
    }

    // Safely captures active elements under the lock to prevent reentrancy deadlocks
    std::vector<std::shared_ptr<ConnectionState>> snapshot_connections() const {
        std::vector<std::shared_ptr<ConnectionState>> snapshot;
        {
            std::shared_lock<std::shared_mutex> lock(listMutex);
            snapshot.reserve(connections.size());
            for (const auto& conn : connections) {
                if (!conn->active.load(std::memory_order_acquire)) continue;
                if (conn->blocked.load(std::memory_order_acquire)) continue;
                if (conn->hasTracker && conn->trackingObject.expired()) continue;
                snapshot.push_back(conn);
            }
        }
        return snapshot;
    }

public:
    Signal() = default;
    ~Signal() = default;

    template <typename F>
    [[nodiscard]] Connection bind(F&& callback, int priority = 0) {
        auto state = std::make_shared<ConnectionState>();
        state->callback = std::forward<F>(callback);
        state->hasTracker = false;
        state->priority = priority;
        insert_prioritized(state);
        return Connection(state);
    }

    template <typename T>
    [[nodiscard]] Connection bind(ReturnType(T::*memberFunc)(Args...), const std::shared_ptr<T>& obj, int priority = 0) {
        auto state = std::make_shared<ConnectionState>();
        std::weak_ptr<T> weakObj = obj;
        state->callback = [weakObj, memberFunc](Args... args) -> ReturnType {
            if (auto sharedObj = weakObj.lock()) { return ((*sharedObj).*memberFunc)(std::forward<Args>(args)...); }
            if constexpr (!std::is_void_v<ReturnType>) { return ReturnType{}; }
        };
        state->trackingObject = weakObj;
        state->hasTracker = true;
        state->priority = priority;
        insert_prioritized(state);
        return Connection(state);
    }

    template <typename T>
    [[nodiscard]] Connection bind(ReturnType(T::*memberFunc)(Args...) const, const std::shared_ptr<T>& obj, int priority = 0) {
        auto state = std::make_shared<ConnectionState>();
        std::weak_ptr<T> weakObj = obj;
        state->callback = [weakObj, memberFunc](Args... args) -> ReturnType {
            if (auto sharedObj = weakObj.lock()) { return ((*sharedObj).*memberFunc)(std::forward<Args>(args)...); }
            if constexpr (!std::is_void_v<ReturnType>) { return ReturnType{}; }
        };
        state->trackingObject = weakObj;
        state->hasTracker = true;
        state->priority = priority;
        insert_prioritized(state);
        return Connection(state);
    }

    // Overload 1: Fire-and-forget
    template <typename Owner>
    void operator()(_AccessKey, const Owner* /*owner*/, Args... args) {
        auto snapshot = snapshot_connections();
        for (const auto& conn : snapshot) {
            // Check flags again in case a prior slot modified this connection
            if (!conn->active.load(std::memory_order_acquire)) continue;
            if (conn->blocked.load(std::memory_order_acquire)) continue;
            conn->callback(std::forward<Args>(args)...);
        }
    }

    // Overload 2: Predicate short-circuit
    // FIX: Moved template pack 'Args... args' to the very end of the arguments list
    template <typename Owner, typename Predicate, 
              typename = std::enable_if_t<!std::is_void_v<ReturnType> && std::is_invocable_r_v<bool, Predicate, ReturnType>>>
    void operator()(_AccessKey, const Owner* /*owner*/, Predicate&& should_stop, Args... args) {
        auto snapshot = snapshot_connections();
        for (const auto& conn : snapshot) {
            if (!conn->active.load(std::memory_order_acquire)) continue;
            if (conn->blocked.load(std::memory_order_acquire)) continue;

            if (should_stop(conn->callback(std::forward<Args>(args)...))) {
                break; 
            }
        }
    }

    // Collect layout protection
    template <typename Owner>
    auto emit_and_collect(_AccessKey, const Owner* /*owner*/, Args... args) {
        static_assert(!std::is_void_v<ReturnType>, "emit_and_collect() cannot be used when the Signal return type is void.");
        std::list<ReturnType> results;
        auto snapshot = snapshot_connections();
        for (const auto& conn : snapshot) {
            if (!conn->active.load(std::memory_order_acquire)) continue;
            if (conn->blocked.load(std::memory_order_acquire)) continue;

            results.push_back(conn->callback(std::forward<Args>(args)...));
        }
        return results;
    }

    void sweep() {
        std::unique_lock<std::shared_mutex> lock(listMutex);
        connections.remove_if([](const auto& conn) { 
            return !conn->active.load(std::memory_order_relaxed) || 
                   (conn->hasTracker && conn->trackingObject.expired()) || 
                   conn.use_count() <= 1; 
        });
    }

    size_t internal_node_count() const {
        std::shared_lock<std::shared_mutex> lock(listMutex);
        return connections.size();
    }
};

class ConnectionBase {
public:
    virtual ~ConnectionBase() = default;
    
    virtual void block() noexcept = 0;
    virtual void unblock() noexcept = 0;
    virtual void disconnect() = 0;
    virtual bool isValid() const noexcept = 0;
};

template <typename ReturnType, typename... Args>
class _ConnectionToken<ReturnType(Args...)> : public ConnectionBase {
public:
    using StateType = typename Signal<ReturnType(Args...)>::ConnectionState;

    _ConnectionToken() = default;
    explicit _ConnectionToken(std::shared_ptr<StateType> state) : connectionState(std::move(state)) {}
    ~_ConnectionToken() { disconnect(); }
    
    _ConnectionToken(const _ConnectionToken&) = delete;
    _ConnectionToken& operator=(const _ConnectionToken&) = delete;
    
    _ConnectionToken(_ConnectionToken&& other) noexcept = default;
    _ConnectionToken& operator=(_ConnectionToken&& other) noexcept {
        if (this != &other) { disconnect(); connectionState = std::move(other.connectionState); }
        return *this;
    }
    
    void block() noexcept override { if (connectionState) connectionState->blocked.store(true, std::memory_order_release); }
    void unblock() noexcept override { if (connectionState) connectionState->blocked.store(false, std::memory_order_release); }
    void disconnect() override { if (connectionState) { connectionState->active.store(false, std::memory_order_release); connectionState.reset(); } }
    bool isValid() const noexcept override { return connectionState && connectionState->active.load(std::memory_order_acquire); }

private:
    std::shared_ptr<StateType> connectionState;
};

}; // namespace v2
}; // namespace Signals
#endif // ENABLE_OBJECTSLOTS_V2
#endif //_OBJECTSLOTS_V2_HPP_
