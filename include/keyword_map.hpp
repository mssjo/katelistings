#ifndef KEYWORD_MAP_H
#define KEYWORD_MAP_H

#include <algorithm>
#include <map>
#include <unordered_map>

#include "katelistings_util.hpp"

namespace util{

template<typename T>
class keyword_map {
private:
    using value_type = T;
    using internal_map = std::unordered_map<std::string, value_type>;
    
    size_t max_len;
    std::map<size_t, internal_map, std::greater<size_t>> map;
    
public:
    
    keyword_map() : max_len(0), map({{0, internal_map()}}) {}
    keyword_map( std::initializer_list< std::pair<std::string, value_type> > init ) : max_len(0), map({{0, internal_map()}}) {
        size_t len;
        for(auto[key, val] : init){
            len = key.length();
            if(max_len < len)
                max_len = len;
            
            map[len][key] = val;
        }
    }
    
    value_type& operator[] (const std::string& key){
        return map[key.length()][key];
    }
    
    std::pair<typename internal_map::iterator, bool> 
    insert(const std::pair<std::string, value_type>& key_val){
        size_t len = key_val.first.length();
        if(len > max_len)
            max_len = len;
        return map[len].insert(key_val);
    }
    std::pair<typename internal_map::iterator, bool> 
    insert(std::pair<std::string, value_type>&& key_val){
        size_t len = key_val.first.length();
        if(len > max_len)
            max_len = len;
        return map[len].insert(key_val);
    }
    
    std::pair<typename internal_map::const_iterator, size_t> 
    match(const std::string& str, size_t pos = 0, bool whole_word = true) const {
        if(whole_word && util::word_char(str, pos-1))
            return std::make_pair(map.find(0)->second.end(), std::string::npos);
        
        size_t max = std::min(max_len, str.length() - pos);
        
        for(auto it = map.lower_bound(max); it != map.end(); ++it){
            const auto& [len, sub_map] = *it;
            
            if(whole_word && util::word_char(str, pos+len))
                continue;
            
            auto match = sub_map.find(str.substr(pos, len));
            
            if(match != sub_map.end())
                return std::make_pair(match, len);
        }
        
        return std::make_pair(map.find(0)->second.end(), std::string::npos);
    }    
    
};

};


#endif
