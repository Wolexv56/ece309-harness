#include "core/sentinel_scanner.h"

#include <stdexcept>

SentinelScanner::SentinelScanner(std::string sentinel)
    : sentinel_(std::move(sentinel)) {
    if (sentinel_.empty()) {
        throw std::invalid_argument("Sentinel must not be empty");
    }
}

SentinelScanner::Out SentinelScanner::feed(std::string_view chunk) {
    std::string combined = pending_;
    combined.append(chunk.data(), chunk.size());

    const std::size_t found = combined.find(sentinel_);
    if (found != std::string::npos) {
        pending_.clear();
        return {combined.substr(0, found), true};
    }

    const std::size_t keep = sentinel_.size() - 1;
    if (combined.size() <= keep) {
        pending_ = std::move(combined);
        return {std::string(), false};
    }

    const std::size_t emit_count = combined.size() - keep;
    std::string safe = combined.substr(0, emit_count);
    pending_ = combined.substr(emit_count);
    return {std::move(safe), false};
}

SentinelScanner::Out SentinelScanner::flush() {
    std::string safe = std::move(pending_);
    pending_.clear();
    return {std::move(safe), false};
}
