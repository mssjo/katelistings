#ifndef KEYWORD_SET_H
#define KEYWORD_SET_H

#include <algorithm>
#include <map>
#include <unordered_set>

#include "katelistings_util.hpp"

namespace util {

class keyword_set {
private:
    using internal_set = std::unordered_set<std::string>;
    
    size_t max_len;
    std::map<size_t, internal_set, std::greater<size_t>> set;
    
public:
    
    keyword_set() : max_len(0), set() {}
    keyword_set( std::initializer_list<std::string> init ) : max_len(0), set() {
        size_t len;
        for(auto key : init){
            len = key.length();
            if(max_len < len)
                max_len = len;
            
            set[len].insert(key);
        }
    }
        
    std::pair<internal_set::const_iterator, bool> insert(const std::string& key){
        size_t len = key.length();
        if(len > max_len)
            max_len = len;
        return set[len].insert(key);
    }
    std::pair<internal_set::const_iterator, bool> insert(std::string&& key){
        size_t len = key.length();
        if(len > max_len)
            max_len = len;
        return set[len].insert(key);
    }
    
    size_t match(const std::string& str, size_t pos = 0, bool whole_word = true) const {
        if(whole_word && util::word_char(str, pos-1))
            std::string::npos;
        
        size_t max = std::min(max_len, str.length() - pos);
        
        for(auto it = set.lower_bound(max); it != set.end(); ++it){
            const auto& [len, sub_map] = *it;
            
            if(whole_word && util::word_char(str, pos+len))
                continue;
            
            auto match = sub_map.find(str.substr(pos, len));
            
            if(match != sub_map.end())
                return len;
        }
        
        return std::string::npos;
    }    
    
};

};


#endif
