#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <termios.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <algorithm>

#include "./extra.hpp"
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
std::string trim(const std::string& str) {
    auto front = std::find_if_not(str.begin(), str.end(), ::isspace);
    auto back  = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
    return (front < back) ? std::string(front, back) : "";
}
void loadElircConfig(std::unordered_map<std::string, std::string>& envVars, const std::string& customPath = "") {
    std::string path;
    Eli eli;

    // تعیین مسیر فایل .elirc
    if (!customPath.empty()) {
        path = customPath;
    } else {
        const char* home = getenv("HOME");
        if (!home) return;
        path = std::string(home) + "/.elirc";
    }

    std::ifstream in(path);
    if (!in.is_open()) return;

    std::string line;
    while (std::getline(in, line)) {
        std::vector<std::string> tokens = eli.program(line);

        for (auto& token : tokens) {
            if (!token.empty() && token[0] == '$') {
                // پردازش متغیرهایی مثل $VARsuffix
                size_t i = 1;
                while (i < token.size() && std::isalnum(token[i])) ++i;
                std::string key = token.substr(1, i - 1);
                std::string suffix = token.substr(i);
                if (envVars.count(key)) {
                    token = envVars[key];
                } else {
                    const char* val = getenv(key.c_str());
                    token = val ? val : "";
                }
                token += suffix;
            }

            else if (!token.empty() && token[0] == '~') {
                const char* home = getenv("HOME");
                if (home) {
                    token = std::string(home) + token.substr(1);
                }
            }
        }

        if (!tokens.empty()) {
            const std::string& cmd = tokens[0];
            if (cmd == "export" && tokens.size() >= 2) {
                int x = tokens[1].size();
                std::string key = tokens[1].substr(0,x-1);
                std::string value;
                if (tokens.size() == 3) {
                    value = tokens[2];
                } else {
                    const char* val = getenv(key.c_str());
                    value = val ? std::string(val) : "";
                }
                const char* env = getenv(key.c_str());
                if (env) {
                  value = std::string(env) + ":" + value;
                } else {
                  value += "" ;
                }
                setenv(key.c_str(), value.c_str(), 1);
                envVars[key] = value;
            }

            else if (cmd == "set" && tokens.size() >= 3) {
                int x = tokens[1].size();
                std::string key = tokens[1].substr(0,x-1);
                std::string value = tokens[2];
                //setenv(key.c_str(), value.c_str(), 1);
                envVars[key] = value;
            }
            else if (cmd == "unset" && tokens.size() >= 2) {
                std::string key = tokens[1];
                envVars.erase(key);
            }

            /*else {
                std::cerr << "eli:\x1b[31m Unknown command or invalid format.\x1b[0m\n";
            }*/
        }
    }
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
                if (seq[1] == 'A' && historyIndex > 0) {
                    historyIndex--;
                    buffer = history[historyIndex];
                    cursor = buffer.size();
                    clearLine(prompt);
                    std::cout << buffer << std::flush;
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
                } else if (seq[1] == 'D' && cursor > 0) {
                    cursor--;
                    std::cout << "\x1b[D" << std::flush;
                } else if (seq[1] == 'C' && cursor < buffer.size()) {
                    cursor++;
                    std::cout << "\x1b[C" << std::flush;
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
void runMultiPipeCommand(std::vector<std::string> cmd) {
    std::vector<std::vector<std::string>> commands;
    std::vector<std::string> current;

    for (const auto& part : cmd) {
        if (part == "|") {
            commands.push_back(current);
            current.clear();
        } else {
            current.push_back(part);
        }
    }
    commands.push_back(current);

    int numCommands = commands.size();
    std::vector<int[2]> pipes(numCommands - 1);

    for (int i = 0; i < numCommands - 1; ++i)
        pipe(pipes[i]);

    for (int i = 0; i < numCommands; ++i) {
        pid_t pid = fork();
        if (pid == 0) {

            if (i > 0) {
                dup2(pipes[i - 1][0], 0);
            }

            if (i < numCommands - 1) {
                dup2(pipes[i][1], 1);
            }
            for (int j = 0; j < numCommands - 1; ++j) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            std::vector<char*> args;
            for (const auto& part : commands[i])
                args.push_back(const_cast<char*>(part.c_str()));
            args.push_back(nullptr);

            execvp(args[0], args.data());
            perror("exec failed");
            exit(1);
        }
    }

    for (int i = 0; i < numCommands - 1; ++i) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    for (int i = 0; i < numCommands; ++i)
        wait(nullptr);
}


int main() {
    Extra ex;
    ex.start("welcome");
    std::unordered_map<std::string, std::string> envVars = {{"version","0.1.0"}};
    loadElircConfig(envVars);

    enableRawMode();
    std::string prompt = envVars.count("PS1") ? envVars["PS1"] : "eli$ ";
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

        history.push_back(line);
        historyIndex = history.size();
        saveToHistoryFile(historyFile, line);

        Eli eli;
        std::vector<std::string> tokens = eli.program(line);

        for (auto& token : tokens) {
            if (!token.empty() && token[0] == '$') {
                std::string key = token.substr(1);
                if (envVars.count(key))
                    token = envVars[key];
                else {
                    const char* val = getenv(key.c_str());
                    token = val ? val : "";
                }
            }if(!token.empty() && token[0] == '~') {
              std::string key = token.substr(1);
              token = getenv("HOME")+key;
            }
        }

          auto pipePos = std::find(tokens.begin(), tokens.end(), "|");
          if (pipePos != tokens.end()) {
            runMultiPipeCommand(tokens);
            continue;
        }
        if(ex.ex_commands(tokens)){
          continue;
        }
        if(envVars.count("TESTING") && envVars["TESTING"] == "ITME"){
          for(auto x : tokens)
            std::cout<<"|"<<x<<"|"<<std::endl;
        }
        if (!tokens.empty()) {
            if (tokens[0] == "set" && tokens.size() >= 2) {
                auto parts = eli.split(tokens[1], '=');
                if (parts.size() == 2)
                    envVars[parts[0]] = parts[1];
                else
                    std::cerr << "eli:\x1b[31m set: invalid format.\x1b[0m\n Use set key=value\n";
                continue;
            }

            if (tokens[0] == "unset" && tokens.size() >= 2) {
                envVars.erase(tokens[1]);
                continue;
            }
            if (tokens[0] == "source" && tokens.size() >= 2) {
              loadElircConfig(envVars,tokens[1]);
              continue;
            }
            if (tokens[0] == "cd") {
                std::string rawPath = (tokens.size() > 1) ? tokens[1] : getenv("HOME");
                if (!rawPath.empty() && rawPath[0] == '~') {
                    const char* home = getenv("HOME");
                    if (home)
                        rawPath = std::string(home) + rawPath.substr(1);
                }
                const char* path = rawPath.c_str();
                if (chdir(path) != 0)
                    std::cerr << "eli:\x1b[31m cd: failed to change directory\x1b[0m\n";;
                continue;
            }if(tokens[0] == "set"){
              for(auto x : envVars){
                std::cout<<"key: "<<x.first << "\tvalue: "<<x.second<<std::endl;
              }
              continue;
            }

            std::vector<char*> args;
            for (const auto& token : tokens)
                args.push_back(strdup(token.c_str()));
            args.push_back(nullptr);

            pid_t pid = fork();
            if (pid == 0) {
                execvp(args[0], args.data());
                std::cerr << "eli:\x1b[31m Failed to execute command: " << args[0] << "\x1b[0m\n";
                if(envVars.count("TESTING") && envVars["TESTING"] == "ITME"){
                  perror("📌 Reason");
                }
                exit(EXIT_FAILURE);
            } else {
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                    int exitCode = WEXITSTATUS(status);
                    envVars["?"] = std::to_string(exitCode);
                    if(envVars.count("TESTING") && envVars["TESTING"] == "ITME"){
                      std::cout << "\x1b[33meli exited with code: " << exitCode << "\x1b[0m\n";
                    }
                }
            }

            for (char* arg : args) free(arg);
        }
    }

    disableRawMode();
    return 0;
}
