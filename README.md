# Javelin

Javelin is a system for **graphics programming**; to this serve this purpose, it aims to provide the following:

* A **just-in-time (JIT) compilation framework** for shader and kernel programming
* Common graphics constructs for fast prototyping of visual applications and research
* A programming language for end-to-end graphics applications, shaders included
* An editor for managing and viewing 3D scenes

Many components of Javelin are currently a work in progress.

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

### Aside: Why bother with JIT shader compilation?

In the particular context of game engines in computer science, shader permutations present a notable problem. Namely, suppose that a material in the game engine has a set of properties, and that it is desired to optimize away some of these options when compiling the final material shader. If these are, for example, $N$ boolean options in the shader, there will be a total of $2^N$ combinations of unique shaders which need to be compiled. This is impractical to do beforehand in most cases, so common solutions include:

1. Inject preprocessor directives (usually) and write code for both cases of each option, for every option.
2. Generate the final shader code during runtime by concatenating strings of code, embedding the right section of code for each option.
3. Write duplicate and specialized shaders for each and every permutation that is known to be used (usually a smaller subset of the $2^N$ permutations).

For the programmer, none of these options are sufficiently ergenomical, as they can quickly lead to technical debt as shaders become more complex.

Beside this reason, it may be be useful to generate programs at runtime, depending on the users input. As an example, take an application in neural graphics in which the user can import and select differing configurations of Neural Radiance Fields (e.g Vanilla, Instant NGP, Zip NeRF, etc. and at different sizes). It could be useful to generate the optimal rendering shader code for each configuration as it is selected, rather than having to restrict configurations to a fixed size specification.

Along this reasoning, another potential feature that runtime generation enables is the possibility of code transformation, to provide language features not native to the target code. For example, as differentiable programming is becoming more popular, some languages like [slang](https://research.nvidia.com/labs/rtr/publication/bangaru2023slangd/) have implemented differentiable programming as a native feature. However, this is not the case with other languages like CUDA. With a multi-target solution, such as the IRE proposed here, language features can be granted to those which do not natively have them.

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