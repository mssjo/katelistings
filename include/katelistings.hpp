#ifndef KATELISTINGS_H
#define KATELISTINGS_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "file_utils.hpp"

#include "print_options.hpp"
#include "language.hpp"


struct katelistings_job {
    std::string input_file;     //empty means stdin
    std::string output_file;    //never empty
    std::string language;       //empty means default
    
    bool inlin;                 //"inline" but avoids keyword
    
    
    katelistings_job() = default;
    katelistings_job(const std::string& in, const std::string& lang, bool inlin) 
    : input_file( in ), output_file(), 
      language(lang), inlin(inlin)
    {
        if(inlin){
            output_file = input_file.empty()
                           ? "./"
                           : util::get_dir(input_file) + "katelistings/";
        }
        else{
            output_file =  input_file.empty() 
                            ? "stdin.lst" 
                            : util::replace_extension( 
                                    util::get_filename(input_file), ".lst" );
        }
    }
};

class latex_highlight {
  
private:
    std::unordered_map<std::string, language::style> default_styles;
    std::unordered_map<std::string, language> languages;    
    
    bool load_language(const std::string& lang_name, const std::string& out_dir,
        std::unordered_map< std::string, util::cref_ptr<dom_element> >& lang_map,
        std::unordered_set< std::string >& loaded,
        print_options opts
    );
    void load_language(const std::string& lang_name, const std::string& out_dir,
        std::unordered_map< std::string, util::cref_ptr<dom_element> >& lang_map,
        print_options opts);
    bool need_new_commands(const std::string& lang_name, const std::string& out_dir);
    void parse_language(const std::string& filename,
        print_options opts);
    
public:    
    void parse_default_styles(const std::string& filename, 
        print_options opts);
    
    static std::string infer_language(const std::string& file, 
        std::unordered_map< std::string, std::list<std::string> >& extensions, 
        std::unordered_map< std::string, std::list<std::string> >& glob_extensions);
    
    void do_job(const katelistings_job& job, 
        std::unordered_map< std::string, util::cref_ptr<dom_element> >& lang_map,  
        std::unordered_map< std::string, std::list<std::string> >& extensions, 
        std::unordered_map< std::string, std::list<std::string> >& glob_extensions,
        print_options opts);
    void do_inline_job(std::istream& in, 
        const std::string& filename, const std::string& output_dir, 
        std::unordered_map< std::string, util::cref_ptr<dom_element> >& lang_map,
        print_options opts );
    void process_inline_listing(util::file_parser& parser, std::ostream& out, size_t leading_space);
    
};

#endif
