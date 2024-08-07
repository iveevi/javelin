# Javelin

A realtime rendering framework.

# Roadmap

## Intermediate Representation Emitter (IRE)

- [x] Revised implementation of vector types with limited swizzling
- [x] Uniform layout concept for custom structs
- [x] Layout input and output variables
- [x] Push constants
- [x] Persistent storage for mutating variables.
- [x] Structure types.
- [ ] Standard floating point functions (sin, cos, exp, log, etc.)
- [ ] Standard vector operations (dot, cross, etc.)
- [ ] Named structures for synthesizing compatible code
- [ ] Uniform buffers
- [ ] Simple SPIRV synthesis

## Host-side Rendering

- [x] Thread pool implementation
- [ ] GGX material description
- [ ] BVH construction methods
- [ ] Naive path tracing
- [ ] Path tracing with BRDF importance sampling
- [ ] Path tracing with NEE
- [ ] MIS path tracer

## Testing

- [ ] Local filecheck-like methods for comparing output
- [ ] Generate tests for simple cases