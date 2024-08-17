---
title: Automatic Differentiation (Prototype)
---

# Differential Type System

Certain [base types](types.md) in the IRE system should be able to be promoted to differential types automatically as is needed.

# Syntax (Prototype)

```cpp
void __square(f32 x)
{
	returns(x * x);
}

auto square = callable <f32> (__square);

void shader()
{
	// ... global variable definitions, other code

	dtype_t <f32> dx = dual_t(1.0, 0.5);       // sets dx.primal to 1.0
	                                           // zero initializes dx.dual to 0.5
	
	dtype_t <f32> dsquare = dfwd(square)(dx);  // dfwd returns the forward derivative of square
	                                           // the return type of dfwd(square) is the differential type of f32
}
```

# Roadmap

- [ ] Type system feature to promote base types into differential types
- [ ] Apply differentiation operations to atoms in both forward and backward passes
- [ ] Complete automatic differentiation of callables
- [ ] Automatic differentiation of shader programs and kernels