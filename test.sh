#!/bin/bash
# test.sh
# Automated test for the harness program.
# Pipes a sequence of inputs into ./harness and checks the output.
# Also runs a leak check with valgrind if it's available (Linux only;
# on Mac, use `leaks` or Xcode Instruments instead — see note below).

set -e

BINARY=./harness

if [ ! -f "$BINARY" ]; then
    echo "Error: $BINARY not found. Compile it first with:"
    echo "  gcc harness.c -o harness"
    exit 1
fi

echo "=== Running functional test ==="

# Feed a sequence of inputs: a greeting, a calculator call,
# a history check, and finally exit.
INPUT="hello
calc 3 + 4
history
exit"

OUTPUT=$(printf "%s\n" "$INPUT" | "$BINARY")

echo "$OUTPUT"
echo ""

# --- Basic assertions -------------------------------------------------
PASS=1

if ! echo "$OUTPUT" | grep -q "Hello there"; then
    echo "FAIL: greeting response not found"
    PASS=0
fi

if ! echo "$OUTPUT" | grep -q "Result = 7"; then
    echo "FAIL: calculator result not found"
    PASS=0
fi

if ! echo "$OUTPUT" | grep -q "Goodbye"; then
    echo "FAIL: exit message not found"
    PASS=0
fi

if [ "$PASS" -eq 1 ]; then
    echo "=== All functional checks PASSED ==="
else
    echo "=== Some functional checks FAILED ==="
    exit 1
fi

# --- Memory leak check --------------------------------------------------
echo ""
echo "=== Checking for memory leaks ==="

if command -v valgrind >/dev/null 2>&1; then
    printf "%s\n" "$INPUT" | valgrind --leak-check=full --error-exitcode=1 "$BINARY" > /dev/null
    echo "valgrind: no leaks detected"
elif command -v leaks >/dev/null 2>&1; then
    echo "Note: on macOS, run this manually for a leak report:"
    echo "  leaks --atExit -- ./harness < <(printf '%s\n' \"$INPUT\")"
else
    echo "No leak-checking tool (valgrind/leaks) found — skipping this step."
fi
