#ifndef KATELISTINGS_UTIL_H
#define KATELISTINGS_UTIL_H

#include <cctype>
#include <string>

namespace util {

std::string lowercase(const std::string& str);
std::string uppercase(const std::string& str);

std::string& convert_lowercase(std::string& str);
std::string& convert_uppercase(std::string& str);

bool word_char(const std::string& str, size_t pos);

};

#endif
