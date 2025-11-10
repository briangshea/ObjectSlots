#ifndef _OBJECTSLOTS_HPP_
#define _OBJECTSLOTS_HPP_

#include <functional>
#ifdef OBJECTSLOTS_ENABLE_THREADS
#define OBJECTSLOTS_THREADED
#include <thread>
#endif
#ifdef OBJECTSLOTS_ENABLE_THREAD_SAFETY
#define OBJECTSLOTS_THREAD_SAFE
#endif

namespace ObjectSlots {

template<class T, class ReturnType, class ... Args>
using SlotMethodP = ReturnType (T::*)(Args...);

template<class ReturnType, class ... Args>
using SlotFunctionP = ReturnType (*)(Args...);

template<class ReturnType, class ... Args>
using SlotLambdaP = ReturnType (*)(Args...);

/**
 * @brief `Base` class to handle slots of either Class/Method
 * or plain functions.
 * @tparam ReturnType The return type of the slot.
 * @tparam Args The argument types of the slot.
 */
template<class ReturnType, class ... Args>
class Base {
    // If this fails, then the size of a function pointer
    // cannot be stored in a void* type and may cause
    // issues. If you see this error please write an
    // issue provide the details of the system architecture
    // that is being used.
    static_assert(sizeof(void(*)()) <= sizeof(void*));
public:
    virtual ~Base() = default;

    /**
     * @brief Returns a pointer to the object associated with the slot.
     *        For SlotFunction, this will always be nullptr.
     * @return A void pointer to the object, or nullptr if not applicable.
     */
    virtual const void* object() const { return nullptr; }

    /**
     * @brief Returns a void pointer representation of the callback function or method.
     *        This is used for comparison and identification of slots.
     * @return A void pointer to the callback.
     */
    virtual const void* callback() const = 0;

    /**
     * @brief Invokes the slot with the given arguments.
     * @param args The arguments to pass to the slot.
     * @return The return value of the slot.
     */
    virtual ReturnType operator()(Args... args) const = 0;
};

/**
 * @brief `SlotMethod` class to handle slots of Class/Method.
 * @tparam T The class type of the object owning the method.
 * @tparam ReturnType The return type of the method.
 * @tparam Args The argument types of the method.
 */
template<class T, class ReturnType, class ... Args>
class SlotMethod : public Base<ReturnType, Args...> {
private:
    T* object_;
    SlotMethodP<T, ReturnType, Args...> callback_;
public:
    SlotMethod(T* object, SlotMethodP<T, ReturnType, Args...> callback)
        : object_(object), callback_(callback) {}

    const void* object() const override { return object_; }
    const void* callback() const override {
        union {
            SlotMethodP<T, ReturnType, Args...> signal_ptr;
            const void *ptr;
        } to_void_ptr;
        to_void_ptr.signal_ptr = callback_;
        return to_void_ptr.ptr;
    }

    ReturnType operator()(Args... args) const override {
        return (object_->*callback_)(args...);
    }
};

/**
 * @brief `SlotFunction` class to handle slots of plain functions.
 * @tparam ReturnType The return type of the function.
 * @tparam Args The argument types of the function.
 */
template<class ReturnType, class ... Args>
class SlotFunction : public Base<ReturnType, Args...> {
private:
    SlotFunctionP<ReturnType, Args...> callback_;

public:
    SlotFunction(SlotFunctionP<ReturnType, Args...> callback)
        : callback_(callback) {}

    const void* object() const override { return nullptr; }
    const void* callback() const override { return reinterpret_cast<void*>(callback_); }

    ReturnType operator()(Args... args) const override {
        return callback_(args...);
    }
};

template<class ReturnType, class ... Args>
class SlotLambda : Base<ReturnType, Args...> {
private:
    std::function<ReturnType(Args...)> lambda_;
    const void *callback_;
    using self_type = SlotLambda<ReturnType, Args...>;
public:
    SlotLambda(std::function<ReturnType(Args...)> lambda, const void* cb) :
        lambda_(lambda), callback_(cb)
    { }

