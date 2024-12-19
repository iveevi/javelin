# Road Map

## Thunder Backend

- [ ] Persistent variables to store computations

## Intermediate Representation Emitter

- [x] Generic type system
- [x] Solidifying types into native `C++` data
- [x] Aggregate structures
- [x] Vector types
- [ ] Arrays of generic items without a statically specified size
	- For CC targets, solidify into wide arrays
- [x] `inout`, `out`, and `in` qualifiers for parameters
- [x] Uniform variables and buffers (GLSL)
- [x] Storage buffer objects (GLSL)
	- read-write, read only, write only
- [x] Image and sampler objects (GLSL)
- [x] For loop control flow
	- Equivalent declarations for `break` and `continue` statements
- [ ] Match-case control flow or equivalent
- [ ] Target conditional code
	- Resolved and optimized during compilation
- [ ] Reflection information for generating descriptor sets
	- Layout types and locations
	- Buffer, image, and sampler binding locations
- [x] Shared variables
- [x] Layout intputs and outputs
	- With interpolation modes: `flat`, `noperspective`, and `smooth`
- [x] Push constants with offsets
- [x] Casting operations
- [ ] Partial evaluation of callables
- [ ] Duplicate bindings with dead code elimination
- [x] Lambdas with capture
- [ ] Deferred declarations for runtime specialization
- [ ] CUDA support with NVRTC

## Type Safe Graphics

- [x] Layout input and output inference
- [x] Type and binding checking for layout inputs and outputs in adjacent stages
- [x] Compiler warnings (via deprecation) for unused layout outputs
- [ ] Descriptor set layout binding generation
- [ ] Mesh shaders

## Examples

- [x] False coloring of triangle meshes using arrays
- [x] Custom color palette rendering of meshes with HSV
- [x] Particle systems with and without compute shaders
- [ ] Specialized glyph rendering through runtime shader generation
	- Parallelized version as well, compiling multiple shaders at the same time
- [ ] Barnes-Hutt simulation for N-body particle simulation
- [ ] Simple raytracing pipeline
- [ ] Ray query pipeline with hybrid rasterization
