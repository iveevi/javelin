---
title: Thunder Specification
---

# Overview

Thunder is the Intermediate Representation (IR) form used in the Javelin framework. Each instruction is an atom, which is a tagged union of the following instruction types (think of these as specific op-codes).

| Atom                    | Description                                                        |
| ----------------------- | ------------------------------------------------------------------ |
| [Global](#global)       | Global variable or parameter of a subroutine                       |
| [TypeField](#typefield) | An index into a primitive type or the type of a struct field       |
| [Primitive](#primitive) | A primitive type in Javelin                                        |
| [Swizzle](#swizzle)     | Swizzle operation for vector types                                 |
| [Operation](#operation) | Builtin operation                                                  |
| [Construct](#construct) | Constructor for a primitive type or struct                         |
| [Call](#call)           | Call a subroutine with arguments                                   |
| [Intrinsic](#intrinsic) | Invoke a platform specific intrinsic                               |
| [List](#list)           | Generic list node, indexes to its value and the next item          |
| [Store](#store)         | Store values into an address by index                              |
| [Load](#load)           | Load the value from an index                                       |
| [Cond](#cond)           | Beginning branch of conditional branch statement                   |
| [Elif](#elif)           | Intermediate and terminal branch of a conditional branch statement |
| [While](#while)         | While loop indicator                                               |
| [Returns](#returns)     | Return instruction                                                 |
| [End](#end)             | Ends the last control flow block                                   |

## Global

| Field       | Type      | Description                                         |
| ----------- | --------- | --------------------------------------------------- |
| `type`      | `index_t` | Index to the type of the underlying global variable |
| `binding`   | `index_t` | Binding index of the global variable, if applicable |
| `qualifier` | `int8_t`  | Qualifier for the global variable, see below        |

The qualifier fied of the `Global` atom indicates the set of attributes available to the global variable. The currently supported set of qualifiers is as follows:

| Qualifier                           | Description                                  |
| ----------------------------------- | -------------------------------------------- |
| `parameter`                         | Parameter to a subroutine                    |
| `layout_in`                         | Layout input variable (GLSL)                 |
| `layout_out`                        | Layout output variable (GLSL)                |
| `push_constant`                     | Push constant variable (GLSL)                |
| `glsl_vertex_intrinsic_gl_Position` | Refers to the `gl_Position` intrinsic (GLSL) |

## TypeField

## Primitive

## Swizzle

## Operation

## Construct

## Call

## Intrinsic

## List

## Store

## Load

## Cond

## Elif

## While

## Returns

## End
