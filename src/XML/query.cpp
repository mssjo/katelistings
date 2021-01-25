
#include "XML.hpp"

#include "katelistings_util.hpp"

using namespace XML;
using Query = xml_element::query;

void Query::validate() {
    if(validity != VALID)
        return;
    
    if(type == query_type::ELEMENTS){
        if(target->elem_map.find(query_str) == target->elem_map.end())
            validity = INVALID;
    }
    else if(type == query_type::ATTRIBUTE){
        if(target->attributes.find(query_str) == target->attributes.end())
            validity = INVALID;
    }
    else if(type == query_type::CONTENTS){
        if(target->_content.empty())
            validity = EMPTY;
    }
    else
        target->error("Invalid query type");
}

Query& Query::all_elements(){
    return elements("");
}
Query& Query::elements(const std::string& name){
    if(type == query_type::ATTRIBUTE)
        target->error("Illegal operation: requesting elements of attribute");
    if(type == query_type::CONTENTS)
        target->error("Illegal operation: requesting elements of contents");
    
    if(validity == VALID){
        target = *(target->elem_map.find(query_str)->second.back());
        query_str = name;
        
        validate();
    }
    else
        query_str += "/" + name;
    
    return *this;
}
Query& Query::attribute(const std::string& name){
    if(type == query_type::ATTRIBUTE)
        target->error("Illegal operation: requesting attribute of attribute");
    if(type == query_type::CONTENTS)
        target->error("Illegal operation: requesting attribute of contents");
    
    type = query_type::ATTRIBUTE;
    if(validity == VALID){
        query_str = name;
        
        validate();
    }
    
    return *this;
}
Query& Query::contents(){
    if(type == query_type::ATTRIBUTE)
        target->error("Illegal operation: requesting contents of attribute");
    if(type == query_type::CONTENTS)
        target->error("Illegal operation: requesting contents of contents");
    
    type = query_type::CONTENTS;
    if(validity == VALID){
        target = *(target->elem_map.find(query_str)->second.back());
        query_str = "";
        
        if(target->_content.empty())
            validity = EMPTY;
    }
    
    return *this;
}

Query& Query::non_empty(const std::string& err){
    //This does not bother with nonexistent things -- use or_error for that
    if(validity == INVALID)
        return *this;
    
    if(type == query_type::ELEMENTS){
        if(target->elem_map.find(query_str)->second.empty())
            target->error(err.empty() ? "Non-empty list of elements required" : err);
    }
    else if(val().empty()){
        target->error(err.empty() ? std::string("Non-empty ") + (type == query_type::ATTRIBUTE ? "attribute" : "contents") + " required" : err);
    }
    
    return *this;
}
    
const xml_element& Query::unique(const std::string& err){
    if(type == query_type::ATTRIBUTE)
        target->error("Illegal operation: requesting element of attribute");
    if(type == query_type::CONTENTS)
        target->error("Illegal operation: requesting element of contents");
    
    if(validity == VALID){
        const auto& list = target->elem_map.find(query_str)->second;
        
        if(list.size() != 1)
            target->error(err.empty() ? "Unique element \"" + query_str + "\"requested, but " + std::to_string(list.size()) + " found" : err);
        
        return *(list.back());
    }
    else
        target->error(err.empty() ? "Unique element \"" + query_str + "\" requested, but not found (or parent is invalid)" : err);
}

Query& Query::or_error(const std::string& err) {
    if(validity == INVALID){
        if(err.empty()){
            if(type == query_type::ATTRIBUTE)
                target->error("Attribute \"" + query_str + "\" requested but not found");
            else if(type == query_type::CONTENTS)
                target->error("Contents requested but not found");
            else
                target->error("Elements " + (query_str.empty() ? "" : "<" + query_str + ">" ) + " requested but not found");
        }
        else
            target->error(err);
    }
    return *this;
}

Query& Query::or_default(const std::string& def) {
    if(validity == VALID)
        return *this;
    else{
        if(type == query_type::ELEMENTS)
            target->error("Illegal operation: assigning default value to elements");
        
        query_str = def;
        validity = DEFAULTED;
    }
    return *this;
}
       
std::string Query::val() const {
    if(type == query_type::ELEMENTS)
        target->error("Illegal operation: extracting value from elements");
    
    switch(validity){
        case EMPTY:
        case VALID:
            if(type == query_type::ATTRIBUTE)
                return target->attributes.find(query_str)->second;
            else
                return target->_content;
        case INVALID:
            target->error(std::string("Illegal operation: extracting value from nonexistent ") 
                            + (type == query_type::ATTRIBUTE ? "attribute" : "contents"));
        case DEFAULTED:
            return query_str;
    }
}

bool Query::bool_val() const {
    std::string str = val();
    
    if(std::isdigit(str[0]))
        return int_val();
        
    std::string ins = util::lowercase(str);
        
    if(ins == "true")
        return true;
    else if(ins == "false")
        return false;
    else
        target->error("Attribute \"" + query_str + "\" has value \"" + str + "\", true/false expected");
}
char Query::char_val() const {
    std::string str = val();
    
    if(str.length() != 1)
        target->error("Attribute \"" + query_str + "\" has value \"" + str + "\", single character expected");
    else
        return str[0];
}
long int Query::int_val() const {
    size_t pos = 0;
    std::string str = val();
    
    long int v = std::stol(str, &pos, 0);
    while(pos < str.length() && std::isspace(str[pos]))
        ++pos;
    
    if(pos != str.length())
        target->error("Attribute \"" + query_str + "\" has value \"" + str + "\", valid integer expected");
    
    return v;
}

size_t Query::uint_val() const {
    size_t pos = 0;
    std::string str = val();
    
    size_t v = std::stoul(str, &pos, 0);
    while(pos < str.length() && std::isspace(str[pos]))
        ++pos;
    
    if(pos != str.length())
        target->error("Attribute \"" + query_str + "\" has value \"" + str + "\", valid unsigned integer expected");
    
    return v;
}

double Query::float_val() const {
    size_t pos = 0;
    std::string str = val();
    
    double v = std::stod(str, &pos);
    while(pos < str.length() && std::isspace(str[pos]))
        ++pos;
    
    if(pos != str.length())
        target->error("Attribute \"" + query_str + "\" has value \"" + str + "\", valid floating-point number expected");
    
    return v;
}

Query::query_iterator Query::begin() const {
    if(type == query_type::ATTRIBUTE)
        target->error("Illegal operation: iterating over attribute");
    if(type == query_type::CONTENTS)
        target->error("Illegal operation: iterating over contents");
    
    if(validity != VALID)
        return query_iterator(target->elem_map.find("")->second.cend());
    
    return query_iterator(target->elem_map.find(query_str)->second.cbegin());
}
Query::query_iterator Query::end() const {
    if(type == query_type::ATTRIBUTE)
        target->error("Illegal operation: iterating over attribute");
    if(type == query_type::CONTENTS)
        target->error("Illegal operation: iterating over contents");
    
    if(validity != VALID)
        return query_iterator(target->elem_map.find("")->second.cend());
    
    return query_iterator(target->elem_map.find(query_str)->second.cend());
}
