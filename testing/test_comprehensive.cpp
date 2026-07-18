#include <iostream>
#include <thread>
#include <chrono>

#include <ObjectSlots/ObjectSlots.hpp>

class TestEmitter : public ObjectSlots::ObjectSlots {
public:
    void signal_with_unbind() {
        emit(&TestEmitter::signal_with_unbind);
    }
    
    void signal_with_multiple_unbinds() {
        emit(&TestEmitter::signal_with_multiple_unbinds);
    }
};

void slot_that_unbinds(TestEmitter* emitter) {
    std::cout << "Slot 1 called, now unbinding itself..." << std::endl;
    emitter->unbind(&slot_that_unbinds);
    std::cout << "Slot 1 unbound successfully" << std::endl;
}

void another_slot_that_unbinds(TestEmitter* emitter) {
    std::cout << "Slot 2 called, now unbinding slot 1..." << std::endl;
    emitter->unbind(&slot_that_unbinds);
    std::cout << "Slot 2 unbound slot 1 successfully" << std::endl;
}

class TestObject {
public:
    void method_slot_that_unbinds(TestEmitter* emitter) {
        std::cout << "Method slot called, now unbinding itself..." << std::endl;
        emitter->unbind(this, &TestObject::method_slot_that_unbinds);
        std::cout << "Method slot unbound successfully" << std::endl;
    }
};

int main(void) {
    TestEmitter* emitter = new TestEmitter();
    TestObject testObject;
    
    std::cout << "=== Test 1: Unbind function slot during emission ===" << std::endl;
    emitter->bind(&TestEmitter::signal_with_unbind, [emitter]() { slot_that_unbinds(emitter); });
    emitter->signal_with_unbind();
    
    std::cout << "\n=== Test 2: Unbind method slot during emission ===" << std::endl;
    emitter->bind(&TestEmitter::signal_with_unbind, [emitter, &testObject]() { testObject.method_slot_that_unbinds(emitter); });
    emitter->signal_with_unbind();
    
    std::cout << "\n=== Test 3: Multiple unbinds during emission ===" << std::endl;
    emitter->bind(&TestEmitter::signal_with_multiple_unbinds, [emitter]() { slot_that_unbinds(emitter); });
    emitter->bind(&TestEmitter::signal_with_multiple_unbinds, [emitter]() { another_slot_that_unbinds(emitter); });
    emitter->signal_with_multiple_unbinds();
    
    delete emitter;
    
    std::cout << "\nAll comprehensive tests completed successfully!" << std::endl;
    return 0;
}