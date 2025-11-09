#include <iostream>

#include <ObjectSlots/ObjectSlots.hpp>

class Test : public ObjectSlots::ObjectSlots {
public:
    void signal_hello(const char* message) {
        emit( &Test::signal_hello, message );
    }

    void signal_other_signal(void) {
        emit( &Test::signal_other_signal );
    }

    void operator()(const char* message) {
        signal_hello(message);
    }
};

void onHello(const char* message)
{
    std::cout << "Function: " << message << std::endl;
}

void onBadHello(int incorrect) {

}

void otherSIgnalHandler(void) {

}

class TestObject {
public:
    void onHelloMethod(const char* message) {
        std::cout << "Method: " << message << std::endl;
    }
};

int main(void) {

    Test* test = new Test();
    TestObject testObject;

    test->bind( &Test::signal_hello, &onHello );
    test->bind( &Test::signal_hello, &testObject, &TestObject::onHelloMethod );
    test->bind( &Test::signal_other_signal, &otherSIgnalHandler );
    
    auto lambda = [](const char* message) {
            std::cout << "Lambda: " << message << std::endl; 
        };

    test->bind( 
        &Test::signal_hello, 
        lambda
    );

    const char* test_string = "Hello World";
    test->operator()(test_string);

    test->unbind( &onHello );
    test->unbind( &testObject, &TestObject::onHelloMethod );
    test->unbind( &otherSIgnalHandler );

    //test->unbind( lambda );
    //test->operator()(test_string);

    delete test;

    return 0;
}