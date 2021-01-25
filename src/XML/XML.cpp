
#include <algorithm>

#include "XML.hpp"
#include "keyword_map.hpp"

using namespace XML;

void xml_element::parse_xml(const std::string& filename){
    elem_list.clear();
    elem_map.clear();
    
    parse_data pd(filename, true);
    pd.load_default_entities();
    
    pd.seek_not_of("", true);
    if(!pd.in)
        return;
    
    parse_xml(pd, tag_type::START);
}
                
void xml_element::parse_xml(parse_data& pd, tag_type context){
    std::ostringstream cont;
    bool only_space = true;
    
    size_t mark = pd.col;
    
    for(;;){
        
        if(context == tag_type::DOCTYPE){
            pd.seek_not_of(" \t", true, "Premature end of file: ']' expected");
            
            if(pd.buf[pd.col] == ']'){
                ++pd.col;
                return;
            }
            if(pd.buf[pd.col] != '<')
                pd.error("Rogue character inside doctype declaration");
        }
        else{
            pd.seek('<', false);
            
            std::cout << pd.buf.substr(mark, pd.col-mark) << std::endl;
            
            if(pd.parse_string(cont, mark)){
                if(_name.empty())
                    pd.error("Rogue character outside tag");
                else
                    only_space = false;
            }                
                
            if(pd.col >= pd.buf.length()){            
                pd.seek_not_of("", true, _name.empty() ? "" : "Premature end of file: <" + _name + "> not closed");
            
                if(!pd.in)
                    return;
                
                cont << '\n';
                continue;
            }       
        }
                
        //If we made it this far, we have the beginning of a tag
        ++pd.col;
        _ptr sub = std::make_unique<xml_element>();
        
        tag_type type = sub->parse_xml_tag(pd, context);
        
        switch(type){
            case OPEN:
                sub->parse_xml(pd, tag_type::OPEN);
                add_element(std::move(sub));
                break;
                
            case CLOSE:
                if(sub->_name != _name)
                    pd.error("Tag mismatch: <" + _name + "> terminated by <" + sub->_name + ">");
                else{
                    _content = only_space ? "" : cont.str();
                        
                    return;
                }
                
            case ENTITY:
            case ELEMENT:
                break;
                
            case DOCTYPE:
                context = tag_type::OPEN;
                break;
               
            case XML_DECL:
                context = tag_type::PROLOGUE;
                //fallthrough
            case EMPTY:
                add_element(std::move(sub));
                break;
        }
        
        mark = pd.col;
    }
}

