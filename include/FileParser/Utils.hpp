#pragma once
#include <expected>
#include <format>
#include <string>

namespace FileParser::utils {
    template <typename T>
    auto getUnexpected(
        const std::expected<T, std::string>& expected, std::string_view errorMsg
    ) -> std::unexpected<std::string> {
        return std::unexpected(std::format("{}: {}", errorMsg, expected.error()));
    }

    template <typename T> requires std::is_integral_v<T>
    auto ceilDivide(T a, T b) -> T {
        return a / b + (a % b != 0 ? 1 : 0);
    };
}
