#include "tokenizer.h"
#include <cctype>

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static char to_lower(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A' + 'a';
    return c;
}

std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string current;

    for (size_t i = 0; i < text.size(); i++) {
        char c = text[i];
        if (is_alpha(c)) {
            current += to_lower(c);
        } else if (c == '\'' && !current.empty() && i + 1 < text.size() && is_alpha(text[i + 1])) {
            current += c;
        } else {
            if (current.size() >= 2) {
                tokens.push_back(current);
            }
            current.clear();
        }
    }
    if (current.size() >= 2) {
        tokens.push_back(current);
    }
    return tokens;
}
