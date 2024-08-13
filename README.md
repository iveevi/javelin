# Javelin

A realtime rendering framework.

# Roadmap

## Intermediate Representation Emitter (IRE)

- [ ] Common
  - [x] Revised implementation of vector types with limited swizzling
  - [x] Uniform layout concept for custom structs
  - [x] Persistent storage for mutating variables
  - [x] Structs and nested structs
  - [ ] Standard floating point functions (sin, cos, exp, log, etc.)
  - [ ] Standard vector operations (dot, cross, etc.)
  - [ ] Named structures for synthesizing compatible code
- [ ] GLSL specific
  - [x] Layout input and output variables
  - [x] Push constants
  - [ ] Uniform buffers

## IR Synthesis

- [ ] GLSL synthesis
  - [x] Structures and nested structures
  - [x] Layout inputs and outputs
  - [x] Push constants with and without structs
  - [ ] Uniform and storage buffers
  - [ ] Image formats and textures
- [ ] C++ synthesis
  - [x] Layout inputs converted to parameters
  - [x] Tuple return types for multiple layout outputs
  - [x] Struct generation for jvl primitive types (vec2, ivec4, etc.)
- [ ] SPIRV synthesis

## Host-side Rendering

- [x] Thread pool implementation
- [ ] GGX material description
- [ ] BVH construction methods
- [ ] Naive path tracing
- [ ] Path tracing with BRDF importance sampling
- [ ] Path tracing with NEE
- [ ] MIS path tracer

## Testing

- [x] IR emitter tests to ensure correct IR is being produced (simple)
- [x] IR synthesis tests to ensure that C++ synthesized by kernels is compileable (simple)
