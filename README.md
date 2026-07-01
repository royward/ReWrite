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
git clone https://github.com/[your-repo]/rewrite2.git
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

(examples included in here)

## Road map

