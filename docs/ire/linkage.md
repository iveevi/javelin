---
title: Linkage Model for Atom IR Kernels
---

# Overview

With the addition of [callabes](callable.md) in the IRE system, one can expect the atoms of generated kernels to contain `Call` instructions to registered callables. These instructions take the form of:

`call $<name>: %<args> -> %<return type>`

and are effectively a reference to a callabe that is cached globally. During synthesis, however, these callables must be resolved in order for them to be synthesized appropriately. Unfortunately it is not enough to delegate the synthesis of the `call` instructions to their callable's synthesizer itself, since it could result in duplicate types and global parameters. Therefore, we have a formal linking procedure before doing a single pass on the synthesis.

# Separate Compilation Model (Prototype)

The current linkage model becomes inefficient when there is significant code reuse, as it re-synthesizes all blocks of intermediate instructions. Luckily, the solution is not too complicated in our case, since each callable has its unique `cid`. We can cache the sythesized output on a per-callable basis, and then load the appropriate version during sythesis. To handle overloaded callables (which have the same name) we may add a suffix of `_Z<cid>`.