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
- [ ] Partial evaluation of callables
- [ ] Duplicate bindings with DEC optimization

## Pipeline Generation

- [ ] Vertex layout inference
	- Generating a buffer with this information from a variant mesh
- [ ] Descriptor set layout binding generation

## Editor

- [x] Visualization for surface normals
- [x] Visualization for surface texture coordinates
- [x] Visualization for depth
- [x] Visualization for individual triangles
- [x] Visualization for textured and untextured albedo
- [ ] Visualization for object roughness

## Engine

- [x] Texture caching to avoid duplicate texture loading

## Examples

- [x] False coloring of triangle meshes using arrays
- [x] Custom color palette rendering of meshes with HSV
- [ ] Specialized glyph rendering through runtime shader generation
	- Parallelized version as well, compiling multiple shaders at the same time