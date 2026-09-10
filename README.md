# S8

S8 is a small interpreted programming language designed around a simple and strict syntax.
The current design focuses on providing a small set of core language features while keeping the syntax explicit and predictable.

## Features

### Variables

S8 provides the `int` type for declaring integer variables.

```s8
int a = 10;
int b;
```

Variables can contain integer and hexadecimal values.

```s8
int a = 10;
int b = 0x10;
```

### Expressions

S8 supports arithmetic and bitwise operations.

**Arithmetic:**

* `+`
* `-`
* `*`
* `/`

**Bitwise:**

* `&`
* `|`
* `^`
* `~`
* `<<`
* `>>`

### Functions

Functions are declared using the `func` keyword.

```s8
func add(int a, int b) {
    ...
}
```

Functions can have parameters and a body.

### Return

The `return` keyword is used to return a value from a function.

```s8
return a;
```

Expressions can also be returned.

```s8
return a + b;
```

### Struct

S8 provides `struct` for defining structured data.

```s8
struct Point {
    int x;
    int y;
}
```

Struct members can be accessed using `.`.

```s8
Point.x;
Point.y;
```

## Syntax

The current core syntax is intentionally strict:

```text
int <identifier> = <expression>;
int <identifier>;

func <name>() {
    ...
}

func <name>(int <parameter>, int <parameter>) {
    ...
}

return <identifier>;
return <expression>;
```

S8 does not currently try to make every line an arbitrary expression. Statements follow specific grammar rules defined by the language.

## Current Development

The following features are still being developed:

* Variable re-assignment
* Using a function call as a value for `int`
* Returning a function call
* Memory access

These features are planned as part of the next development stage.

## Design Goal

S8 is intentionally kept small.

The goal is not to reproduce the syntax or flexibility of existing general-purpose languages, but to experiment with a compact language design, expressions, structured data, functions, and eventually low-level memory access.