xml_element::tag_type xml_element::parse_xml_tag(parse_data& pd, tag_type context){
    
    _source = source_info(pd);
    
    const util::keyword_map<tag_type> tag_prefixes (
        {
            {"!--",         tag_type::COMMENT   },
            {"!DOCTYPE",    tag_type::DOCTYPE   },
            {"!ENTITY",     tag_type::ENTITY    },
            {"!ELEMENT",    tag_type::ELEMENT   },
            {"/",           tag_type::CLOSE     },
            {"?xml",        tag_type::XML_DECL  },
        });
    
    tag_type type;
    
//     std::cout << "Matching \"" + pd.buf.substr(pd.col, 10) + "\"...\n";
    
    auto [match, match_len] = tag_prefixes.match(pd.buf, pd.col, false);
    
    if(match_len == std::string::npos){
//         std::cout << "No match\n";
        type = OPEN;
    }
    else{
//         std::cout << "Matched \"" + match->first + "\"\n";
        
        type = match->second;
        pd.col += match_len;
    }
        
    switch(type){
        case COMMENT:
            for(;;){
                pd.seek('-', true, "Premature end of file: comment not closed");
                if(++pd.col < pd.buf.length() && pd.buf[pd.col] == '-'){
                    if(++pd.col < pd.buf.length() & pd.buf[pd.col] == '>'){
                        ++pd.col;
                        return type;
                    }
                    else
                        pd.error("Non-terminal \"--\" in comment");
                }
            }
            
        case ELEMENT:
            if(context != tag_type::DOCTYPE)
                pd.error("Element specification outside doctype declaration");
            
            //Element specifications are presently ignored
            pd.seek('>', true, "Premature end of file: element specification not closed");
            ++pd.col;
            return type;
            
        case ENTITY:
            if(context != tag_type::DOCTYPE)
                pd.error("Entity definition outside doctype declaration");
            
            parse_xml_entity(pd);
            
            return type;
            
        case DOCTYPE:
            if(context != tag_type::PROLOGUE)
                pd.error("Doctype declaration outside prologue");
            
            parse_xml_doctype(pd);
            
            return type;
            
        case XML_DECL:
            if(context != tag_type::START)
                pd.error("XML declaration outside prologue");
            break;
           
        case OPEN:
        case CLOSE:
            if(context == tag_type::START)
                pd.error("<?xml ... ?> expected before regular tags");
            if(context == tag_type::DOCTYPE)
                pd.error("Regular XML tag inside doctype declaration");
            break;
    }
    
    if(type == XML_DECL)
        _name = "?xml";
    else{
        size_t mark = pd.col;
        pd.seek_any_of(" \t?/>", false);
        _name = pd.buf.substr(mark, pd.col - mark);
            
//         std::cout << "Parsed tag name: \"" << ptr->_name << "\"" << std::endl;
        validate_name(pd, _name);
    }
        
    for(;;){
        pd.seek_not_of(" \t", true, "Premature end of file: '>' expected");
        
        //Now pd.col points to a non-whitespace character
        
        if(pd.buf[pd.col] == '/'){
            if(type != OPEN)
                pd.error("Invalidly formatted tag");
            
            if(++pd.col >= pd.buf.length()|| pd.buf[pd.col] != '>')
                pd.error("Expected '>' after '/'");
                   
            ++pd.col;
            return EMPTY;
        }
        if(pd.buf[pd.col] == '>'){
            if(type != OPEN && type != CLOSE)
                pd.error("Invalidly formatted tag");
            
            ++pd.col;
            return type;
        }
        if(pd.buf[pd.col] == '?'){
            if(type != XML_DECL)
                pd.error("Invalidly formatted tag");
            
            if(++pd.col >= pd.buf.length() || pd.buf[pd.col] != '>')
                pd.error("Expected '>' after '?'");
            
            ++pd.col;
            return type;
        }
            
        
        //Seems like we have an attribute...
        
        if(type == CLOSE)
            pd.error("Closing tags cannot have attributes");
        
        auto[key, val] = parse_xml_keyval(pd);
        
        attributes[key] = val;
    }
}

void xml_element::validate_name(xml_element::parse_data& pd, const std::string& name){
    if(name.empty())
        pd.error("Empty name");
    
    if( !(std::isalpha(name[0]) || std::string(":_").find(name[0]) != std::string::npos) )
        pd.error("Invalid character in name", pd.col - name.length());
    
    for(size_t i = 1; i < name.length(); i++){
        if( !(std::isalpha(name[i]) || std::isdigit(name[i]) || std::string(".-:_").find(name[i]) != std::string::npos) )
            pd.error("Invalid character in name", pd.col - name.length() + i);
    }     
}


void xml_element::add_element(_ptr&& sub){
    auto sub_iter = elem_list.insert(elem_list.cend(), std::move(sub));
    
    elem_map[(*sub_iter)->_name].push_back( **sub_iter );
    elem_map[""] .push_back( **sub_iter );
}

void xml_element::parse_xml_doctype(parse_data& pd){
    pd.seek_any_of("[>", true, "Premature end of file: '[' or '>' expected");
    if(pd.buf[pd.col] == '>'){
        ++pd.col;
        return;
    }
    
    ++pd.col;
    
    parse_xml(pd, tag_type::DOCTYPE);
    
    pd.seek_not_of(" \t", true, "Premature end of file: '>' expected");
    if(pd.buf[pd.col] != '>')
        pd.error("Doctype declaration must end after ']'");
    
    ++pd.col;
}
void xml_element::parse_xml_entity(parse_data& pd){
    pd.seek_not_of(" \t", true, "Premature end of file: entity name expected");
    
    auto[it, success] = pd.entities.insert( parse_xml_keyval(pd, false, false) );
    
    if(!success)
        pd.error("Entity &" + it->first + "; already defined"); 
    
    pd.seek_not_of(" \t", true, "Premature end of file: '>' expected");
    if(pd.buf[pd.col] != '>')
        pd.error("Extra characters in entity definition, '>' expected");
    
    ++pd.col;
}

