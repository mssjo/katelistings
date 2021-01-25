#include "katelistings_util.hpp"

std::string util::lowercase(const std::string& str){
    std::string lower(str.length(), ' ');
    for(size_t i = 0; i < str.length(); ++i)
        lower[i] = std::tolower(str[i]);
    
    return lower;
}
std::string util::uppercase(const std::string& str){
    std::string upper(str.length(), ' ');
    for(size_t i = 0; i < str.length(); ++i)
        upper[i] = std::toupper(str[i]);
    
    return upper;
}

std::string& util::convert_lowercase(std::string& str){
    for(size_t i = 0; i < str.length(); ++i)
        str[i] = std::tolower(str[i]);
    
    return str;
}
std::string& util::convert_uppercase(std::string& str){
    for(size_t i = 0; i < str.length(); ++i)
        str[i] = std::toupper(str[i]);
    
    return str;
}

bool util::word_char(const std::string& str, size_t pos){
    return pos < str.size() && (std::isalnum(str[pos]) || str[pos] == '_');
}