    ReturnType operator()(Args ... args) const override {
        return lambda_(args...);
    }

    const void* object() const override { return this; }
    const void* callback() const override { return callback_; }
};

/**
 * @brief `ObjectSlots` is a base class that provides signal/slot functionality.
 *        Derived classes can emit signals, and other objects or functions can bind to these signals as slots.
 *
 * Example Usage:
 * ```cpp
 * #include <iostream>
 * #include <slots/slots.hpp>
 *
 * class MyEmitter : public Slots::ObjectSlots {
 * public:
 *     void valueChanged(int newValue) {
 *         emit(&MyEmitter::valueChanged, newValue);
 *     }
 * };
 *
 * void globalFunctionSlot(int value) {
 *     std::cout << "Global function received: " << value << std::endl;
 * }
 *
 * class MyReceiver {
 * public:
 *     void memberFunctionSlot(int value) {
 *         std::cout << "Member function received: " << value << std::endl;
 *     }
 * };
 *
 * int main() {
 *     MyEmitter emitter;
 *     MyReceiver receiver;
 *     emitter.bind(&MyEmitter::valueChanged, &globalFunctionSlot);
 *     emitter.bind(&MyEmitter::valueChanged, &receiver, &MyReceiver::memberFunctionSlot);
 *     emitter.valueChanged(42); // This will call both slots
 *     return 0;
 * }
 * ```
 */
class ObjectSlots {
private:
    using LockP = void*;
public:
    ObjectSlots();
    virtual ~ObjectSlots();

    template<class SignalType, class ReturnType, typename Func, class ... Args>
    void bind(
        SlotMethodP<SignalType, ReturnType, Args...> signal,
        //T* object,
        Func&& f)
    {
        union {
            SlotMethodP<SignalType, ReturnType, Args...> signal_ptr;
            void *ptr;
        } to_void_ptr;
        to_void_ptr.signal_ptr = signal;
        SlotLambda<ReturnType, Args...>* lambda = new SlotLambda<ReturnType, Args...>(f, &f);
        slotStore(to_void_ptr.ptr, lambda);
    }

    template<typename Func>
    void unbind(Func&& f) {
        slotRemove(nullptr, &f);
    }

    /**
     * @brief Binds a member function slot to a signal.
     *
     * @tparam SignalType The class type of the object emitting the signal.
     * @tparam ReturnType The return type of the signal and slot.
     * @tparam T The class type of the object owning the slot method.
     * @tparam Args The argument types of the signal and slot.
     * @param signal A pointer to the member function representing the signal.
     * @param object A pointer to the object instance that owns the slot method.
     * @param callback A pointer to the member function representing the slot.
     */
    template <class SignalType, class ReturnType, class T, class ... Args>
    void bind(
        SlotMethodP<SignalType, ReturnType, Args...> signal,
        T* object,
        SlotMethodP<T, ReturnType, Args...> callback)
    {
        union {
            SlotMethodP<SignalType, ReturnType, Args...> signal_ptr;
            void *ptr;
        } to_void_ptr;
        to_void_ptr.signal_ptr = signal;
        SlotMethod<T, ReturnType, Args...>* method = new SlotMethod<T, ReturnType, Args...>(object, callback);
        slotStore(to_void_ptr.ptr, method);
    }

    /**
     * @brief Binds a free function slot to a signal.
     *
     * @tparam SignalType The class type of the object emitting the signal.
     * @tparam ReturnType The return type of the signal and slot.
     * @tparam Args The argument types of the signal and slot.
     * @param signal A pointer to the member function representing the signal.
     * @param callback A pointer to the free function representing the slot.
     */
    template <class SignalType, class ReturnType, class ... Args>
    void bind(
        SlotMethodP<SignalType, ReturnType, Args...> signal,
        SlotFunctionP<ReturnType, Args...> callback)
    {
        union {
            SlotMethodP<SignalType, ReturnType, Args...> signal_ptr;
            void *ptr;
        } to_void_ptr;
        to_void_ptr.signal_ptr = signal;
        SlotFunction<ReturnType, Args...>* function = new SlotFunction<ReturnType, Args...>(callback);
        slotStore(to_void_ptr.ptr, function);
    }

