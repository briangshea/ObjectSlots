#include <catch2/catch_test_macros.hpp>
#include <ObjectSlots/ObjectSlots.hpp>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>

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

// Stress test: High rate event emission with frequent bind/unbind operations
TEST_CASE("StressTest_HighRateEmissionWithBindUnbind", "[v2][stress]") {
    DataEmitter emitter;
    
    const int numEvents = 100000; // 100K events
    const int numBindings = 1000; // 1000 bind/unbind cycles
    
    std::atomic<int> totalEvents{0};
    
    // Bind a handler that increments the counter
    auto token = emitter.Event.bind([&totalEvents](int value) {
        totalEvents.fetch_add(1, std::memory_order_relaxed);
        volatile int dummy = value; // Prevent optimization
        (void)dummy;
    });
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Emit events with frequent bind/unbind operations
    for (int i = 0; i < numEvents; ++i) {
        // Perform bind/unbind operations at regular intervals
        if (i % (numEvents / numBindings) == 0 && i > 0) {
            // Unbind the handler
            token.disconnect();
            
            // Rebind a new handler
            token = emitter.Event.bind([&totalEvents](int value) {
                totalEvents.fetch_add(1, std::memory_order_relaxed);
                volatile int dummy = value;
                (void)dummy;
            });
        }
        
        emitter.emitEvent(i);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    CAPTURE(duration.count());
    CAPTURE(totalEvents.load());
    
    // Should complete without segfaulting
    REQUIRE(totalEvents.load() == numEvents);
    REQUIRE(duration.count() < 5000000); // Should complete in less than 5 seconds
}

// Stress test: High rate event emission with multiple concurrent bind/unbind operations
TEST_CASE("StressTest_ConcurrentBindUnbindOperations", "[v2][stress]") {
    DataEmitter emitter;
    
    const int numEvents = 50000; // 50K events
    const int numThreads = 4;
    const int eventsPerThread = numEvents / numThreads;
    
    std::atomic<int> totalEvents{0};
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create multiple threads that perform bind/unbind operations while emitting events
    std::vector<std::thread> threads;
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&emitter, &totalEvents, eventsPerThread, t]() {
            // Each thread will have its own token
            Signals::Signal<void(int)>::Connection localToken;
            
            // Bind a handler for this thread
            localToken = emitter.Event.bind([&totalEvents](int value) {
                totalEvents.fetch_add(1, std::memory_order_relaxed);
                volatile int dummy = value;
                (void)dummy;
            });
            
            // Emit events
            for (int i = 0; i < eventsPerThread; ++i) {
                emitter.emitEvent(i + t * eventsPerThread);
                
                // Occasionally unbind and rebind to stress the system
                if (i % 100 == 0 && i > 0) {
                    localToken.disconnect();
                    localToken = emitter.Event.bind([&totalEvents](int value) {
                        totalEvents.fetch_add(1, std::memory_order_relaxed);
                        volatile int dummy = value;
                        (void)dummy;
                    });
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    CAPTURE(duration.count());
    CAPTURE(totalEvents.load());
    
    // Should complete without segfaulting
    REQUIRE(totalEvents.load() == numEvents);
    REQUIRE(duration.count() < 5000000); // Should complete in less than 5 seconds
}

// Stress test: High rate event emission with random bind/unbind operations
TEST_CASE("StressTest_RandomBindUnbindOperations", "[v2][stress]") {
    DataEmitter emitter;
    
    const int numEvents = 10000; // 10K events
    const int maxHandlers = 100; // Maximum number of handlers to manage
    
    std::atomic<int> totalEvents{0};
    std::vector<Signals::Signal<void(int)>::Connection> tokens;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 100);
    
    // Emit events with random bind/unbind operations
    for (int i = 0; i < numEvents; ++i) {
        // Occasionally add/remove handlers to stress the system
        if (dis(gen) < 5) { // 5% chance of adding/removing a handler
            if (tokens.size() < maxHandlers) {
                // Add a new handler
                tokens.push_back(emitter.Event.bind([&totalEvents](int value) {
                    totalEvents.fetch_add(1, std::memory_order_relaxed);
                    volatile int dummy = value;
                    (void)dummy;
                }));
            } else if (!tokens.empty()) {
                // Remove a random handler
                size_t index = dis(gen) % tokens.size();
                tokens.erase(tokens.begin() + index);
            }
        }
        
        emitter.emitEvent(i);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    CAPTURE(duration.count());
    CAPTURE(totalEvents.load());
    CAPTURE(tokens.size());
    
    // Should complete without segfaulting
    REQUIRE(totalEvents.load() == numEvents);
    REQUIRE(duration.count() < 2000000); // Should complete in less than 2 seconds
}

// Stress test: High rate event emission with rapid bind/unbind operations
TEST_CASE("StressTest_RapidBindUnbindOperations", "[v2][stress]") {
    DataEmitter emitter;
    
    const int numEvents = 50000; // 50K events
    
    std::atomic<int> totalEvents{0};
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Rapid bind/unbind operations
    for (int i = 0; i < numEvents; ++i) {
        // Bind a handler
        auto token = emitter.Event.bind([&totalEvents](int value) {
            totalEvents.fetch_add(1, std::memory_order_relaxed);
            volatile int dummy = value;
            (void)dummy;
        });
        
        // Emit event immediately
        emitter.emitEvent(i);
        
        // Unbind immediately after emission
        token.disconnect();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    CAPTURE(duration.count());
    CAPTURE(totalEvents.load());
    
    // Should complete without segfaulting
    REQUIRE(totalEvents.load() == numEvents);
    REQUIRE(duration.count() < 5000000); // Should complete in less than 5 seconds
}