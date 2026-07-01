# ReWrite2

A rule-based language for expressing recursive computation through pattern matching. Initial version an interpreter in Modern C++, but the plan is to bootstrap it - write a ReWrite2 compiler in itself that generates instructions suitable for a Virtual Machine, and maybe eventually x86-64 and ARM machine code.

Note: ReWrite2 is a working name while I look for a better one.

## Overview

A ReWrite2 program is a list of functions, where each function is a set of rules, and the execution path is that a function call will try each of the rules, then fire the first one that matches (without Prolog style backtracking). For a simple example:

```
fact(0) -> 1;
fact(n) -> n*fact(n-1);
```

Each call will check first if the argument is 0, in which case it will terminate returning 1, or it will calculate `fact(n)` recursively in terms of `fact(n-1)`.

ReWrite2:
* is currently interpreted, with the goal of later being compiled.
* will be strongly typed - it is currently dynamically typed, but this will change in the compiled version.
* is mostly functional (does allow side effect for things like logging).
* uses patterns instead of if/then/else and case.
* uses tail recursion instead of loops.
* uses no garbage collector - everything is copied, modified in place or disposed of, so that there is exactly one reference to anything. This will be replaced by reference counting in the compiled version. Both of these techniques avoid garbage collection pauses and make memory management predictable.
* is intended to have the Rust/Haskell property that where possible things fail at parse or compile time rather than run time.

## License

ReWrite2 is licensed under the [Apache License 2.0](LICENSE).

