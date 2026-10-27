// src/tools/calculator_eval.cpp — two-stack evaluator (spec §4.6).

#include "tools/calculator.h"

#include <stack>

namespace {
using Kind = CalcToken::Kind;

bool is_operator(Kind kind) {
    return kind == Kind::Plus || kind == Kind::Minus ||
           kind == Kind::Times || kind == Kind::Divide;
}

int precedence(Kind kind) {
    if (kind == Kind::Times || kind == Kind::Divide) return 2;
    if (kind == Kind::Plus || kind == Kind::Minus) return 1;
    return 0;
}

bool apply_top(std::stack<double>& values, std::stack<Kind>& ops,
               std::string* error) {
    if (ops.empty() || !is_operator(ops.top()) || values.size() < 2) {
        *error = "invalid expression state";
        return false;
    }

    const Kind op = ops.top();
    ops.pop();

    const double right = values.top();
    values.pop();
    const double left = values.top();
    values.pop();

    double result = 0.0;
    switch (op) {
        case Kind::Plus:   result = left + right; break;
        case Kind::Minus:  result = left - right; break;
        case Kind::Times:  result = left * right; break;
        case Kind::Divide:
            if (right == 0.0) {
                *error = "division by zero";
                return false;
            }
            result = left / right;
            break;
        default:
            *error = "invalid operator";
            return false;
    }

    values.push(result);
    return true;
}
}

bool calc_evaluate(const std::vector<CalcToken>& tokens, double* out,
                   std::string* error) {
    std::stack<double> values;
    std::stack<Kind> ops;
    error->clear();

    for (const CalcToken& token : tokens) {
        switch (token.kind) {
            case Kind::Number:
                values.push(token.value);
                break;

            case Kind::LParen:
                ops.push(token.kind);
                break;

            case Kind::RParen:
                while (!ops.empty() && ops.top() != Kind::LParen) {
                    if (!apply_top(values, ops, error)) return false;
                }
                if (ops.empty()) {
                    *error = "mismatched parentheses";
                    return false;
                }
                ops.pop();
                break;

            case Kind::Plus:
            case Kind::Minus:
            case Kind::Times:
            case Kind::Divide:
                while (!ops.empty() && is_operator(ops.top()) &&
                       precedence(ops.top()) >= precedence(token.kind)) {
                    if (!apply_top(values, ops, error)) return false;
                }
                ops.push(token.kind);
                break;
        }
    }

    while (!ops.empty()) {
        if (ops.top() == Kind::LParen || ops.top() == Kind::RParen) {
            *error = "mismatched parentheses";
            return false;
        }
        if (!apply_top(values, ops, error)) return false;
    }

    if (values.size() != 1) {
        *error = "invalid expression state";
        return false;
    }

    *out = values.top();
    return true;
}
