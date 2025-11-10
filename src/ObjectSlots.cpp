#include "ObjectSlots/ObjectSlots.hpp"

#include <map>
#include <list>

#ifdef OBJECTSLOTS_THREAD_SAFE
#include <mutex>
#include <shared_mutex>
#define READLOCK()  std::shared_lock<std::shared_mutex> lock(impl_->mutex)
#define WRITELOCK() std::unique_lock<std::shared_mutex> lock(impl_->mutex)
#else
#define READLOCK()
#define WRITELOCK()
#endif
#include <vector>

namespace ObjectSlots {

struct ObjectSlots::impl {
    using SlotList = std::vector<void*>;
    using SignalMap = std::map<const void*, SlotList>;
    using const_iterator = SignalMap::const_iterator;
    using iterator = SignalMap::iterator;

    inline const_iterator begin() const { return Signals.begin(); }
    inline const_iterator end() const { return Signals.end(); }
    inline iterator find(void* signal) { return Signals.find(signal); }

    inline const_iterator cbegin() const { return Signals.cbegin(); }
    inline const_iterator cend() const { return Signals.cend(); }

    inline iterator begin() { return Signals.begin(); }
    inline iterator end() { return Signals.end(); }

    inline iterator erase(iterator it) { return Signals.erase(it); }
    inline iterator erase(const_iterator it) { return Signals.erase(it); }
    inline iterator erase(iterator first, iterator last) { return Signals.erase(first, last); }

    inline SlotList& operator[](void* &signal ) { return Signals[signal]; }
    inline SlotList& operator[](void* &&signal ) { return Signals[signal]; }

    ~impl() {
        for( auto it = Signals.begin(); it != Signals.end(); ++it ) {
            for( auto i = it->second.begin(); i != it->second.end(); ++i ) {
                delete reinterpret_cast<Base<void>*>(*i);
            }
        }
    }

#ifdef OBJECTSLOTS_THREAD_SAFE
    mutable std::shared_mutex mutex;
#endif

private:
    SignalMap Signals;
};

ObjectSlots::ObjectSlots() : impl_(new impl()) { }

ObjectSlots::~ObjectSlots() {
    WRITELOCK();
    delete impl_;
}

void* ObjectSlots::getSlot(void* signal, int index) {
    const auto& slots = impl_->find(signal);
    if( slots != impl_->end() && index < slots->second.size() ) {
        return slots->second[index];
    }
    return nullptr;
}

void ObjectSlots::slotStore(void* signal, void* slot) {
    WRITELOCK();
    (*impl_)[signal].emplace_back(slot);
}

void ObjectSlots::slotRemove(void* object, void* slot) {
    WRITELOCK();
    // mode:
    // 0 : no object or method (should never happen)
    // 1 : slot but no object
    // 2 : object but no slot
    // 3 : object and slot
    const int mode = (object!=nullptr)<<1 | (slot!=nullptr);
    for( auto it = impl_->begin(); it != impl_->end(); ) {
        for( auto i = it->second.begin(); i != it->second.end();) {
            Base<void> *SlotOrMethod = reinterpret_cast<Base<void>*>(*i);
            bool remove = false;
            switch (mode)
            {
            case 1: remove = (SlotOrMethod->callback() == slot); break;
            case 2: remove = (SlotOrMethod->object() == object); break;
            case 3: remove = (SlotOrMethod->callback() == slot && SlotOrMethod->object() == object); break;
            default: break;
            }

            if(remove) {
                i = it->second.erase(i);
                delete SlotOrMethod;
                continue;
            }
            ++i;
        }
        if ( it->second.empty() ) {
            it = impl_->erase(it);
            continue;
        }
        ++it;
    }
}

#ifdef OBJECTSLOTS_THREAD_SAFE
ObjectSlots::LockP ObjectSlots::acquireLock() {
    return new std::shared_lock<std::shared_mutex>(impl_->mutex);
}
void ObjectSlots::releaseLock(ObjectSlots::LockP lock) {
    delete reinterpret_cast<std::shared_lock<std::shared_mutex>*>(lock);
}
#endif

} // end namespace Slots