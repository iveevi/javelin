---
title: Callables in Javelin
---

# Overview

Callables are objects which interface between ordinary C++ functions and the Javelin IRE system, and provide an abstraction for subroutines in the underlying Atom IR. In the backend, they are represented by their own vector of atoms, which means that callables can undergo various code transformations indepedently of each other and the global emitter.

# Implementation

Under the hood, a callable has a type of the form:

```cpp
template <typename R, typename ... Args>
struct callable_t;
```

Here, `R` is the return type of the callable, and `Args...` are its expected inputs. This type provides a call operator with the signature `R operator()(const Args &...)`, which will create an argument list via an `atom::List` list chain, that holds tags to the passed inputs. The expectation is that the callable is invoked only during the IRE lifetime of all inputs.

# Usage

The current implementation enables passing ordinary functions, lambda functions, and functor objects into the `callable(...)` method:

```cpp
// Using an ordinary function...
f32 square(f32 x)
{
	return pow(x, 2);
}

auto cbl1 = callable(square);              // cbl1 can be invoked as cbl1(<f32>)

// Using a lambda function...
auto lambda = [&](f32 x)
{
	return square(x) - x + 1;
};

auto cbl2 = callable(lambda);              // cbl2 can be invoked as cbl2(<f32>)

// Using a functor object...
struct functor {
	i32 base;

	i32 operator()(i32 x, i32 y) {
		return mod(x * x - y * y, base);
	}
};

auto fnc = functor(3);

auto cbl3 = callable(fnc);                 // cbl3 can be invoked as cbl3(<i32>, <i32>)
```

For invocables that do not return a result (e.g. `void` return types), an explicit return type must be specified when using the method:

```cpp
// Revisting the lambda example above...
auto lambda = [&](f32 x)
{
	returns(square(x) - x + 1);
};

auto cbl = callable <f32> (lambda);        // cbl can be invoked as cbl2(<f32>)
```