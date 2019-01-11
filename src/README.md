## Installation Instructions

A build script is included for your convenience.  Simply run `./build.sh`.  This will create a binary file `bin/code_generator`. Note that this program uses the `Boost` library. I have installed a copy of Boost on ix-dev in the location `/home/users/zayd/boost_1_68_0`.  Global read and execute permissions were given on this folder.

To run the program, simply call:

`bin/code_generator <filename>`

This code iterates through all steps of the compilation process namely:
1. Lexing
2. Parsing
3. AST Construction
4. Type Checking
5. Code generation

If an error is encountered, the compiler quits immediately.  Otherwise, the program generates an output `.c` file.  This file is in the same location as the specified `<filename>` passed to the script. The filename is also the same.  The only modification is that the file extension is changed to `.c`.

To compile the generated output, you call:

`gcc <filename.c> builtins.c`

Observe that `builtins.c` is a dependency of the generated code.  `builtins.c` and `builtins.h` are included in this directory.  Calling `gcc` as above should yield a compiled binary named `a.out` (or whatever name you specify with the `-o` option).  This file can simply be run via: `./a.out`.  

## Testbench

All test cases are in the repo folder `hw/demo` and for the programs that are valid, the expected output is in the folder `hw/demo/expected`.  

The full testbench suite was verified on my Mac (running Mojave) and on ix-dev.

## GitHub Repo

The GitHub repository cited in the config file **is a private repository.** I have sent you a collaborator invitation so you can download and use it. 
 
