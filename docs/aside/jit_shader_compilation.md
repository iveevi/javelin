# Aside: Why bother with JIT shader compilation?

In the particular context of game engines in computer science, shader permutations present a notable problem. Namely, suppose that a material in the game engine has a set of properties, and that it is desired to optimize away some of these options when compiling the final material shader. If these are, for example, $N$ boolean options in the shader, there will be a total of $2^N$ combinations of unique shaders which need to be compiled. This is impractical to do beforehand in most cases, so common solutions include:

1. Inject preprocessor directives (usually) and write code for both cases of each option, for every option.
2. Generate the final shader code during runtime by concatenating strings of code, embedding the right section of code for each option.
3. Write duplicate and specialized shaders for each and every permutation that is known to be used (usually a smaller subset of the $2^N$ permutations).

For the programmer, none of these options are sufficiently ergenomical, as they can quickly lead to technical debt as shaders become more complex.

Beside this reason, it may be be useful to generate programs at runtime, depending on the users input. As an example, take an application in neural graphics in which the user can import and select differing configurations of Neural Radiance Fields (e.g Vanilla, Instant NGP, Zip NeRF, etc. and at different sizes). It could be useful to generate the optimal rendering shader code for each configuration as it is selected, rather than having to restrict configurations to a fixed size specification.

Along this reasoning, another potential feature that runtime generation enables is the possibility of code transformation, to provide language features not native to the target code. For example, as differentiable programming is becoming more popular, some languages like [slang](https://research.nvidia.com/labs/rtr/publication/bangaru2023slangd/) have implemented differentiable programming as a native feature. However, this is not the case with other languages like CUDA. With a multi-target solution, such as the IRE proposed here, language features can be granted to those which do not natively have them.