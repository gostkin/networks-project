#ifndef UTILS_H
#define UTILS_H

#include <optional>
#include <fstream>

template <typename T>
std::string GetString(const std::optional<T>& from) {
    if (from.has_value()) {
        return std::to_string(from.value());
    }
    return {};
}

std::optional<std::string> GetToken(char *path) {
    std::ifstream input_file;
    input_file.open(path, std::ios::in);
    std::string token;

    if (input_file.is_open()) {
        std::getline(input_file, token);
        return token;
    }

    return std::nullopt;
}

#endif  // UTILS_H