Example programs (in the `tests/` directory) are released under [CC0 1.0 Universal (Public Domain)](https://creativecommons.org/publicdomain/zero/1.0/) — you are free to use them without restriction.

## History and Motivation

In the 1990s, I got really interested in using pattern matching for programming, partly inspired by Prolog (I know that is unification, not pattern matching), and partly inspired by Mathematica. This seemed particularly suited to logic where there are many cases with different specificity to deal with. I wrote a language called ReWrite that worked on Classic MacOS, initially writing a minimal interpreter in Pascal, then bootstrapped it to write a ReWrite -> 68000 compiler in ReWrite itself, then discarded the interpreter. This worked well - the source for the ReWrite -> 68000 compiler was about 100K of text (excluding some of the library stuff). I wanted to develop this into a generally useful language, but writing full library support was an enormous amount of work, so the project died on the vine. I briefly explored porting it to PowerPC and having a way to integrate it with C++, but this would have been totally dependent on CodeWarrior (the MacOS PowerPC development environment at the time). I also looked at compiling to Java Virtual Machine, but that lacks some of the features like tail recursion needed for a language like this. [Mercury](https://www.mercurylang.org/) (a strongly typed logic language) has been the language closest to filling this niche, but its built-in backtracking makes it less suited to rule-based dispatch where exactly one rule fires per call.

This idea lay dormant until recently, when as part of my work on the [Galaxy Generator Project](https://orange-kiwi.com/galaxy-generator/) (now being developed in Rust), I realised that some of the logic in there is going to get very complex, and there would be value in having a way of having some of the logic in a ReWrite style language in a module embedded into Rust. This avoids the whole library issue of trying to write a general purpose language. A lot has changed in language design in the last 30 years, so this project is not the original ReWrite - I'm using a more modern syntax, have somewhat more powerful semantics.

Most importantly, the original ReWrite was dynamically typed (types checked at runtime), and this new project will be strongly statically typed (types checked at compile time), as that avoids runtime errors, and allows more optimal code generation.

As a secondary motivation, I was interested to see how well Modern C++ could be used to rapidly develop the initial interpreter. The interpreter was written in under four days of actual development time, suggesting the approach was successful.

## Use cases

ReWrite2 can be used for general programming, but is particularly good at dealing with complex-case logic. For instance, this makes it well suited to lexers, parsers and compilers. I also think it is going to have value in complex game logic (I haven't tested this yet), and converting a data structure into readable text (for example, planet descriptions).

The example programs demonstrate recursive algorithms including prime generation and the n-queens problem, giving a sense of the language's expressive range beyond compiler construction.

It is designed to be embedded in other languages (currently C++, later will also support C and Rust), so the plan is to use it for the logic that goes alongside heavy numerical computation in the embedding language.


## Installation

ReWrite2 requires a C++23 compatible compiler (GCC 14+ or Clang 18+) and CMake 3.20+. It uses no platform-specific code and should work on any platform supporting these tools, but has only been tested on Linux.

```bash
git clone https://github.com/royward/rewrite2.git
cd rewrite2
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

The binary will be at `build/release/rewrite_cpp`.

For a debug build:

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug
```

I welcome bug reports if there is some issue where it won't build on another platform with a recent enough C++ compiler.

Note that once ReWrite2 builds itself, compiling the VM will not require C++-23 - probably C++17 at most.

## Usage

ReWrite2 currently has a command line utility:

```
<path>/rewrite_cpp <program_file> <expression>
```

where `<expression>` is any valid ReWrite2 expression, which may call functions defined in `<program_file>`.

So for instance (from the `build/release` directory):

```
./rewrite_cpp ../../tests/factorial.rw "fact(10)"
Results:
3628800
```

It is also possible to just use the classes directly (just look at `main.cpp` for the code for this). A note of caution with doing this - the current version is the throwaway prototype, and the API to phase 2 will be quite different.

## Language description

This section has examples which can all be found in the `tests` directory.

#### General Structure

The structure of a ReWrite2 program is simple - it is a list of functions, where each function is a list of rules of the form:

```
<function_name>(<match parameters>)[::<optional guard expression>] -> <expressions>;
```

and the language just fires the first rule that successfully matches (it is a runtime error if none of them fire - later versions might make this a compile time error).

So for instance, in the example from earlier:

```
fact(0)->1;
fact(n)->n*fact(n-1);
```

If `fact(3)` is evaluated, it fails to match with the first rule, matches with the second which fires, so `n*fact(n-1)` is evaluated, which calls `fact(2)`.

When `fact(0)` is called, it will fire the first rule, but having succeeded, it will _not_ fire the second one - unlike Prolog or Mercury, there is no backtracking.

#### Guards

Guards are useful when the condition can't be expressed purely as a pattern match. They are expected to be an expression returning a single boolean.

An alternative version of `fact` using a guard is:

```
fact(n)::n>0 -> n*fact(n-1);
fact(_) -> 1;
```

The first rule will fire if `n` is matched (which will match to any single argument), but the rule will fire only if the guard condition is met: `n>0`, giving the same result as the first version (except it won't crash the stack like the first version would).

The `_` is a special symbol that means "match with anything". Unlike a named parameter, it doesn't bind to anything, so you can use multiple `_` in the same rule without conflict.

#### Multiple return values

One unusual feature of ReWrite2 is that functions can return multiple values, and these will be taken as separate values when inserted into a list or another function call.

For instance, in the following, `min_max` returns two values, the min and the max:

```
min_max(a,b) :: a<b -> a,b;
min_max(a,b) -> b,a;
```

and this will be taken as two parameters by another function, for instance:

```
sub(a,b) -> b-a;
```
so `sub(min_max(10,3))` and `sub(min_max(3,10))` will both return `7`.

It's worth unpacking what is going on here: `min_max(10,3)` returns the results `3` (the min) and `10` (the max). The `3` and `10` are passed as two arguments to `sub(a,b)`, so `sub(3,10)` is evaluated.

One motivation for this (using features not yet in the language) was `polar_to_rectangular`:

```
// hypothetical - floating point not yet supported
polar_to_rectangular(r, theta) -> r*cos(theta), r*sin(theta);
```

It naturally returns multiple values, and while many other languages allow the result to be returned as a pair (or more generally, a tuple), it then needs to be unpacked, usually by assigning it to something and getting out indexed values. ReWrite2 makes this much more natural - if the parameter order matches up (if it doesn't, ReWrite2 would easily support a one line shim to fix this).

#### Lists and splat

### More complex examples

#### First n prime numbers

#### N-Queens

## Road map

ReWrite2 development will occur in phases:

#### Phase 1: minimal interpreter

This is mostly done - the only things left to complete this are:

* char/string support
* constants
* additional library functions required by the phase 2 compiler

#### Phase 2: compiling to VM

This stage does not involve adding language features, but writing a ReWrite2 program that compiles itself to a VM targetted binary. Steps are:

* write ReWrite2 -> VM compiler, written in ReWrite2 itself (self-hosting)
* write the VM (probably in C++ or C - this will be fairly low level)

#### Phase 3+: strong typing and other features

Once ReWrite2 can build and run itself, the interpreter is no longer necessary, so other features can be added:

* Interface from C, C++ and Rust
* Strong static typing (types checked at compile time)
* Structs
* Better code optimization
* More language features and libraries as useful.
  
