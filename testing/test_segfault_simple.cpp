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

int main(void) {
    TestEmitter* emitter = new TestEmitter();
    
    // Bind a lambda that will unbind itself during emission
    auto lambda = [emitter]() {
        std::cout << "Lambda slot called, now unbinding..." << std::endl;
        emitter->unbind(&slot_that_unbinds);
        std::cout << "Unbound successfully" << std::endl;
    };
    
    emitter->bind(&TestEmitter::signal_with_unbind, lambda);
    
    std::cout << "Calling signal that will unbind during emission..." << std::endl;
    emitter->signal_with_unbind();
    
    delete emitter;
    
    std::cout << "Test completed successfully!" << std::endl;
    return 0;
}