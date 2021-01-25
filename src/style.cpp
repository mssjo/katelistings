#include "katelistings.hpp"

std::string language::style::format_colour(const std::string& col, const dom_element& src){
    std::string err = "Invalid colour \"" + col + "\"\n\t(Colours must be specified as \"#rgb\" or \"#rrggbb\" where r,g,b are hexadecimal digits)";
    
//     std::cout << "Parsing colour \"" << col << "\"..." <<std::endl;
    
    if(col.empty())
        src.error(err);
    
    size_t offs = (col[0] == '#') ? 1 : 0;
    
    std::string result(6, '0');
    if(col.length() == 6+offs){
        for(size_t i = 0; i < 6; ++i){
            if(std::isxdigit(col[i+offs]))
                result[i] = std::toupper(col[i+offs]);
            else
                src.error(err);
        }
    }
    else if(col.length() == 3+offs){
        for(size_t i = 0; i < 3; ++i){
            if(std::isxdigit(col[i+offs]))
                result[2*i] = std::toupper(col[i+offs]);
            else
                src.error(err);
        }
    }
    else
        src.error(err);
    
    return result;
}
