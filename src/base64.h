#pragma once
#include <string>
#include <vector>

class Base64 {
public:
  static std::string Encode(const std::string &in);
  static std::string Decode(const std::string &in);
};
