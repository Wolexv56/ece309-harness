// tests/p3/test_p3.cpp — Project 3 tests.

#include "../mini_test.h"
#include "../test_helpers.h"

#include "tools/calculator.h"
#include "tools/tool_registry.h"
#include "tools/workspace.h"

#include <cstdio>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

class NamedTool : public Tool {
public:
    explicit NamedTool(std::string n) : n_(std::move(n)), schema_(JsonValue::object()) {}
    std::string name() const override { return n_; }
    std::string description() const override { return "test tool"; }
    const JsonValue& schema() const override { return schema_; }
    ToolResult invoke(const JsonValue&, ToolContext&) override { return ToolResult::success(n_); }
private:
    std::string n_;
    JsonValue schema_;
};

class CountingTool : public Tool {
public:
    CountingTool(std::string n, int* destroyed)
        : n_(std::move(n)), destroyed_(destroyed), schema_(JsonValue::object()) {}
    ~CountingTool() override { ++(*destroyed_); }
    std::string name() const override { return n_; }
    std::string description() const override { return "counting tool"; }
    const JsonValue& schema() const override { return schema_; }
    ToolResult invoke(const JsonValue&, ToolContext&) override { return ToolResult::success(n_); }
private:
    std::string n_;
    int* destroyed_;
    JsonValue schema_;
};

bool eval(const std::string& expr, double* value, std::string* error) {
    std::vector<CalcToken> tokens;
    if (!calc_tokenize(expr, &tokens, error)) return false;
    return calc_evaluate(tokens, value, error);
}

bool resolve_ok(Workspace& ws, const std::string& input, const fs::path& expected) {
    fs::path out;
    std::string reason;
    if (!ws.resolve(input, &out, &reason)) return false;
    return out == expected;
}

bool resolve_bad(Workspace& ws, std::string_view input) {
    fs::path out;
    std::string reason;
    return !ws.resolve(input, &out, &reason) && !reason.empty();
}

}  // namespace

// ---------------------------------------------------------------- registry

TEST(reg_add_and_find) {
    ToolRegistry r;
    CHECK(r.size() == 0);
    CHECK(r.bucket_count() >= 1 && r.bucket_count() <= 64);

    auto t = std::make_unique<NamedTool>("alpha");
    Tool* raw = t.get();
    REQUIRE(r.add(std::move(t)));
    CHECK(r.size() == 1);
    CHECK(r.find("alpha") == raw);
    CHECK(r.find("beta") == nullptr);
}

TEST(reg_duplicate_keeps_original) {
    ToolRegistry r;
    auto first = std::make_unique<NamedTool>("alpha");
    Tool* original = first.get();
    REQUIRE(r.add(std::move(first)));
    CHECK(!r.add(std::make_unique<NamedTool>("alpha")));
    CHECK(r.find("alpha") == original);
    CHECK(r.size() == 1);
}

TEST(reg_null_rejected) {
    ToolRegistry r;
    std::unique_ptr<Tool> null_tool;
    CHECK(!r.add(std::move(null_tool)));
    CHECK(r.size() == 0);
}

TEST(reg_duplicate_new_tool_destroyed) {
    int destroyed = 0;
    {
        ToolRegistry r;
        REQUIRE(r.add(std::make_unique<CountingTool>("same", &destroyed)));
        CHECK(destroyed == 0);
        CHECK(!r.add(std::make_unique<CountingTool>("same", &destroyed)));
        CHECK(destroyed == 1);
    }
    CHECK(destroyed == 2);
}

TEST(reg_for_each_visits_each_once) {
    ToolRegistry r;
    REQUIRE(r.add(std::make_unique<NamedTool>("alpha")));
    REQUIRE(r.add(std::make_unique<NamedTool>("beta")));
    REQUIRE(r.add(std::make_unique<NamedTool>("gamma")));

    std::set<std::string> names;
    r.for_each([&](const Tool& tool) { names.insert(tool.name()); });
    CHECK(names.size() == 3);
    CHECK(names.count("alpha") == 1);
    CHECK(names.count("beta") == 1);
    CHECK(names.count("gamma") == 1);
}

TEST(reg_hash_is_order_sensitive) {
    CHECK(ToolRegistry::hash("ab") != ToolRegistry::hash("ba"));
    CHECK(ToolRegistry::hash("tool_1") != ToolRegistry::hash("tool_2"));
}

TEST(reg_10000_tools) {
    ToolRegistry r;
    auto first = std::make_unique<NamedTool>("tool_0");
    Tool* first_ptr = first.get();
    REQUIRE(r.add(std::move(first)));

    for (int i = 1; i < 10000; ++i) {
        REQUIRE(r.add(std::make_unique<NamedTool>("tool_" + std::to_string(i))));
        CHECK(r.load_factor() <= 1.0);
    }

    CHECK(r.size() == 10000);
    CHECK(r.find("tool_0") == first_ptr);
    for (int i = 0; i < 10000; ++i) {
        CHECK(r.find("tool_" + std::to_string(i)) != nullptr);
    }
    CHECK(r.max_probe_length() <= 64);

    std::printf("registry scale: buckets=%zu load=%.6f max_probe=%zu\n",
                r.bucket_count(), r.load_factor(), r.max_probe_length());
}

