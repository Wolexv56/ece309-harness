# Vibe Coding Log — ECE 309 Project 1

This log documents the Specification Driven Development (SDD) process used to
generate `harness.c` and `test.sh` with AI assistance.

## 1. Architectural Rules (Spec) Given to the AI

Before generating any code, the following constraints were set:

- **Language:** Standard C only (POSIX-compliant), no external libraries
  beyond `<stdio.h>`, `<stdlib.h>`, `<string.h>`.
- **Core loop:** Infinite loop reading input via `fgets`, exits cleanly on
  the input `exit`.
- **Context management:** Store the last 5 conversation turns in a
  fixed-size, dynamically allocated ring buffer. All allocated memory must
  be freed before the program exits (no leaks).
- **Tool execution:** Support a calculator tool, triggered by input of the
  form `calc <num> <op> <num>`, that performs the arithmetic instead of
  routing it through the mock model.
- **Mock model:** A function that simulates an LLM response — greets the
  user on "hello", otherwise echoes their input back.

## 2. Prompt Used to Generate `harness.c`

> "I need to write a command-line LLM agent harness in C. Use only standard
> libraries (`<stdio.h>`, `<stdlib.h>`, `<string.h>`) — no external
> dependencies. Requirements:
> 1. An infinite loop that reads user input with `fgets` and prints a
>    simulated model response.
> 2. Typing `exit` breaks the loop and shuts down safely.
> 3. Store the last 5 conversation turns (user input + response) in a
>    dynamically allocated history buffer. Free all memory before exit —
>    no leaks.
> 4. Support a calculator tool: if the input matches `calc <num> <op>
>    <num>`, compute the result directly instead of using the mock model.
> 5. If the user types 'history', print the stored conversation turns.
> 6. Otherwise, fall back to a mock model that greets on 'hello' or echoes
>    the input.
> 7. Add clear, line-by-line comments explaining what the code does."

**AI response:** Generated a single-file `harness.c` implementing a
`History` struct with a ring buffer (`turns[MAX_HISTORY]`), a
`tool_calculator()` function using `sscanf` to parse arithmetic
expressions, a `mock_model()` function, and a `main()` loop wiring
everything together. Code compiled successfully on the first attempt with
`gcc -Wall -Wextra harness.c -o harness` — no errors or warnings.

## 3. Prompt Used to Generate `test.sh`

> "I have a compiled C program named `harness`. Write a simple Bash script
> that pipes the inputs `hello`, `calc 3 + 4`, `history`, and `exit` into
> the program, checks the output for the expected responses, and also
> checks for memory leaks using `valgrind` if available (or `leaks` on
> macOS)."

**AI response:** Generated `test.sh`, which pipes a predefined sequence of
inputs into `./harness`, greps the output for expected substrings
("Hello there", "Result = 7", "Goodbye"), and attempts a leak check with
`valgrind` (falls back to a note about macOS's `leaks` tool when
`valgrind` isn't installed).

## 4. Iteration / Debugging Notes

- Compiled with `gcc -Wall -Wextra harness.c -o harness` — **no errors or
  warnings** on the first pass.
- Ran `./harness` manually with inputs `hello`, `calc 10 * 5`, `history`,
  and `exit` — all behaved as expected.
- Ran `bash test.sh` — all functional checks passed.
- Manually tested the division-by-zero guard in `tool_calculator()` by
  temporarily changing the check from `if (b == 0)` to `if (b == 1)` and
  recompiling. Running `calc 5 / 0` with this change confirmed the
  program no longer rejected the division and produced an incorrect
  result instead of safely falling back. Reverting the condition to
  `if (b == 0)`, recompiling, and re-running `calc 5 / 0` confirmed the
  tool now correctly rejects the operation and the harness falls back
  to the mock model instead of crashing or returning garbage output.
  This confirmed the zero-check guard is working as intended.

## 5. Reflection

This project demonstrated how a precise, constraint-driven specification
(SDD) produces working C code from an AI on the first or near-first
attempt, versus a vague request like "write a chat app in C." Being
explicit about memory management (fixed-size buffer, explicit `free()`
calls) was the key requirement that prevented memory leaks.
