# clusterfuck

Clusterfuck is an optimizing x64 JIT compiler for brainfuck, an esoteric programming language. 

More about the language here: https://esolangs.org/wiki/Brainfuck

## Usage

`./clusterfuck program.bf`

## Optimization

1. Instruction collapsing

The instructions `+`, `-`, `<` and `>` can be sort of run-length encoded.

Two consecutive `+` would mean executing `add [tape], 1` twice. Instead we generate a single `add [tape], 2` instruction. If there are 15 consecutive `>` for example, this saves a lot of instructions.

When run with the Mandelbrot set renderer, this results in a ~3x speedup. This JIT compiled is also about 13% faster than the same program transcompiled to `C` and compiled with `GCC -O3`.

## Benchmarks

Comming soon.

## Programs to test it out!

* [Tic Tac Toe with AI](https://github.com/mitxela/bf-tic-tac-toe/blob/master/tictactoe.bf)
* [Mandelbrot set fractal viewer](https://github.com/frerich/brainfuck/blob/master/samples/mandelbrot.bf)

