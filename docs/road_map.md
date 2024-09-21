# Road Map

## Intermediate Representation Emitter

- [ ] Arrays of generic items without a statically specified size
	- For CC targets, solidify into wide arrays
- [ ] `inout` and `out` qualifiers for parameters
- [ ] Uniform variables and buffers (GLSL)
- [ ] Storage buffer objects (GLSL)
- [x] Image and sampler objects (GLSL)
- [ ] For loop control flow
- [ ] Match-case control flow or equivalent
- [ ] Target conditional code
	- Resolved and optimized during compilation
- [ ] Reflection information for generating descriptor sets
	- Layout types and locations
	- Buffer, image, and sampler binding locations
- [x] Flat layout outputs

## Examples

- [x] False coloring of triangle meshes using arrays
- [ ] Specialized glyph rendering through runtime shader generation
	- Parallelized version as well, compiling multiple shaders at the same time