std::pair<std::string, std::string> xml_element::parse_xml_keyval(parse_data& pd, 
                                                                  bool equal_sign, bool parse){
    size_t mark = pd.col;
    pd.seek_any_of(" \t=?/>", false);
    if(pd.col < pd.buf.length() && std::string("?/>").find(pd.buf[pd.col]) != std::string::npos)
        pd.error("Missing value in key-value pair");
    
    std::string key = pd.buf.substr(mark, pd.col - mark);
    validate_name(pd, key);
    
    if(equal_sign){
        pd.seek_not_of(" \t", true, "Premature end of file: '=' expected");
        if(pd.buf[pd.col] != '=')
            pd.error("Expected '=' after attribute key");
        ++pd.col;
    }
    
    pd.seek_not_of(" \t", true, "Premature end of file: value expected");
    if(pd.buf[pd.col] != '"' && pd.buf[pd.col] != '\'')
        pd.error("Attribute value must be quoted");
        
    char quote_char = pd.buf[pd.col];
    ++pd.col;
    
    std::ostringstream val;
    for(mark = pd.col;;){
        
        pd.seek(quote_char, false);
        
        if(pd.col >= pd.buf.length()){
            if(parse)
                pd.parse_string(val, mark);
            else
                val << pd.buf.substr(mark, pd.col-mark);
            
            val << '\n';
            mark = 0;
            
            pd.seek_not_of("", true, "Premature end of file: end quote expected");
        }
        else{
            if(parse)
                pd.parse_string(val, mark);
            else
                val << pd.buf.substr(mark, pd.col-mark);
            
            break;
        }
    }
    
    ++pd.col;
    
//     if(!equal_sign && !parse)
//         std::cout << "Key-val pair \"" + key + "\" \"" + val.str() + "\"\n";
    return std::make_pair(key, val.str()); 
}

void xml_element::parse_json(const std::string& filename){
    elem_list.clear();
    elem_map.clear();
    
    parse_data pd(filename, true);
    pd.load_default_entities();
    
    pd.seek_not_of(" \t", true);
    if(!pd.in)
        return; //empty file
                
    parse_json_value(pd, "");
    
    pd.seek_not_of(" \"", true);
    if(pd.in)
        pd.error("Rogue character outside JSON object");
        
}

void xml_element::parse_json_object(parse_data& pd){    
    
//     std::cout << "Parsing JSON object\n";
    
    _source = source_info(pd);
    
    pd.seek_not_of(" \t", true, "File ended prematurely, ']' expected");
    
    if(pd.buf[pd.col] == '}'){
        ++pd.col;
        return;
    }
    else if(pd.buf[pd.col] == ']')
        pd.error("Mismatched brackets: '[' terminated by '}'");
    
    for(;;){
        
        pd.seek_not_of(" \t", true, "File ended prematurely, '}' expected");
        if(pd.buf[pd.col] != '"')
            pd.error("Rogue character in JSON object");
        
        size_t mark = ++pd.col;
        pd.seek('"', false, "Unterminated element name, '\"' expected");
        
        std::string tag = pd.buf.substr(mark, pd.col - mark);
        
        pd.seek(':', true, "File ended prematurely, ':' expected");
        ++pd.col;
        pd.seek_not_of(" \t", true, "File ended prematurely, value expected");
        
//         std::cout << "Tag: \"" << tag << "\"\n";
        _ptr sub = std::make_unique<xml_element>();
        sub->parse_json_value(pd, tag);
        
        add_element(std::move(sub));
        
        pd.seek_not_of(" \t", true, "File ended prematurely, '}' expected");
        
        if(pd.buf[pd.col] == '}')
            break;
        else if(pd.buf[pd.col] == ']')
            pd.error("Mismatched brackets: '{' terminated by ']'");
        else if(pd.buf[pd.col] != ',')
            pd.error("Values must be separated by ','");
        
        ++pd.col;
    }
    
    ++pd.col;
}
void xml_element::parse_json_array(parse_data& pd){
    
//     std::cout << "Parsing JSON array\n";
    
    _source = source_info(pd);
    
    pd.seek_not_of(" \t", true, "File ended prematurely, ']' expected");
    
    if(pd.buf[pd.col] == ']'){
        ++pd.col;
        return;
    }
    else if(pd.buf[pd.col] == '}')
        pd.error("Mismatched brackets: '[' terminated by '}'");
    
    for(;;){
        _ptr item = std::make_unique<xml_element>();
        item->parse_json_value(pd, "*");
        
        add_element(std::move(item));
        
        pd.seek_not_of(" \t", true, "File ended prematurely, ']' expected");
        
        if(pd.buf[pd.col] == ']')
            break;
        else if(pd.buf[pd.col] == '}')
            pd.error("Mismatched brackets: '[' terminated by '}'");
        else if(pd.buf[pd.col] != ',')
            pd.error("Values must be separated by ','");
        
        ++pd.col;
        pd.seek_not_of(" \t", true, "File ended prematurely, value expected");
    }
    
    ++pd.col;
}

