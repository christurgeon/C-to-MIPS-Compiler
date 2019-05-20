# C-to-MIPS-Compiler
This program reads in arithmetic C code and compiles it into MIPS Assembly instructions. Comments are also included which show the original line being translated. 

## Input
Code reads the input file specified as first command-line argument (argv[1]).

## Assumptions
* Assumes that all input files are valid (see example text files) and of length at most 128 characters
* Each C variable name is one lowercase letter (e.g. a, b, c, etc.) and of type int
* Integer constants may be positive, negative, or zero
* Supports addition (+), subtraction (-), multiplication (*), division (/) and mod (%) operators
* Treats all of these as 32-bit integer operations with each operation requiring two operands
* Assumes that each assignment statement consists of a mix of addition and subtraction operations, a mix of multiplication or division operations, or
a simple assignment  
* Assumes that constants will only appear as the second operand to a valid operator (e.g., x = 42 / y
is not possible)
* Assume that expressions will never have two constants adjacent to one another (e.g., x = 42 / 13
is not possible)

## Source
RPI CSCI 2500 - Computer Organization Homework 4
