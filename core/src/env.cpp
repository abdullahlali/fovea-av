#include "fovea/env.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <string>

namespace fovea {

namespace {

std::string trim(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }
    return value;
}

std::string strip_quotes(std::string value) {
    if (value.size() >= 2) {
        const char first = value.front();
        const char last = value.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            return value.substr(1, value.size() - 2);
        }
    }
    return value;
}

}  // namespace

void load_dotenv(const char* path, const bool override_existing) {
    std::ifstream input(path);
    if (!input) {
        return;
    }

    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }

        std::string key = trim(line.substr(0, eq));
        std::string value = strip_quotes(trim(line.substr(eq + 1)));
        if (key.empty() || value.empty()) {
            continue;
        }

        if (override_existing || std::getenv(key.c_str()) == nullptr) {
#if defined(_WIN32)
            _putenv_s(key.c_str(), value.c_str());
#else
            setenv(key.c_str(), value.c_str(), 1);
#endif
        }
    }
}

void load_project_dotenv() {
    const char* candidates[] = {
        ".env",
        "../.env",
        "../../.env",
        "../../../.env",
        "../../../../.env",
    };

    for (const char* path : candidates) {
        std::ifstream probe(path);
        if (!probe) {
            continue;
        }
        // Project .env should win over stale shell exports (common dev pitfall).
        load_dotenv(path, true);
    }
}

}  // namespace fovea
