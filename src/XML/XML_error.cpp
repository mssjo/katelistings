#include "XML.hpp"

using namespace XML;


void xml_element::error(const std::string& message) const {
    error(_source, message);
}
void xml_element::error(const source_info& source, const std::string& message){
    auto[pd, got_context] = parse_data::replicate(source);
    pd.error(got_context, message);
}
void xml_element::parse_data::error(const std::string& message) const {
    error(true, message);
}
void xml_element::parse_data::error(const std::string& message, size_t alt_col){
    col = alt_col;
    error(true, message);
}
void xml_element::parse_data::error(bool show_context, const std::string& message) const {
    std::cerr << "\nERROR in file \"" << filename << "\"";
    if(!show_context)
        std::cerr << " (inaccessible)";
    std::cerr << ", line " << line << ", column " << col;
    
    std::cerr << "\nERROR: " << message << "\n\n";
    
    if(show_context)
        show_error_context(buf, col);
    
    exit(EXIT_FAILURE);
}

void xml_element::parse_data::show_error_context(const std::string& buf, size_t col){
    const size_t max_print_length = 64;
    
    if(col >= buf.length())
        col = buf.empty() ? 0 : buf.length() - 1;
    
    if(!buf.empty() && buf.length() < max_print_length){
        std::cerr << "\t" << buf << "\n";
        std::cerr << "\t" << std::string(col, '_') << "^\n";
    }
    else if(buf.length() > max_print_length){
        if(col < max_print_length/2){
            std::cerr << "\t" << buf.substr(0, max_print_length - 3) << "...\n";
            std::cerr << "\t" << std::string(col, '_') << "^\n";
        }
        else if(buf.length() - col < max_print_length/2){
            std::cerr << "\t..." << buf.substr(buf.length() - (max_print_length - 3)) << "\n";
            std::cerr << "\t" << std::string(col - (buf.length() - max_print_length), '_') << "^\n";
        }
        else{
            std::cerr << "\t..." << buf.substr(col - (max_print_length/2) + 3, max_print_length - 6) << "...\n";
            std::cerr << "\t" << std::string(max_print_length/2, '_') << "^\n";
        }
    }
    std::cerr << "\n";
}
