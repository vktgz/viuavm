# Viua VM

A simple, bytecode driven, register-based virtual machine.

I develop Viua VM to learn how such software is created and
to help myself in my computer language implementation studies.

----

## Programming in Viua

Viua can be programmed in an assembler-like language which must then be compiled into bytecode.
Assembler uses **Bytecode Generation API** to generate bytecode after the assembler frontend
performs initial analysis.

Typical code-n-debug cycle is shown below (assuming current working directory
is the local clone of Viua repository):

```
nvim ./some_file.asm
./bin/vm/asm -o some_file.out ./some_file.asm
./bin/vm/cpu some_file.out
./bin/vm/wdb some_file.out
```


----

# Development

----

## Tools and libraries

### C++11 Compiler

GNU Compiler Collection's C++ compiler version 4.9 and above, or
clang C++ compiler version 3.6.1 and above.
Clang is tested less frequently.


### OpenSSL

OpenSSL version supplying `<openssl/sha.h>` header, and providing `SHA256()` and `SHA512()` functions.


### Valgrind (optional)

Valgrind is used for debugging memory leaks.


### Python 3 (optional)

Test suite is written in Python 3.


----

## Compilation

Before compiling, Git submodule for `linenoise` library must be initialised.

Compilation is simple and can be executed by typing `make` in the shell.

Alternatively, `./scripts/recompile` can be executed to get a rebuild, or
`./scripts/compile` to build only changed parts.
Both scripts will detect number of cores on the machine used for compilation and
adjust make's `-j` option accordingly.


## Testing

There is a simple test suite located in `tests/` directory.
It can be invoked by `make test` command.
Python 3 must be installed on the machine to run the tests.

Code used for unit tests can be found in `sample/` directory.


### Viua development scripts

In the `scripts/` directory, you can find scripts that are used during development of Viua.
The shell installed in dev environment is ZSH but the scripts should be compatible with BASH as well.


## Git workflow

Each feature and fix is developed in a separate branch.
Bugs which are discovered during development of a certain feature,
may be fixed in the same branch as their parent issue.
This is also true for small features.

**Branch structure:**

- `master`: master branch - contains stable, working version of VM code,
- `devel`: development branch - all fixes and features are first merged here,
- `issue/<number>/<slug>`: for issues,


Explained with arrows:

```
issue/* ←----→ devel
                 |
                 ↓
              master
```


----

## License

The code is licensed under GNU GPL v3.
