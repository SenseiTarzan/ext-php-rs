# `#[php_const]` Attribute

Exports a Rust constant as a global PHP constant. The constant can be any type
that implements `IntoConst`.

The `wrap_constant!()` macro can be used to simplify the registration of constants.
It sets the name and doc comments for the constant.

You can rename the const with options:

- `name` - Allows you to rename the property, e.g.
  `#[php(name = "new_name")]`
- `change_case` - Allows you to rename the property using rename rules, e.g.
  `#[php(change_case = "PascalCase")]`

A global constant has no visibility. `#[php(vis = "...")]` on a `#[php_const]`
is a compile error. Use `vis` on constants inside a `#[php_impl]` block.

## Examples

```rust,no_run
# #![cfg_attr(windows, feature(abi_vectorcall))]
# extern crate ext_php_rs;
use ext_php_rs::prelude::*;

#[php_const]
const TEST_CONSTANT: i32 = 100;

#[php_const]
#[php(name = "I_AM_RENAMED")]
const TEST_CONSTANT_THE_SECOND: i32 = 42;

#[php_const]
const ANOTHER_STRING_CONST: &'static str = "Hello world!";

// Fixed-size arrays [T; N] are supported as compile-time constants:
#[php_const]
const ALLOWED_METHODS: [&'static str; 3] = ["GET", "POST", "HEAD"];

#[php_const]
const PRIMES: [i64; 4] = [2, 3, 5, 7];

#[php_module]
pub fn get_module(module: ModuleBuilder) -> ModuleBuilder {
    // Dynamic collections like Vec<T> cannot be Rust `const` items due to heap
    // allocation, but can be registered dynamically in the module builder:
    let dynamic_tags: Vec<String> = vec!["php".into(), "rust".into(), "ext-php-rs".into()];

    module
        .constant(wrap_constant!(TEST_CONSTANT))
        .constant(wrap_constant!(TEST_CONSTANT_THE_SECOND))
        .constant(wrap_constant!(ALLOWED_METHODS))
        .constant(wrap_constant!(PRIMES))
        .constant(("MANUAL_CONSTANT", ANOTHER_STRING_CONST, &[]))
        .constant(("DYNAMIC_TAGS", dynamic_tags, &[]))
}
# fn main() {}
```

## PHP usage

```php
<?php

var_dump(TEST_CONSTANT); // int(100)
var_dump(I_AM_RENAMED); // int(42)
var_dump(MANUAL_CONSTANT); // string(12) "Hello world!"
var_dump(ALLOWED_METHODS); // array(3) { [0]=> string(3) "GET", ... }
var_dump(PRIMES); // array(4) { [0]=> int(2), ... }
var_dump(DYNAMIC_TAGS); // array(3) { [0]=> string(3) "php", ... }
```

## Supported Constant Types

Types implementing `IntoConst` and `IntoZval` can be registered as constants:
- Scalar types: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `f32`, `f64`, `bool`.
- Strings: `&'static str`, `String`.
- Fixed-size arrays: `[T; N]` where `T: IntoConst + IntoZval`. In PHP, these are registered as persistent immutable arrays.
- Vectors: `Vec<T>` where `T: IntoConst + IntoZval`. Since `Vec` allocates on the heap, it cannot be a compile-time Rust `const`, but it can be passed at module startup via `module.constant(("NAME", vec, &[]))` or `module.dyn_constant(...)`.

> **Note on Immutability:**
> Passing a `Vec<T>` as a constant copies its values into a persistent, immutable Zend array (`IS_ARRAY_IMMUTABLE`). It does **not** create a live or mutable binding. Even if you retain a `Vec` or modify it internally in Rust after module initialization, the registered PHP constant will **never** change. Furthermore, if PHP user code attempts to modify the array, PHP's Copy-On-Write (COW) mechanism ensures the constant remains untouched.


