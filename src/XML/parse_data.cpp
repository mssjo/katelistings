#include "XML.hpp"

using namespace XML;

void xml_element::parse_data::load_default_entities(){
    entities["gt"] = ">";
    entities["lt"] = "<";
    entities["amp"] = "&";
    entities["apos"] = "'";
    entities["quot"] = "\"";
}

std::pair<xml_element::parse_data, bool>
xml_element::parse_data::replicate(const xml_element::source_info& source){
    
    parse_data pd(source.filename, false);
    
    while(pd.in){
        if(++pd.line == source.line){
            std::getline(pd.in, pd.buf);
            pd.col = source.column;
            
            return std::pair<xml_element::parse_data, bool>(std::move(pd), true);
        }            
        
        pd.in.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    }
    
    return std::pair<xml_element::parse_data, bool>(std::move(pd), false);
}

void xml_element::parse_data::seek(char ch, bool allow_newline, const std::string& err){
    seek_impl(std::string(1, ch), allow_newline, err, CHAR);
}
void xml_element::parse_data::seek_any_of(const std::string& str, bool allow_newline, const std::string& err){
    seek_impl(str, allow_newline, err, CHARS);
}
void xml_element::parse_data::seek_not_of(const std::string& str, bool allow_newline, const std::string& err){
    seek_impl(str, allow_newline, err, NOT_CHARS);
}
void xml_element::parse_data::seek_impl(const std::string& str, bool allow_newline, const std::string& err, seek_style style){
    for(;;){
        switch(style){
            case CHAR:  
                col = buf.find             (str[0], col); 
                break;
            case CHARS: 
                col = buf.find_first_of    (str,    col); 
                break;
            case NOT_CHARS: 
                col = buf.find_first_not_of(str,    col); 
                break;
        }
        
        if(col != std::string::npos)
            return;
        else if(allow_newline){
            std::string tmp_buf;
            if(!std::getline(in, tmp_buf)){
                if(err.empty())
                    return;
                else
                    error(err, -1);
            }
            
            std::swap(buf, tmp_buf);
            col = 0;
            ++line;
        }
        else if(!err.empty())
            error(err, -1);
        else
            return;
    }
}

std::string xml_element::parse_data::parse_string(const std::string& str) const {
    std::ostringstream ost;

    parse_string(str, ost);
     
    return ost.str();        
}

bool xml_element::parse_data::parse_string(std::ostringstream& ost, size_t mark) const { 
    return parse_string(buf, ost, mark, col);
}

bool xml_element::parse_data::parse_string(const std::string& str, std::ostringstream& ost, 
                                           size_t begin, size_t end) const {    
    
    bool only_space = true;
                                               
    end = std::min(end, str.length());
    for(size_t pos = begin; pos < end; ++pos){
//         if(pos == begin)
//             std::cout << "Parse \"" + str.substr(begin, end-begin) + "\" ";
//         std::cout << "parse '" + std::string(1,str[pos]) + "' ";
        if(str[pos] == '&'){
            
            size_t sc = str.find(';', ++pos);
            if(sc == std::string::npos)
                error("Unterminated entity reference, ';' expected\n\tIn \"" + str + "\"");
            
//             std::cout << "Parsing entity &" + str.substr(pos, sc - pos) + ";\n";
            
            if(str[pos] == '#'){
                ++pos;
                if(str[pos] == 'x')
                    ++pos;
                
                size_t n_proc;
                char ch = stoi(str.substr(pos, sc - pos), &n_proc, (str[pos-1] == 'x') ? 16 : 10);
                if(n_proc != sc - pos)
                    error("Invalid character entity");
                
                if(only_space && !std::isspace(ch))
                    only_space = false;
                
                ost << ch;
            }
            else{
            
                std::string ent = str.substr(pos, sc - pos);
                if(ent != "amp"){
                    auto match = entities.find(ent);
                    if(match == entities.end())
                        error("Undefined entity: &" + ent + ";");
                    
                    only_space = !parse_string(match->second, ost) && only_space;
                }
                else
                    ost << "&";
            }
            pos = sc;
        }
        else{
            if(only_space && !std::isspace(str[pos]))
                only_space = false;
            ost << str[pos];
        }
    }
    
//     std::cout << "Finished parsing, got \"" + ost.str() + "\" in context \"" + buf + "\"\n";
    
    return !only_space;
}
