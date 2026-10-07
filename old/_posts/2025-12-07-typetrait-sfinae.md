---
layout: post
title:  C++ Type Traits & SFINAE 
date:   2025-12-07 13:03:03 +0700
categories: post
author: "Royyan"
tags: cpp
comments: true
---
One of the tasks I need to do when developing Native Windows App Libraries is creating unit tests for each component. That process requires me to heavily use and maintain Interfaces, or abstract classes, in order to Mock the component in test, and therefore I can force insert specific values. That causes such a headache because of the class inheritence explosion *vomit*

A different approach I find interesting is rather than 'inheriting' member variables, it can be done by injecting those variables during initialization (composition). Templates then help me create mocks of some annoying Windows Runtime specific objects like BluetoothAdvertisementWatcher.

Here is some snippets of using Type Traits to enable SFINAE (Substitution Failure is Not an Error), making this kind of Mocking possible.

In this example, I made a digital_input that will be inserted inside a Button class (a study case of Embedded Systems). Here I assume the digital_input is some sort of given library that I need to replace with a Mock. So I created a type trait that gets satisfied iff any type has the init() and read() function in them. I also made an example of a BAD digital_input_mock that does not have the required functions, hence spitting out a compilation error (noice!)

```cpp
// ----------- Type Traits ----------- 
// This is the default fallback of the return Type Trait if after any other template 
// specialization fails
template<typename T, typename = void>
struct is_digital_input_like : std::false_type {};

// Here, declval acts as a silent instance of T, decltype ensures init() and read() exists, 
// resulting into an std::true_type as the final return of template type trait
template<typename T>
struct is_digital_input_like<T, std::void_t<
    decltype(std::declval<T>().init()),
    decltype(std::declval<T>().read())>> : std::true_type {};

// This is a helper to extract the value of the then either std::true_type or false_type
// into the primitive boolean value of `true` or `false`
template<typename T>
constexpr bool is_digital_input_like_v = 
	is_digital_input_like<T>::value;

// ----------- Button Template Class ----------- 
template <typename T, typename = std::enable_if_t<is_digital_input_like_v<T>>>
class Button {
public:
	MyButton(T *input) : m_DigitalInput(input) {}

	void init() {
		m_DigitalInput->init();
		// rest of logic...
	}

	int read() {
		return m_DigitalInput->read();
	}

private:	
	T *m_DigitalInput;
};

// ----------- Digital Input Classes ----------- 
// Digital Input (Real)
class DigitalInput {
public:
	void init() {}
	bool read() { return value_; }

	void set_value(bool v) { value_ = v; }
private:
	bool value_{};
};

// Digital Input (Mock)
class DigitalInputMock {
public:
	void init();
	bool read();
};

auto main() -> int {
    DigitalInputMock digitalInput;
    Button<DigitalInputMock> buttonWithMock(&digitalInput);

	digitalInput.set_value(true);
	std::println("Test Good Mock");
	std::println("button value: {}", button.read() ? "True" : "False");
	assert(button.read() == true);

	return 0;
}
```

Output:

```bash
Test Good Mock
button value: True
# assert passess succesfully
```

```cpp
// ...

// Digital Input (Bad Mock)
class DigitalInputMock_Bad { 
public:
	/* void init();  */ // init() missing!
	bool not_read(); 	// read() missing!
};

auto main() -> int {
    DigitalInputMock_Bad digitalInputBad;
	std::println("Testing Bad Mock");
    Button<DigitalInputMock_Bad> buttonWithBadMock(&digitalInputBad); 
	// SFINAE in action! Resulting in a Compile error caused by the Type Trait returning std::false_type

	return 0;
}
```

Output:

```bash
main.cpp(281,5): error C2976: 'Button': too few template arguments, see declaration of 'Button'
```