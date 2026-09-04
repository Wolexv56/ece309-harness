# ECE 309 Project 1 — LLM Mini-Harness in C

A minimal LLM agent harness written in standard C, built via vibe coding /
Specification Driven Development (SDD).

## What It Does

- **Core loop:** Reads user input from the terminal and prints a simulated
  ("mock") model response.
- **Context management:** Stores the last 5 conversation turns in a
  dynamically allocated ring buffer, with all memory safely freed on exit.
- **Tool execution:** Recognizes `calc <num> <op> <num>` and performs the
  arithmetic directly via a calculator tool, instead of routing it through
  the mock model — demonstrating how an agent harness delegates work an
  LLM isn't suited for.
- **History command:** Type `history` to view the stored conversation
  turns.

## Files

| File                  | Purpose                                              |
|------------------------|-------------------------------------------------------|
| `harness.c`            | Main C program (core loop, history, tool execution)  |
| `test.sh`              | Automated Bash test script                            |
| `vibe_coding_log.md`   | SDD process log — prompts used and AI responses        |
| `README.md`            | This file                                              |

## Build

```bash
gcc harness.c -o harness
```

Only standard C libraries are used (`<stdio.h>`, `<stdlib.h>`,
`<string.h>`), so this should compile cleanly in any POSIX environment.

## Run

```bash
./harness
```

Example session:
```
You: hello
Bot: Hello there! How can I help you today?
You: calc 3 + 4
Bot: [tool:calculator] Result = 7
You: history
--- Conversation history (last 2 turns) ---
  [1] You: hello
      Bot: Hello there! How can I help you today?
  [2] You: calc 3 + 4
      Bot: [tool:calculator] Result = 7
--------------------------------------------
You: exit
Bot: Goodbye!
```

## Test

```bash
bash test.sh
```

This pipes a predefined sequence of inputs into the compiled program,
checks the outputs match expected values, and attempts a memory leak
check (`valgrind` on Linux, or a manual `leaks` command suggested on
macOS).

## Author

*(Your name / ECE 309, Fall 2026)*
