#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <termios.h>
#include <sys/wait.h>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <cstring>
#include "./head.hpp"

termios orig_termios;

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void clearLine(const std::string& prompt) {
    std::cout << "\r\x1b[2K" << prompt << std::flush;
}

void saveToHistoryFile(const std::string& path, const std::string& line) {
    std::ofstream out(path, std::ios::app);
    if (out) out << line << "\n";
}

std::string readLine(const std::string& prompt, const std::vector<std::string>& history, int& historyIndex) {
    std::string buffer;
    size_t cursor = 0;
    std::cout << prompt << std::flush;

    while (true) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) continue;

        if (c == '\n') {
            std::cout << "\n";
            return buffer;
        } else if (c == 127 || c == '\b') {
            if (cursor > 0) {
                buffer.erase(cursor - 1, 1);
                cursor--;
                clearLine(prompt);
                std::cout << buffer << std::flush;
                std::cout << "\r\x1b[" << (prompt.size() + cursor) << "C" << std::flush;
            }
        } else if (c == '\x1b') {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) == 0) continue;
            if (read(STDIN_FILENO, &seq[1], 1) == 0) continue;
            if (seq[0] == '[') {
                if (seq[1] == 'A') {
                    if (!history.empty() && historyIndex > 0) {
                        historyIndex--;
                        buffer = history[historyIndex];
                        cursor = buffer.size();
                        clearLine(prompt);
                        std::cout << buffer << std::flush;
                    }
                } else if (seq[1] == 'B') {
                    if (historyIndex + 1 < (int)history.size()) {
                        historyIndex++;
                        buffer = history[historyIndex];
                    } else {
                        historyIndex = history.size();
                        buffer.clear();
                    }
                    cursor = buffer.size();
                    clearLine(prompt);
                    std::cout << buffer << std::flush;
                } else if (seq[1] == 'D') {
                    if (cursor > 0) {
                        cursor--;
                        std::cout << "\x1b[D" << std::flush;
                    }
                } else if (seq[1] == 'C') {
                    if (cursor < buffer.size()) {
                        cursor++;
                        std::cout << "\x1b[C" << std::flush;
                    }
                }
            }
        } else {
            buffer.insert(cursor, 1, c);
            cursor++;
            clearLine(prompt);
            std::cout << buffer << std::flush;
            std::cout << "\r\x1b[" << (prompt.size() + cursor) << "C" << std::flush;
        }
    }
}

std::unordered_map<std::string, std::string> envVars;


std::string trim(const std::string& str) {
    auto front = std::find_if_not(str.begin(), str.end(), ::isspace);
    auto back = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
    return (front < back) ? std::string(front, back) : "";
}

void loadElircConfig(std::unordered_map<std::string, std::string>& envVars) {
    const char* home = getenv("HOME");
    if (!home) return;

    std::ifstream in(std::string(home) + "/.elirc");
    if (!in.is_open()) return;

    std::string line;
    while (std::getline(in, line)) {
        size_t eq = line.find('=');
        if (eq != std::string::npos && eq > 0) {
            std::string key = trim(line.substr(0, eq));
            std::string value = trim(line.substr(eq + 1));

            if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }

            envVars[key] = value;
        }
    }
}



int main() {
    enableRawMode();
    loadElircConfig(envVars);
    const std::string prompt = envVars.count("PS1") ? envVars["PS1"] : "eli$ ";
    const std::string historyFile = std::string(getenv("HOME")) + "/.eli_history";
    std::vector<std::string> history;
    std::ifstream infile(historyFile);
    std::string line;
    while (std::getline(infile, line)) history.push_back(line);
    int historyIndex = history.size();

    while (true) {
        std::string line = readLine(prompt, history, historyIndex);
        if (line.empty()) continue;
        if (line == "exit") break;
        //std::cout<<"your command =>"<<line<<std::endl;
        history.push_back(line);
        historyIndex = history.size();
        saveToHistoryFile(historyFile, line);
        /*std::istringstream iss(line);
        std::vector<char*> args;
        std::string token;
        while (iss >> token) args.push_back(strdup(token.c_str()));
        args.push_back(nullptr);*/
        Eli eli;
        std::vector<std::string> tokens = eli.program(line);
        std::vector<char*> args;


        for (auto& token : tokens) {
          size_t pos = token.find('$');
          if (pos != std::string::npos) {
            std::string key = eli.extractVarName(token, pos);
            std::string value;
            if (envVars.count(key))
            value = envVars[key];
            else {
              const char* val = getenv(key.c_str());
              value = val ? val : "";
            }
            token.replace(pos, key.size() + 1, value);
          }
        }
        for (const auto& token : tokens) {
            args.push_back(strdup(token.c_str()));
        }
        if (tokens[0] == "set" && tokens.size() >= 2) {
          auto parts = eli.split(tokens[1], '=');
          if (parts.size() == 2){
            std::cout<<parts[0]<<"->"<<parts[1]<<std::endl;
            envVars[parts[0]] = parts[1];
          }
          else
          std::cerr << "eli: set: invalid format. Use set KEY=VALUE\n";
          for (char* arg : args) free(arg);
          continue;
        }
        else if (!tokens.empty() && tokens[0]  == "cd") {
          std::string rawPath = tokens.size() > 1 ? tokens[1] : getenv("HOME");
          if (!rawPath.empty() && rawPath[0] == '~') {
            const char* home = getenv("HOME");
            if (home) {
              rawPath = std::string(home) + rawPath.substr(1);
            }
          }

          const char* path = rawPath.c_str();
          if (chdir(path) != 0) {
            std::cerr << "eli: cd: failed to change directory\n";
          }
          for (char* arg : args) free(arg);
          continue;
        }else if (tokens[0] == "unset" && tokens.size() >= 2) {
          envVars.erase(tokens[1]);
          for (char* arg : args) free(arg);
          continue;
        }
        if(envVars.count("TESTING") && envVars["TESTING"] == "TRUE"){
          for(auto c : args)
          std::cout<<"|"<<c<<"|"<<std::endl;
        }
        if (!args.empty()) {
            pid_t pid = fork();
            if (pid == 0) {
                execvp(args[0], args.data());

                std::cerr << "Command failed\n";
                exit(1);
            } else {
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                  int exitCode = WEXITSTATUS(status);
                  if (exitCode != 0)
                  std::cout << "eli exited with code: " << exitCode << "\n";
                }
            }
            for (char* arg : args) free(arg);
        }
    }

    disableRawMode();
    return 0;
}
