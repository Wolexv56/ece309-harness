// src/tools/workspace.cpp — implementation of Workspace::resolve (spec §4.4).

#include "tools/workspace.h"

namespace fs = std::filesystem;

bool Workspace::resolve(std::string_view user_path, fs::path* out,
                        std::string* reason) const {
    try {
        reason->clear();

        if (user_path.empty()) {
            *reason = "empty path";
            return false;
        }
        if (user_path.find('\0') != std::string_view::npos) {
            *reason = "path contains NUL byte";
            return false;
        }

        fs::path p{std::string(user_path)};
        if (p.is_absolute() || p.has_root_path()) {
            *reason = "absolute paths are not allowed";
            return false;
        }

        fs::path n = p.lexically_normal();
        if (n.begin() != n.end() && *n.begin() == "..") {
            *reason = "path escapes workspace";
            return false;
        }

        *out = (n == ".") ? root() : root() / n;
        return true;
    } catch (...) {
        *reason = "invalid path";
        return false;
    }
}
