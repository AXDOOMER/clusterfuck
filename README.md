# clusterfuck

[![CircleCI](https://circleci.com/gh/AXDOOMER/clusterfuck.svg?style=svg)](https://circleci.com/gh/AXDOOMER/clusterfuck)

Clusterfuck is an optimizing x64 JIT compiler for brainfuck, an esoteric programming language. 

More about the language here: https://esolangs.org/wiki/Brainfuck

## Usage

`./clusterfuck program.bf`

I may add a way to dump the compiled code to an ELF soon.

# Specs

* Tape size: 30000 cells
* Cell size: 8 bits (1 byte)
* Invalid brainfuck characters are ignored

## Optimization

The instructions `+`, `-`, `<` and `>` can be sort of run-length encoded. Let's call it instruction collapsing.

Two consecutive `+` would mean executing the assembly `add [tape], 1` twice. Instead the compiler generates a single `add [tape], 2` instruction. If there are 15 consecutive `>` for example, this saves a lot of instructions and makes the code much smaller.

When run with the Mandelbrot set renderer, this optimization results in a ~3x speedup. This JIT compiler is also about 13% faster than the same program transcompiled to `C` and compiled with `GCC -O3`.

## Benchmarks

Coming soon.

Use: `perf stat -r 10 -d`

## Programs to test it out!

* [Tic Tac Toe with AI](https://github.com/mitxela/bf-tic-tac-toe/blob/master/tictactoe.bf)
* [Mandelbrot set fractal viewer](https://github.com/frerich/brainfuck/blob/master/samples/mandelbrot.bf)
* [Towers of Hanoi](http://www.clifford.at/bfcpu/hanoi.bf)

## Screenshots

### Hello world

![Screenshot_2020-07-11_22-09-03](https://user-images.githubusercontent.com/6194072/87237366-76f49180-c3c3-11ea-89f7-126ac788e790.png)

\*This is a development screenshot. All the debugging information has been removed.

### Mandelbrot set fractal

![Screenshot_2020-07-11_21-55-31](https://user-images.githubusercontent.com/6194072/87237367-76f49180-c3c3-11ea-957c-b7f925b502ed.png)

## Bugs

* Not handling out of bounds on the tape or malformed programs (e.g. not an equal number of `[` and `]`)
* It has static limits for the code size and stack size
* Not portable -- Only works on Linux with a x64 CPU for now
