#include <iostream>
#include <thread>
#include <chrono>

#include <ObjectSlots/ObjectSlots.hpp>

class TestEmitter : public ObjectSlots::ObjectSlots {
public:
    void signal_with_unbind() {
        emit(&TestEmitter::signal_with_unbind);
    }
};

void slot_that_unbinds(TestEmitter* emitter) {
    // This slot will unbind itself during emission
    std::cout << "Slot called, now unbinding..." << std::endl;
    emitter->unbind(&slot_that_unbinds);
    std::cout << "Unbound successfully" << std::endl;
}

void other_slot() {
    std::cout << "Other slot called" << std::endl;
}

class TestObject {
public:
    void method_slot_that_unbinds(TestEmitter* emitter) {
        std::cout << "Method slot called, now unbinding..." << std::endl;
        emitter->unbind(this, &TestObject::method_slot_that_unbinds);
        std::cout << "Unbound method slot successfully" << std::endl;
    }
};

int main(void) {
    TestEmitter* emitter = new TestEmitter();
    TestObject testObject;
    
    // Test 1: Unbind a function slot during emission
    std::cout << "=== Test 1: Unbind function slot during emission ===" << std::endl;
    emitter->bind(&TestEmitter::signal_with_unbind, [emitter]() { slot_that_unbinds(emitter); });
    emitter->signal_with_unbind();
    
    // Test 2: Unbind method slot during emission
    std::cout << "\n=== Test 2: Unbind method slot during emission ===" << std::endl;
    emitter->bind(&TestEmitter::signal_with_unbind, [emitter, &testObject]() { testObject.method_slot_that_unbinds(emitter); });
    emitter->signal_with_unbind();
    
    delete emitter;
    
    std::cout << "All tests completed successfully!" << std::endl;
    return 0;
}