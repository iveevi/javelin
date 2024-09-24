# Road Map

## Intermediate Representation Emitter

- [x] Generic type system
- [x] Solidifying types into native `C++` data
- [x] Aggregate structures
- [x] Vector types
- [ ] Arrays of generic items without a statically specified size
	- For CC targets, solidify into wide arrays
- [ ] `inout` and `out` qualifiers for parameters
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
- [ ] Casting operations

## Examples

- [x] False coloring of triangle meshes using arrays
- [ ] Specialized glyph rendering through runtime shader generation
	- Parallelized version as well, compiling multiple shaders at the same time