---
layout: post
title:  C++ Type Traits & SFINAE 
date:   2025-12-07 13:03:03 +0700
categories: post
---
One of the tasks I need to do when developing Native Windows App Libraries is creating unit tests for each component. That process requires me to heavily use and maintain Interfaces, or abstract classes, in order to Mock the component in test, and therefore I can force insert specific values. That causes such a headache because of the class inheritence explosion *vomit*

A different approach I find interesting is rather than 'inheriting' member variables, it can be done by injecting those variables during initialization (composition). Templates then help me create mocks of some annoying Windows Runtime specific objects like BluetoothAdvertisementWatcher.

Here is some snippets of using Type Traits to enable SFINAE (Substitution Failure is Not an Error), making this kind of Mocking possible.

In this example, I made a digital_input that will be inserted inside a Button class (a study case of Embedded Systems). Here I assume the digital_input is some sort of given library that I need to replace with a Mock. So I created a type trait that gets satisfied iff any type has the init() and read() function in them. I also made an example of a BAD digital_input_mock that does not have the required functions, hence spitting out a compilation error (noice!)

```cpp
// -------------------------------------------
// TYPE TRAIT using SFINAE
// -------------------------------------------
template<typename T, typename = void>
struct is_digital_input : std::false_type {};

template<typename T>
struct is_digital_input<T, std::void_t<
    decltype(std::declval<T>().init()),
    decltype(std::declval<T>().read())>> : std::true_type {};

template<typename T>
constexpr bool is_digital_input_v = 
	is_digital_input<T>::value;

// -------------------------------------------
// BUTTON TEMPLATE, REQUIRES A CORRECT TYPE
// -------------------------------------------
template <typename DIn, typename = std::enable_if_t<is_digital_input_v<DIn>>>
class Button {
public:
	MyButton(DIn *input) : digitalInput_(input) {}

	void init() {
		digitalInput_->init();
		// rest of logic...
	}

	int read() {
		return digitalInput_->read();
	}

private:	
	DIn *digitalInput_;
};

// -------------------------------------------
// DIGITAL INPUT MOCK (GOOD)
// -------------------------------------------
class DigitalInputMock {
public:
	void init() {}
	int read() { return value_; }

	void set_value(int v) { value_ = v; }
private:
	int value_{};
};

void test_good_mock() {
	DigitalInputMock input;
	Button<DigitalInputMock> button(&input);

	input.set_value(42);
	std::println("test_good_mock");
	std::println("button value: {}", button.read());
	assert(button.read() == 42);
}

/**
 * Output:
 * > test_good_mock
 * > button value: 42
 */

// -------------------------------------------
// DIGITAL INPUT MOCK (BAD)
// -------------------------------------------
class MalformedDigitalInputMock { /* no init(), no read() */ };

void test_malformed_mock() {
    MalformedDigitalInputMock malformedInput;
	// SFINAE in action! Compile error]
	std::println("test_malformed_mock");
    Button<MalformedDigitalInputMock> button(&malformedInput); 
}
```

Output:

```bash
/**
main.cpp(281,5): error C2976: 'ButtonWithSfinae': too few template arguments, see declaration of 'ButtonWithSfinae'

```