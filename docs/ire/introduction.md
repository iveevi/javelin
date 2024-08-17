---
title: Intermediate Representation Emitter (IRE)
---

# About

The Javelin IRE framework aims to bring shader programming and general purpose GPU computing natively to the C++ programming language. Rather than creating a dialect of C++ with a new compiler system, we enable these features using only the existing type system, meta-programming, and operator overloading capabilities present in C++ itself.

This document describes the design choices taken to establish a framework to make this possible. Later documents will dive into more details about specifics of the IRE:

- [Atom IR Specification](atom.md)
- [Type System](types.md)
- [Callables in Javelin](callable.md)
- [Automatic Differentiation (Prototype)](autodiff.md)

# Overview

At the highest level, the goal of the IRE module is to translate syntactically seamless C++ code into arbitrary targets for downstream applications. These targets could include GLSL or HLSL shader code, SPIRV assembly or binaries, CUDA code or PTX assembly, or perhaps even C++ code or dynamic libraries. Effectively, this means that the IRE is a multi-target Just-in-Time (JIT) compiler framework.

This is of course an ambitious goal, so one ought to raise a few precautionary questions:

1. In general, what would motivate someone to write host code to generate target code, rather than writing the target code from the start?
2. Why choose C++ as the base language to implement such as system?
3. What kind of features and restriction can a user expect from a system which delivers the capabilities above?

## 1. Shader permutations and runtime generation

In the particular context of game engines in computer science, shader permutations present a notable problem. Namely, suppose that a material in the game engine has a set of properties, and that it is desired to optimize away some of these options when compiling the final material shader. If these are, for example, $N$ boolean options in the shader, there will be a total of $2^N$ combinations of unique shaders which need to be compiled. This is impractical to do beforehand in most cases, so common solutions include:

1. Inject preprocessor directives (usually) and write code for both cases of each option, for every option.
2. Generate the final shader code during runtime by concatenating strings of code, embedding the right section of code for each option.
3. Write duplicate and specialized shaders for each and every permutation that is known to be used (usually a smaller subset of the $2^N$ permutations).

For the programmer, none of these options are sufficiently ergenomical, as they can quickly lead to technical debt as shaders become more complex.

Beside this reason, it may be be useful to generate programs at runtime, depending on the users input. As an example, take an application in neural graphics in which the user can import and select differing configurations of Neural Radiance Fields (e.g Vanilla, Instant NGP, Zip NeRF, etc. and at different sizes). It could be useful to generate the optimal rendering shader code for each configuration as it is selected, rather than having to restrict configurations to a fixed size specification.

Along this reasoning, another potential feature that runtime generation enables is the possibility of code transformation, to provide language features not native to the target code. For example, as differentiable programming is becoming more popular, some languages like [slang](https://research.nvidia.com/labs/rtr/publication/bangaru2023slangd/) have implemented differentiable programming as a native feature. However, this is not the case with other languages like CUDA. With a multi-target solution, such as the IRE proposed here, language features can be granted to those which do not natively have them.

# Atom IR