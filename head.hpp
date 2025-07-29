#pragma once
#include <vector>
#include <string>
#include <cctype>

class Eli {
public:

  std::vector<std::string> program(const std::string& text) {
    std::vector<std::string> tokens;
    std::string buf;
    size_t i = 0;

    while (i < text.size()) {
        char ch = text[i];

        if (std::isspace(ch)) {
            ++i;
            continue;
        }

        if (ch == '\"') {
            ++i;
            while (i < text.size() && text[i] != '\"') {
                buf += text[i++];
            }
            tokens.push_back(buf);
            buf.clear();
            if (i < text.size()) ++i;
            continue;
        }
        if (ch == '\'') {
            ++i;
            while (i < text.size() && text[i] != '\'') {
                buf += text[i++];
            }
            tokens.push_back(buf);
            buf.clear();
            if (i < text.size()) ++i;
            continue;
        }

        if (std::isalpha(ch)) {
            buf += ch;
            ++i;
            while (i < text.size() && std::isalnum(text[i])) {
                buf += text[i++];
            }
            tokens.push_back(buf);
            buf.clear();
            continue;
        }

        buf += ch;
        ++i;
        while (i < text.size() && !std::isspace(text[i]) && text[i] != '\"') {
            buf += text[i++];
        }
        tokens.push_back(buf);
        buf.clear();
    }

    return tokens;
}

    std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> parts;
        std::stringstream ss(str);
        std::string item;
        while (std::getline(ss, item, delimiter)) {
            parts.push_back(item);
        }
        return parts;
    }
};
