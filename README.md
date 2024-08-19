# Javelin

A realtime rendering framework.

# Roadmap

## Intermediate Representation Emitter (IRE)

- **Common:**
  - [x] Revised implementation of vector types with limited swizzling
  - [x] Uniform layout concept for custom structs
  - [x] Persistent storage for mutating variables
  - [x] Structs and nested structs
  - [x] Standard vector operations (dot, cross, etc.)
  - [x] Return calls
  - [x] Callable functions from ordinary functions
  - [ ] Standard floating point functions (sin, cos, exp, log, etc.)
  - [ ] Named structures for synthesizing compatible code
  - [ ] Branching (if, else if, else)
  - [ ] While loops (while, do-while)
  - [ ] For loops (for, foreach)
  - [ ] Kernel modules for linking chunks of Atom IR
- **Automatic differentiation:**
  - [ ] Dual type generation through `dual_t` and construction with `dual`
  - [ ] Forward derivative functions of simple callables
- **GLSL specific:**
  - [x] Layout input and output variables
  - [x] Push constants
  - [ ] Uniform buffers

## Code Synthesis

- **GLSL synthesis:**
  - [x] Structures and nested structures
  - [x] Layout inputs and outputs
  - [x] Push constants with and without structs
  - [x] Callable functions with ordinary parameters
  - [ ] Uniform and storage buffers
  - [ ] Image formats and textures
- **C++ synthesis:**
  - [x] Layout inputs converted to parameters
  - [x] Tuple return types for multiple layout outputs
  - [x] Struct generation for jvl primitive types (vec2, ivec4, etc.)
- **SPIRV synthesis:**
  - [ ] Direct translation of basic atoms to SPIRV ASM
- **JIT executable:**
  - [ ] Implementation using `libgccjit`

## CPU Rendering

- [x] Thread pool implementation
- [x] IRE shaders for vertex/fragment pipeline
- [x] BVH construction methods
- [x] Naive path tracing
- [x] Path tracing with BRDF importance sampling
- [ ] GGX material description
- [ ] Path tracing with NEE
- [ ] MIS path tracer

## Testing

- **IRE:**
  - [x] Simple layout in/out check for presence of atoms
  - [ ] Checks for control flow operations
  - [ ] Ensure that uniform compatible structures are preserved
- **Code synthesis:**
  - [x] Simple compatibility test for C++ synthesis
  - [x] Simple compatibility test for GLSL synthesis