TEST(reg_alternative_hash) {
    ToolRegistry r;
    for (int i = 0; i < 10000; ++i) {
        REQUIRE(r.add(std::make_unique<NamedTool>("tool_" + std::to_string(i))));
    }

    std::vector<int> chains(r.bucket_count(), 0);
    for (int i = 0; i < 10000; ++i) {
        const std::string name = "tool_" + std::to_string(i);
        const std::size_t weak = static_cast<unsigned char>(name[0]) + name.size();
        ++chains[weak % chains.size()];
    }

    int longest = 0;
    for (int n : chains) {
        if (n > longest) longest = n;
    }
    CHECK(longest > static_cast<int>(r.max_probe_length()));
    std::printf("hash comparison: FNV max_chain=%zu, weak max_chain=%d (buckets=%zu)\n",
                r.max_probe_length(), longest, r.bucket_count());
}

// ---------------------------------------------------------------- workspace

TEST(ws_accepts_normal_and_normalized_paths) {
    TempDir d;
    Workspace ws(d.path());
    CHECK(resolve_ok(ws, "notes/todo.txt", ws.root() / "notes" / "todo.txt"));
    CHECK(resolve_ok(ws, "./notes//todo.txt", ws.root() / "notes" / "todo.txt"));
    CHECK(resolve_ok(ws, "a/../b", ws.root() / "b"));
    CHECK(resolve_ok(ws, "new/dir/file.txt", ws.root() / "new" / "dir" / "file.txt"));
    CHECK(resolve_ok(ws, "my..notes.txt", ws.root() / "my..notes.txt"));
}

TEST(ws_accepts_root_forms) {
    TempDir d;
    Workspace ws(d.path());
    CHECK(resolve_ok(ws, ".", ws.root()));
    CHECK(resolve_ok(ws, "a/..", ws.root()));
}

TEST(ws_rejects_empty) {
    TempDir d;
    Workspace ws(d.path());
    CHECK(resolve_bad(ws, ""));
}

TEST(ws_rejects_nul) {
    TempDir d;
    Workspace ws(d.path());
    const std::string with_nul = std::string("notes") + '\0' + "/x";
    CHECK(resolve_bad(ws, std::string_view(with_nul.data(), with_nul.size())));
}

TEST(ws_rejects_absolute) {
    TempDir d;
    Workspace ws(d.path());
    CHECK(resolve_bad(ws, "/etc/passwd"));
}

TEST(ws_rejects_parent_escape) {
    TempDir d;
    Workspace ws(d.path());
    CHECK(resolve_bad(ws, ".."));
    CHECK(resolve_bad(ws, "../etc/passwd"));
    CHECK(resolve_bad(ws, "a/../../b"));
    CHECK(resolve_bad(ws, "notes/../../ws/notes/todo.txt"));
}

// ---------------------------------------------------------------- calculator

TEST(calc_precedence) {
    double v = 0;
    std::string err;
    REQUIRE(eval("2 + 3 * 4", &v, &err));
    CHECK(v == 14);
}

TEST(calc_left_to_right) {
    double v = 0;
    std::string err;
    REQUIRE(eval("10 - 4 - 3", &v, &err));
    CHECK(v == 3);
    REQUIRE(eval("100 / 10 / 5", &v, &err));
    CHECK(v == 2);
}

TEST(calc_parentheses) {
    double v = 0;
    std::string err;
    REQUIRE(eval("(2 + 3) * 4", &v, &err));
    CHECK(v == 20);
    REQUIRE(eval("2 * (3 + 4) - 5", &v, &err));
    CHECK(v == 9);
}

TEST(calc_negative_numbers) {
    double v = 0;
    std::string err;
    REQUIRE(eval("-3 * -3", &v, &err));
    CHECK(v == 9);
}

TEST(calc_decimal_result) {
    double v = 0;
    std::string err;
    REQUIRE(eval("7 / 2", &v, &err));
    CHECK(v == 3.5);
}

TEST(calc_division_by_zero) {
    double v = 0;
    std::string err;
    CHECK(!eval("1 / (2 - 2)", &v, &err));
    CHECK(err == "division by zero");
}

TEST(calc_custom_expressions) {
    double v = 0;
    std::string err;
    REQUIRE(eval("6 * 7 + 8", &v, &err));
    CHECK(v == 50);
    REQUIRE(eval("3.5 * 2 + 1", &v, &err));
    CHECK(v == 8);
}

TEST(calc_long_expression) {
    std::string expr = "1";
    for (int i = 0; i < 2000; ++i) expr += "+1";
    double v = 0;
    std::string err;
    REQUIRE(eval(expr, &v, &err));
    CHECK(v == 2001);
}

int main(int argc, char** argv) { return mt::run_all(argc, argv); }
