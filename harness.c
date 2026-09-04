/*
 * harness.c
 * A minimal LLM agent harness written in standard C.
 *
 * Features:
 *   1. Core loop: reads user input from the terminal, passes it to a
 *      mock "model" function, and prints the simulated response.
 *   2. Context management: stores the last N turns of conversation
 *      history in a fixed-size, dynamically allocated buffer.
 *   3. Tool execution: if the user asks for a calculation (e.g.
 *      "calc 3 + 4"), the harness parses it and calls a calculator
 *      tool instead of the mock model.
 *
 * Only standard C libraries are used: <stdio.h>, <stdlib.h>, <string.h>.
 * Compile with:  gcc harness.c -o harness
 * Run with:      ./harness
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT_LEN   256   /* max characters per line of input        */
#define MAX_HISTORY     5     /* how many past turns we remember         */

/* ---------------------------------------------------------------------
 * Context management
 * -------------------------------------------------------------------*/

/* Each "turn" stores what the user said and what the harness replied. */
typedef struct {
    char *user_input;
    char *model_output;
} Turn;

/* Simple ring-buffer style history: when full, the oldest turn is
 * freed and overwritten by the newest one. */
typedef struct {
    Turn turns[MAX_HISTORY];
    int  count;      /* how many slots are currently filled  */
    int  next_slot;  /* index where the next turn will go     */
} History;

static void history_init(History *h) {
    h->count = 0;
    h->next_slot = 0;
    for (int i = 0; i < MAX_HISTORY; i++) {
        h->turns[i].user_input = NULL;
        h->turns[i].model_output = NULL;
    }
}

/* Adds a new turn to the history, freeing the oldest turn's memory
 * if the buffer is already full (this is our "safe memory management"). */
static void history_add(History *h, const char *user_input, const char *model_output) {
    int slot = h->next_slot;

    /* Free whatever used to live in this slot before overwriting it. */
    free(h->turns[slot].user_input);
    free(h->turns[slot].model_output);

    h->turns[slot].user_input = strdup(user_input);
    h->turns[slot].model_output = strdup(model_output);

    if (h->turns[slot].user_input == NULL || h->turns[slot].model_output == NULL) {
        fprintf(stderr, "Error: out of memory while storing history.\n");
        exit(1);
    }

    h->next_slot = (h->next_slot + 1) % MAX_HISTORY;
    if (h->count < MAX_HISTORY) {
        h->count++;
    }
}

/* Frees all memory owned by the history. Must be called before exit
 * to avoid memory leaks. */
static void history_free(History *h) {
    for (int i = 0; i < MAX_HISTORY; i++) {
        free(h->turns[i].user_input);
        free(h->turns[i].model_output);
        h->turns[i].user_input = NULL;
        h->turns[i].model_output = NULL;
    }
}

/* Prints the stored conversation history, oldest first. */
static void history_print(const History *h) {
    if (h->count == 0) {
        printf("(no history yet)\n");
        return;
    }
    printf("--- Conversation history (last %d turns) ---\n", h->count);
    int start = (h->next_slot - h->count + MAX_HISTORY) % MAX_HISTORY;
    for (int i = 0; i < h->count; i++) {
        int idx = (start + i) % MAX_HISTORY;
        printf("  [%d] You: %s\n", i + 1, h->turns[idx].user_input);
        printf("      Bot: %s\n", h->turns[idx].model_output);
    }
    printf("--------------------------------------------\n");
}

/* ---------------------------------------------------------------------
 * Tool execution: a very small calculator tool
 * -------------------------------------------------------------------*/

/* Tries to parse input of the form "calc <num> <op> <num>", e.g.
 * "calc 3 + 4". Returns 1 and fills *result if successful, 0 otherwise. */
static int tool_calculator(const char *input, double *result) {
    double a, b;
    char op;

    /* "calc" must be the first word. */
    if (strncmp(input, "calc ", 5) != 0) {
        return 0;
    }

    /* Parse: number, operator, number */
    if (sscanf(input + 5, "%lf %c %lf", &a, &op, &b) != 3) {
        return 0;
    }

    switch (op) {
        case '+': *result = a + b; return 1;
        case '-': *result = a - b; return 1;
        case '*': *result = a * b; return 1;
        case '/':
            if (b == 0) {
                return 0; /* avoid division by zero */
            }
            *result = a / b;
            return 1;
        default:
            return 0;
    }
}

/* ---------------------------------------------------------------------
 * Mock model: stands in for a real LLM call
 * -------------------------------------------------------------------*/

/* Writes a simulated model response into out_buf (size out_buf_size). */
static void mock_model(const char *input, char *out_buf, size_t out_buf_size) {
    if (strstr(input, "hello") != NULL) {
        snprintf(out_buf, out_buf_size, "Hello there! How can I help you today?");
    } else if (strncmp(input, "history", 7) == 0) {
        snprintf(out_buf, out_buf_size, "(showing history below)");
    } else {
        /* Default behavior: echo the input back. */
        snprintf(out_buf, out_buf_size, "You said: %s", input);
    }
}

/* Strips the trailing newline that fgets() leaves on the input string. */
static void strip_newline(char *s) {
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') {
        s[len - 1] = '\0';
    }
}

/* ---------------------------------------------------------------------
 * Main program loop
 * -------------------------------------------------------------------*/

int main(void) {
    char input[MAX_INPUT_LEN];
    char output[MAX_INPUT_LEN + 32];
    History history;

    history_init(&history);

    printf("=== Mini LLM Harness ===\n");
    printf("Type a message, 'calc <num> <op> <num>' to use the calculator tool,\n");
    printf("'history' to view recent turns, or 'exit' to quit.\n\n");

    while (1) {
        printf("You: ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            /* EOF (e.g. Ctrl+D) — shut down safely. */
            printf("\nEOF detected, shutting down.\n");
            break;
        }

        strip_newline(input);

        /* 1. Check for exit condition first. */
        if (strcmp(input, "exit") == 0) {
            printf("Bot: Goodbye!\n");
            break;
        }

        /* 2. Skip empty input. */
        if (strlen(input) == 0) {
            continue;
        }

        /* 3. Check for the "history" command. */
        if (strcmp(input, "history") == 0) {
            history_print(&history);
            continue;
        }

        /* 4. Try the calculator tool before falling back to the mock model. */
        double calc_result;
        if (tool_calculator(input, &calc_result)) {
            snprintf(output, sizeof(output), "[tool:calculator] Result = %g", calc_result);
        } else {
            /* 5. Fall back to the mock model for a normal response. */
            mock_model(input, output, sizeof(output));
        }

        printf("Bot: %s\n", output);

        /* 6. Record this turn in the context history. */
        history_add(&history, input, output);
    }

    /* Always free everything before exiting, to avoid memory leaks. */
    history_free(&history);

    return 0;
}
