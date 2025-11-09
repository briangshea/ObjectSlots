# ObjectSlots: A C++ Signal-Slot Library

`ObjectSlots` is a lightweight, C++17 library that provides a flexible signal-slot mechanism for decoupled communication between objects. It allows a class (the "emitter") to send signals that one or more functions or methods (the "slots") can connect to and be called when the signal is emitted.

This pattern is particularly useful for event-driven programming, helping to reduce direct dependencies and create more modular and maintainable code.

## Features

*   **Flexible Slot Types**: Connect signals to member functions, free (global) functions, and lambdas.
*   **Type-Safe**: Signal and slot signatures are checked at compile time.
*   **Decoupling**: Emitters do not need to know anything about the receivers connected to their signals.
*   **Multiple Connections**: A single signal can be connected to multiple slots.
*   **Easy to Use**: Simple `bind`, `unbind`, and `emit` API.

## How It Works

1.  Create an "emitter" class that inherits from `ObjectSlots::ObjectSlots`.
2.  Define signals within the emitter class as regular member functions.
3.  In the implementation of your signal methods, call the protected `emit()` method, passing a pointer to the signal itself and any arguments.
4.  `bind()` slots (member functions, free functions, or lambdas) to a signal on an instance of the emitter.
5.  When the signal method is called on the emitter, all bound slots are invoked with the provided arguments.

## Building and Linking with CMake

The provided `CMakeLists.txt` sets up the project. To use `ObjectSlots` in your own CMake project, you can include it as a subdirectory.

### Example Project Structure

```
MyProject/
├── CMakeLists.txt
├── main.cpp
└── libs/
    └── ObjectSlots/      <-- Clone this repository here
        ├── CMakeLists.txt
        ├── include/
        └── src/
```

### Example `CMakeLists.txt` for Your Application

Here is an example `CMakeLists.txt` for an executable that links against `ObjectSlots`.

```cmake
cmake_minimum_required(VERSION 3.15)
project(MyAwesomeApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the ObjectSlots library from the 'libs' directory
# This makes the 'ObjectSlots' target available.
add_subdirectory(libs/ObjectSlots)

# Add your executable
add_executable(MyAwesomeApp main.cpp)

# Link your executable against the ObjectSlots library
# This also handles include directories automatically.
target_link_libraries(MyAwesomeApp PRIVATE ObjectSlots)
```

## Usage Examples

The following examples demonstrate how to define signals and connect different kinds of slots.

### Basic Setup

First, include the main header and define an emitter class that inherits from `ObjectSlots::ObjectSlots`.

```cpp
#include <iostream>
#include "ObjectSlots/ObjectSlots.hpp"

// 1. Define an Emitter class
class Sensor : public ObjectSlots::ObjectSlots {
public:
    // Define a signal. The implementation calls `emit`.
    void valueChanged(int newValue, float temperature) {
        std::cout << "[Sensor] Emitting valueChanged signal...\n";
        emit(&Sensor::valueChanged, newValue, temperature);
    }
};

// 2. Define a Receiver class with a member function slot
class Display {
public:
    void onValueChanged(int value, float temp) {
        std::cout << "  -> [Display] Member slot received: Value=" << value << ", Temp=" << temp << " C\n";
    }
};

// 3. Define a free function slot
void loggerFunction(int value, float temp) {
    std::cout << "  -> [Logger] Free function received: Value=" << value << ", Temp=" << temp << " C\n";
}
```

### Connecting and Emitting

Now, you can create instances and connect the slots to the signal.

```cpp
int main() {
    Sensor sensor;
    Display display;

    // --- Binding Slots ---

    // Bind a member function slot
    sensor.bind(&Sensor::valueChanged, &display, &Display::onValueChanged);

    // Bind a free function slot
    sensor.bind(&Sensor::valueChanged, &loggerFunction);

    // Bind a lambda function slot
    sensor.bind(&Sensor::valueChanged,  {
        std::cout << "  -> [Lambda] Lambda slot received: Value=" << value << ", Temp=" << temp << " C\n";
    });

    // --- Emitting a Signal ---
    // This will invoke all three bound slots.
    sensor.valueChanged(42, 25.5f);

    // --- Unbinding a Slot ---
    std::cout << "\n[Sensor] Unbinding the display's slot.\n";
    sensor.unbind(&display, &Display::onValueChanged);

    // This will now only invoke the free function and the lambda.
    sensor.valueChanged(99, 26.1f);

    return 0;
}
```

### Expected Output

```
[Sensor] Emitting valueChanged signal...
  -> [Display] Member slot received: Value=42, Temp=25.5 C
  -> [Logger] Free function received: Value=42, Temp=25.5 C
  -> [Lambda] Lambda slot received: Value=42, Temp=25.5 C

[Sensor] Unbinding the display's slot.

[Sensor] Emitting valueChanged signal...
  -> [Logger] Free function received: Value=99, Temp=26.1 C
  -> [Lambda] Lambda slot received: Value=99, Temp=26.1 C
```
