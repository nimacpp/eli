#pragma once

class Extra {
private:


public:
  bool start(std::string command){
    if(command == "welcome"){
      std::cout<<"Welcome to \x1b[35mEli\x1b[0m, the The Epic Linux Interaction \n"
      "Type \x1b[32m\'help\'\x1b[0m for instructions on how to use Eli \n";
      return true;
    }
    return false;
  }
  bool ex_commands(std::vector<std::string> cmd){
    if (cmd[0] == "extras" && cmd.size() >= 2) {
      if(cmd[1] == "hello"){
        std::cout<<"HELLO NIMA"<<std::endl;
      }
      return true;
    }if (cmd[0] == "version") {
std::cout << R"(
▗▄▄▄▖▗▖   ▗▄▄▄▖
▐▌   ▐▌     █
▐▛▀▀▘▐▌     █
▐▙▄▄▖▐▙▄▄▖▗▄█▄▖ - The Epic Linux Interaction
═══════════════════════════════════════
📦 Version      : Eli v0.1.0
🧠 Developer    : Nimacpp
💻 Terminal     : kitty
🛠️ Built with    : C++
🎨 Theme        : Simple
🔗 github       : https://github.com/nimacpp

═════ "Built for control, designed for elegance" ═════
)" << std::endl;
      return true;
    }
    else{
      return false;
    }
  }
};
