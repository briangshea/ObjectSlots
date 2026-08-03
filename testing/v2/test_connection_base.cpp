#include <catch2/catch_test_macros.hpp>
#include <ObjectSlots/ObjectSlots.hpp>
#include <string>
#include <vector>
#include <memory>

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

TEST_CASE("TestBaseConnectionClass", "[v2]") {
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

TEST_CASE("TestConnectionStorageInContainer", "[v2]") {
    // Create an emitter instance
    DataEmitter emitter;
    
    // Create a logger that binds to the emitter
    DataLogger logger(emitter);
    
    // Test storing connections in containers using the new base class
    std::vector<std::shared_ptr<Signals::v2::ConnectionBase>> connections;
    
    // Bind multiple connections and store them in container
    auto conn1 = emitter.Event.bind([](int value) {
        // First handler
    });
    
    auto conn2 = emitter.Event.bind([](int value) {
        // Second handler  
    });
    
    // Store connections using base class pointers
    connections.push_back(std::make_shared<Signals::v2::_ConnectionToken<void(int)>>(conn1));
    connections.push_back(std::make_shared<Signals::v2::_ConnectionToken<void(int)>>(conn2));
    
    // Verify we can call methods on the base class
    REQUIRE(connections.size() == 2);
    
    // Test that we can call virtual methods through base class
    for (auto& conn : connections) {
        REQUIRE(conn->isValid() == true);
        conn->block();
        conn->unblock();
        conn->disconnect();
    }
}

TEST_CASE("TestMixedConnectionTypes", "[v2]") {
    // Create an emitter instance
    DataEmitter emitter;
    
    // Test with different signal types
    Signals::v2::Signal<void(std::string)> stringSignal;
    Signals::v2::Signal<int(int, int)> intSignal;
    
    // Bind connections to different signals
    auto conn1 = emitter.Event.bind([](int value) {
        // Handler for int signal
    });
    
    auto conn2 = stringSignal.bind([](const std::string& str) {
        // Handler for string signal
    });
    
    auto conn3 = intSignal.bind([](int a, int b) -> int {
        return a + b;
    });
    
    // Store all connections in one container using base class
    std::vector<std::shared_ptr<Signals::v2::ConnectionBase>> allConnections;
    allConnections.push_back(std::make_shared<Signals::v2::_ConnectionToken<void(int)>>(conn1));
    allConnections.push_back(std::make_shared<Signals::v2::_ConnectionToken<void(std::string)>>(conn2));
    allConnections.push_back(std::make_shared<Signals::v2::_ConnectionToken<int(int, int)>>(conn3));
    
    REQUIRE(allConnections.size() == 3);
    
    // Test that we can call methods on all connections
    for (auto& conn : allConnections) {
        REQUIRE(conn->isValid() == true);
        conn->disconnect();
    }
}

TEST_CASE("TestBackwardCompatibility", "[v2]") {
    // Create an emitter instance
    DataEmitter emitter;
    
    // Old usage pattern should still work exactly as before
    Signals::v2::Signal<void(int)>::Connection conn = emitter.Event.bind([](int value) {
        // Handler code
    });
    
    // This should compile and work the same way as before
    REQUIRE(conn.isValid() == true);
    conn.block();
    conn.unblock();
    conn.disconnect();
    
    // Test that we can still use it in RAII fashion
    Signals::v2::Signal<void(int)>::Connection conn2 = emitter.Event.bind([](int value) {
        // Another handler
    });
    
    REQUIRE(conn2.isValid() == true);
}