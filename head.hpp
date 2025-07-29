#pragma once

#include <vector>

class Eli {
public:
  std::vector<std::string> program(std::string text){
    std::vector<std::string> tokens {};
	  std::string buf;
    for(int i=0;i< text.length();i++){
      if(isalpha(text[i])){
			buf = text[i];
			i++;
			while(isalnum(text[i]) || text[i] == '='){
				buf += text[i];
				i++;
			}
			i--;
      tokens.push_back(buf);
      buf.clear();
    }else if(text[i] == '\"'){
			i++;
			while(text[i] != '\"'){
				buf += text[i];
				i++;
			}
			tokens.push_back(buf);
			buf.clear();
		}else if(isspace(text[i])){
			continue;
		}else {
      buf = text[i];
      i++;
      while(!isspace(text[i])){
				buf += text[i];
				i++;
			}
      i--;
			tokens.push_back(buf);
			buf.clear();
    }
  }
  return tokens;
  }
  std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        tokens.push_back(item);
    }
    return tokens;
}std::string extractVarName(const std::string& s, size_t start) {
  size_t end = start + 1;
  while (end < s.size() && std::isalnum(s[end])) ++end;
  return s.substr(start + 1, end - start - 1);
}


private:
};