    /**
     * @brief Unbinds a member function slot from all signals it might be connected to.
     *
     * @tparam T The class type of the object owning the slot method.
     * @tparam ReturnType The return type of the slot.
     * @tparam Args The argument types of the slot.
     * @param object A pointer to the object instance that owns the slot method.
     * @param method A pointer to the member function representing the slot.
     *               This method will be unbound from any signal it's connected to on this ObjectSlots.
     */
    template <class T, class ReturnType, class ... Args>
    void unbind(T* object, SlotMethodP<T, ReturnType, Args...> method) {
        union {
            SlotMethodP<T, ReturnType, Args...> method_ptr;
            void *ptr;
        } to_void_ptr;
        to_void_ptr.method_ptr = method;
        slotRemove(object, to_void_ptr.ptr);
    }

    /**
     * @brief Unbinds a free function slot from all signals it might be connected to.
     *
     * @tparam ReturnType The return type of the slot.
     * @tparam Args The argument types of the slot.
     * @param function A pointer to the free function representing the slot.
     *                 This function will be unbound from any signal it's connected to on this ObjectSlots.
     *                 Note: This unbinds the specific function, not all functions.
     */
    template <class ReturnType, class ... Args>
    void unbind(SlotFunctionP<ReturnType, Args...> function) {
        union {
            SlotFunctionP<ReturnType, Args...> function_ptr;
            void *ptr;
        } to_void_ptr;
        to_void_ptr.function_ptr = function;
        slotRemove(nullptr, to_void_ptr.ptr);
    }

    /**
     * @brief Unbinds all slots associated with a specific object.
     *
     * @tparam T The class type of the object.
     * @param object A pointer to the object whose member function slots should be unbound.
     *               All member function slots belonging to this object that are bound to any signal
     *               on this ObjectSlots will be removed.
     */
    template <class T>
    void unbind(T* object) {
        slotRemove(object, nullptr);
    }

    /**
     * @brief Unbinds a specific free function slot from all signals it might be connected to.
     * @param function A void pointer to the free function to be unbound.
     */
    void unbind(void* function) {
        slotRemove(nullptr, function);
    }

protected:
    /**
     * @brief Emits a signal, invoking all bound slots.
     *
     * This method is intended to be called by derived classes to trigger a signal.
     * It iterates through all slots bound to the given signal and invokes them with the provided arguments.
     *
     * @tparam T The class type of the object emitting the signal.
     * @tparam Args The argument types of the signal and the slots.
     * @param callback A pointer to the member function representing the signal being emitted.
     * @param args The arguments to pass to the bound slots.
     */
    template<class T, class ... Args>
    void emit(
        SlotMethodP<T, void, Args...> callback,
        Args... args)
    {
#ifdef OBJECTSLOTS_THREAD_SAFE
        auto lock = acquireLock();
#endif
        union {
            SlotMethodP<T, void, Args...> signal_ptr;
            void *ptr;
        } to_void_ptr;
        to_void_ptr.signal_ptr = callback;
        int i = 0;
        while( void* slot = getSlot(to_void_ptr.ptr, i++) ) {
#ifdef OBJECTSLOTS_THREADED
            std::thread t( [slot, args...]() {

                (*reinterpret_cast<Base<void, Args...>*>(slot))(args...);
            });
            t.detach();
#else
            (*reinterpret_cast<Base<void, Args...>*>(slot))(args...);
#endif
        }
#ifdef OBJECTSLOTS_THREAD_SAFE
        releaseLock(lock);
#endif
    }
private:
    struct impl;
    impl* impl_;

    void* getSlot(void*, int);
    void slotStore(void*, void*);
    void slotRemove(void*, void*);
#ifdef OBJECTSLOTS_THREAD_SAFE
    LockP acquireLock();
    void releaseLock(LockP);
#endif
};

} // end namespace Slots

#endif //_OBJECTSLOTS_HPP_