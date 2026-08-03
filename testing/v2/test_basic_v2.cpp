#include <catch2/catch_test_macros.hpp>
#include <ObjectSlots/ObjectSlots.hpp>
#include <string>
#include <vector>

// Test class that acts as an emitter with 1 signal
class DataEmitter : public Signals::SignalBase {
public:
    // The signal is public so ANY external component can call .bind() cleanly
    Signals::v2::Signal<void(int)> Event;
    
    // Method to emit an event
    void emitEvent(int value) {
        // Emit using the passkey mechanism - we are inside DataEmitter, so we can create the key
        Event(emit_key(), this, value);
    }
};

// Test class that acts as a data logger that binds to the emitter
class DataLogger {
public:
    // Vector to capture emitted values
    std::vector<int> capturedValues;
    
    // Constructor that binds to an emitter
    DataLogger(DataEmitter& emitter) {
        // Bind a lambda to capture the emitted value
        onEventToken = emitter.Event.bind([this](int value) {
            capturedValues.push_back(value);
        });
    }
    
    // Connection token for RAII cleanup
    Signals::v2::Signal<void(int)>::Connection onEventToken;
};

TEST_CASE("TestEmitterAndLogger", "[v2]") {
    // Create an emitter instance
    DataEmitter emitter;
    
    // Create a logger that binds to the emitter
    DataLogger logger(emitter);
    
    // Generate 1 event with value 42
    emitter.emitEvent(42);
    
    // Capture 1 event and compare values
    REQUIRE(logger.capturedValues.size() == 1);
    REQUIRE(logger.capturedValues[0] == 42);
}

TEST_CASE("TestMultipleEvents", "[v2]") {
    // Create an emitter instance
    DataEmitter emitter;
    
    // Create a logger that binds to the emitter
    DataLogger logger(emitter);
    
    // Generate multiple events
    emitter.emitEvent(10);
    emitter.emitEvent(20);
    emitter.emitEvent(30);
    
    // Capture and compare values
    REQUIRE(logger.capturedValues.size() == 3);
    REQUIRE(logger.capturedValues[0] == 10);
    REQUIRE(logger.capturedValues[1] == 20);
    REQUIRE(logger.capturedValues[2] == 30);
}

TEST_CASE("TestNoSegfaults", "[v2]") {
    // Create an emitter instance
    DataEmitter emitter;
    
    // Create a logger that binds to the emitter
    DataLogger logger(emitter);
    
    // Generate multiple events to ensure no segfaults occur
    for (int i = 0; i < 100; ++i) {
        emitter.emitEvent(i);
    }
    
    // Verify all values were captured correctly
    REQUIRE(logger.capturedValues.size() == 100);
    for (int i = 0; i < 100; ++i) {
        REQUIRE(logger.capturedValues[i] == i);
    }
}