void xml_element::parse_json_value(parse_data& pd, const std::string& tag){
    size_t mark;
    
    switch(pd.buf[pd.col]){
        case '[':
            ++pd.col;
            parse_json_array(pd);
            break;
            
        case '{':
            ++pd.col;
            parse_json_object(pd);
            break;
            
        case '"':
            mark = ++pd.col;
            pd.seek('"', false, "Unterminated value string, '\"' expected");
                        
            _content = pd.buf.substr(mark, pd.col - mark);
//             std::cout << "Value: \"" << _content << "\"\n";
            ++pd.col;
            break;
            
        default:
            if(std::isdigit(pd.buf[pd.col])){
                mark = pd.col;
                pd.seek_not_of("0123456789", false);
                _content = pd.buf.substr(mark, pd.col - mark);
            }
            else if(pd.buf.substr(pd.col, 4) == "true"){
                _content = "true";
                pd.col += 4;
            }
            else if(pd.buf.substr(pd.col, 5) == "false"){
                _content = "false";
                pd.col += 5;
            }
            else if(pd.buf.substr(pd.col, 4) == "null"){
                _content = "null";
                pd.col += 4;
            }
            else{
                mark = pd.col;
                pd.seek_any_of(" \t,", false);
                pd.error("Invalid element value: \"" + pd.buf.substr(mark, pd.col-mark) + "\"");
            }
    }
    
    _name = tag;
}

void xml_element::print() const {
    print(std::cout);
}
void xml_element::print(std::ostream& out, size_t indent) const {
    if(_name.empty()){
        for(const xml_element& sub : all_elements())
            sub.print(out, indent);
        return;
    }
    
    out << std::string(indent*4, ' ');
    
    out << "<" << _name;
    
    for(auto& attr : attributes){
        out << " " << attr.first << "=\""; 
        unparse_string(out, attr.second);
        out << "\"";
    }
        
    if(elem_list.empty() && _content.empty())
        out << (_name == "?xml" ? "?>" : "/>") << "\n";
    else{
        out << ">";
        unparse_string(out, _content);
        
        if(!elem_list.empty()){
            out << "\n";
            for(const xml_element& sub : all_elements())
                sub.print(out, indent+1);
            out << std::string(indent*4, ' ');
        }
        
        out << "</" << _name << ">\n";
    }
}

void xml_element::unparse_string(std::ostream& out, const std::string& str){
    for(size_t i = 0; i < str.length(); ++i){
        switch(str[i]){
            case '<':   out << "&lt;";      break;
            case '>':   out << "&gt;";      break;
            case '&':   out << "&amp;";     break;
            case '"':   out << "&quot;";    break;
            case '\'':  out << "&apos;";    break;
            default:    out << str[i];
        }
    }
}


xml_element::query xml_element::all_elements() const {
    return elements("");
}
xml_element::query xml_element::elements(const std::string& name) const {
    return query(*this, name, query::query_type::ELEMENTS);
}
xml_element::query xml_element::attribute(const std::string& attr) const {
    return query(*this, attr, query::query_type::ATTRIBUTE);
}
xml_element::query xml_element::contents() const {
    return query(*this, "", query::query_type::CONTENTS);
}


        
