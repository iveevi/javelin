# Javelin

Javelin is a **just-in-time (JIT) compilation framework** for shader and kernel programming.

Many components of Javelin are a work in progress. See the [road map](docs/road_map.md) to get an idea of what is coming next.

![GoogleTest](https://github.com/iveevi/javelin/actions/workflows/ci.yml/badge.svg)

## Intermediate Representation Emitter

The Intermediate Representation Emitter (IRE) is Javelin's JIT framework for runtime shader and kernel generation. It aims to provide a seamless C++ experience when working with graphics shaders as well as multi-platform kernels.

For example, the following C++ code demonstrates how to use the IRE to write a simple vertex-fragment shader that simply projects vertices and sets the surface color to a static value.

```cpp
// Model-view-projection structure
struct MVP {
    // Typical matrix structures
    mat4 model;
    mat4 view;
    mat4 proj;

    // Define a method to perform the projection
    vec4 project(vec3 position) {
        return proj * (view * (model * vec4(position, 1.0)));
    }

    // Define the layout structure of this struct for generated shader code
    auto layout() const {
        return uniform_layout(
            "MVP",
            named_field(model),
            named_field(view),
            named_field(proj)
        );
    }
};

// Shader kernels
void vertex()
{
    // Vertex inputs
    layout_in <vec3> position(0);

    // Projection information
    push_constant <MVP> mvp;

    // Projecting the vertex
    gl_Position = mvp.project(position);
}

void fragment(float3 color)
{
    // Resulting fragment color
    layout_out <vec4> fragment(0);

    // Set the color
    fragment = vec4(color.x, color.y, color.z, 1);
}
```

The corresponding GLSL source code can be generated as follows:

```cpp
auto vs_callable = procedure("main") // Name of the kernel
	<< vertex;

// Instantiate a fragment shader that colors the surface RED
auto fs_callable = procedure("main")
	<< std::make_tuple(float3(1, 0, 0)) // Pass arguments to the fragment function
	<< fragment;

std::string vertex_shader = link(vs_callable).generate_glsl();
std::string fragment_shader = link(fs_callable).generate_glsl();
```

Other, more complex examples can be found in the `examples` directory.

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