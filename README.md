# Javelin

Javelin is a system for **graphics programming**; to this serve this purpose, it aims to provide the following:

* A **just-in-time (JIT) compilation framework** for shader and kernel programming
* Common graphics constructs for fast prototyping of visual applications and research
* A programming language for end-to-end graphics applications, shaders included
* An editor for managing and viewing 3D scenes

Many components of Javelin are a work in progress. See the [road map](docs/road_map.md) for to get an idea of what is coming next.

![GoogleTest](https://github.com/iveevi/javelin/actions/workflows/ci.yml/badge.svg)

## Intermediate Representation Emitter

The Intermediate Representation Emitter (IRE) is Javelin's JIT framework for runtime shader and kernel generation. It aims to provide a seamless C++ experience when working with graphics shaders as well as multi-platform kernels.

A simple example of using the IRE is the following vertex shader:

```cpp
// Contrived example when 2D input vertices are scale by a fixed amount
void vertex(float scale)
{
    layout_in <vec2> position(0);

    gl_Position = vec4(scale * position, 0, 1);
}

// ...

void compile_pipeline(float scale)
{
    // Generate a kernel object from the shader
    auto kernel = kernel_from_args(vertex, scale);

    // Generat GLSL (450) source code from the kernel
    std::string vertex_shader = kernel.compile(profiles::glsl_450);

    // use <vertex_shader> however it is needed...
}
```

The IRE supports compiling to multiple targets, with more to come in the future:

* GLSL, **Vulkan-style (working)** and OpenGL-style (planned)
* C++, generates **C++11 code (working)**, with OpenMP support (planned)
* Host exectuable code through JIT, with **libgccjit (working)** and LLVM (planned) backends
* SPIRV assembly (planned) and binary (planned)
* CUDA kernels through NVRTX (planned)

All IRE features are provided in the `include/ire/core.hpp` header file. The corresponding library is built into `javelin-ire` using CMake. See the [Building](#building) section for more details.

**Asides:**

[Why bother with JIT shader compilation?](docs/aside/jit_shader_compilation.md)

## Building

This project is configured with CMake and requires C++20. Additional dependencies are listed below:

* libgccjit (14.x.x onwards) for the IRE to load compiled kernels
* GLFW3 for graphics applications
* Vulkan, SPIRV, and glslang for Vulkan specific graphics applications

The following should be sufficient to get started:

```
$ cmake -B build .
$ cmake --build build -j
OR
$ cmake --build build -j -t [target] # if only certains targets are needed
```