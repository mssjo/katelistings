#include "katelistings.hpp"

using fp = util::file_parser;

std::string get_ID(){
    //TODO: more portable version with no risk for collision
    return std::to_string( getpid() );
}


using namespace util;

std::string latex_highlight::infer_language(const std::string& file, 
    std::unordered_map< std::string, std::list<std::string> >& extensions, 
    std::unordered_map< std::string, std::list<std::string> >& glob_extensions)
{
    if(get_extension(file).empty()){
        std::cerr << "ERROR: Cannot infer language from file without extension\n"
                  << "       \"" << file << "\",\n"
                  << "       explicit language choice (-l <language>) required\n";
        exit(EXIT_FAILURE);
    }
    
    auto ext = get_extension(file);

    auto ext_iter = extensions.find(ext);
    
    if(ext_iter == extensions.end()){
        for( ext_iter = glob_extensions.begin(); ext_iter != glob_extensions.end(); ++ext_iter ){
            if( ext_iter->first.compare(0, ((std::string) ext).length(), ext) == 0 )
                break;
        }
    }
    if(ext_iter == glob_extensions.end()){
        std::cerr << "ERROR: No language associated with file extension \"" << ext << "\",\n"
                  << "       explicit language choice (-l <language>) required\n";
        exit(EXIT_FAILURE);
    }
    
    const auto& list = ext_iter->second;
    
    if(list.size() > 1){
        std::cerr << "ERROR: Several languages are associated with this extension:\n";
        size_t i = 0;
        for(const auto& name : ext_iter->second)
            std::cerr << "\t\t" << i++ << ": " << name << "\n";
        std::cerr << "       Explicit language choice (-l <language>) required\n";
        exit(EXIT_FAILURE);
    }
    
    return list.front();
}
        

void latex_highlight::do_job(const katelistings_job& job, 
        std::unordered_map< std::string, cref_ptr<dom_element> >& lang_map,  
        std::unordered_map< std::string, std::list<std::string> >& extensions, 
        std::unordered_map< std::string, std::list<std::string> >& glob_extensions,
        print_options opts)
{    
    std::istream* in;
    
    std::string lang_name;
    
    if(job.input_file.empty()){
        in = &std::cin;
        
        if(!job.inlin && job.language.empty()){
            std::cerr << "ERROR: Cannot infer language from stream input,\n"
                      << "       explicit language choice (-l <language>) required\n";
            exit(EXIT_FAILURE);
        }
        
        if(PRINT_OPT(NORMAL))
            std::cout << "Highlighting standard input...\n";
    }
    else {
        if(!file_exists( job.input_file )){
            std::cerr << "ERROR: Input file \"" << job.input_file << "\" does not exist\n";
            exit(EXIT_FAILURE);
        }
        
        if(!job.inlin){
            if(job.language.empty())
                lang_name = infer_language(job.input_file, extensions, glob_extensions);
            else
                lang_name = job.language;
        }
        
        if(PRINT_OPT(NORMAL))
            std::cout << "Highlighting file \"" << job.input_file << "\"...\n";
        in = new std::ifstream(job.input_file);
    }
    
    if(job.inlin){
        do_inline_job(*in, job.input_file, job.output_file, lang_map, opts);
    }
    else{
        std::ofstream out(job.output_file);
        
        //Find specified language if not already loaded
        load_language(lang_name, util::get_dir(job.output_file), lang_map, opts);
        
        if(PRINT_OPT(NORMAL))
            std::cout << "    Using language \"" + lang_name + "\"\n";
            
        out << "\\begin{alltt}\n";
        
        languages.find(lang_name)->second.highlight(*in, out, opts);
                
        out << "\\end{alltt}\n";
        
        if(PRINT_OPT(NORMAL))
            std::cout << "...done. Output written to \"" + job.output_file + "\".\n";
        
        out.close();
        
    }
    
    //Deletes the in pointer if it was owned
    if(!job.input_file.empty())
        delete in;
    
}

void latex_highlight::do_inline_job(std::istream& in, 
        const std::string& filename, const std::string& output_dir, 
        std::unordered_map< std::string, cref_ptr<dom_element> >& lang_map,
        print_options opts){
    
    const std::string name_base = (output_dir.empty() ? "./" : output_dir + "/")
                                + replace_extension(get_filename(filename), "") 
                                + "_";
    
    file_parser parser(in);
    
    for(size_t lst_counter = 0;;){
        parser.set_mark();
        if(!parser.seek("\\begin{katelistings}", fp::consume))
            break;
        
        //Check for comment (is fooled by escaped comment char)
        if(parser.seek('%', fp::backwards | fp::single_line | fp::lookahead))
            continue;
        else
            ++lst_counter;
        
        parser.seek('{', fp::consume, "Missing language argument");
                
        parser.set_mark();
        parser.seek('}', 0, "Language argument not closed, '}' expected");
        
        std::string lang_name = parser.substr();
        auto lang_iter = lang_map.find(lang_name);
        if(lang_iter == lang_map.end())
            parser.error("Language \"" + lang_name + "\" not recognised");
        
        load_language(lang_name, output_dir, lang_map, opts);            
        
        std::string out_file = name_base + std::to_string(lst_counter) + ".lst";
        
        if(PRINT_OPT(VERBOSE))
            std::cout << "Processing listing " << lst_counter 
                      << " in language \"" << lang_name << "\"...\n";
                
        parser.advance_line();
        parser.set_mark();
        parser.seek_not_of(fp::whitespace, fp::single_line);
        
        std::stringstream tmp;
        
        process_inline_listing(parser, tmp, parser.substr().length());
        
        std::ofstream out(out_file);
        
        if(!out.good())
            parser.error("Unable to write to file \"" + out_file + "\"");
                
        languages.find(lang_name)->second.highlight(tmp, out, opts);
        
        if(PRINT_OPT(VERBOSE))
            std::cout << "...done. Output written to \"" << out_file << "\".\n";
        
        out.close();
    }
}

void latex_highlight::process_inline_listing(file_parser& parser, std::ostream& out, size_t leading_space){
    for(;;){
        parser.set_mark();
        parser.seek('\n');
        out << parser.substr() << '\n';
        
        if(!parser.advance_line())
            parser.error("File ended prematuely, \"\\end{katelistings}\" expected");
                    
        for(size_t i = 0; parser && i < leading_space; ++i){
            if(parser.match("\\end{katelistings}", fp::consume))
                return;
                
            ++parser;
        }
    }
}

void latex_highlight::load_language(const std::string& lang_name, const std::string& out_dir,
        std::unordered_map< std::string, cref_ptr<dom_element> >& lang_map, print_options opts)
{
    std::unordered_set< std::string > loaded;
    load_language(lang_name, out_dir, lang_map, loaded, opts);
}

bool latex_highlight::load_language(const std::string& lang_name, const std::string& out_dir,
        std::unordered_map< std::string, cref_ptr<dom_element> >& lang_map,
        std::unordered_set< std::string >& loaded, print_options opts
){
    
    auto lang_iter = lang_map.find(lang_name);
    if(lang_iter == lang_map.end())
        return false;
    
    //Ignore already parsed languages
    auto existing = languages.find(lang_name);
    if(existing != languages.end()){
        //...but still generate commands if needed
        if(PRINT_OPT(USE_COMMANDS) && need_new_commands(lang_name, out_dir))
            existing->second.generate_commands(*(lang_iter->second), out_dir);
        
        return true;
    }
    
    if(PRINT_OPT(DEBUG))
        std::cout << "        Resolving language dependency \"" << lang_name << "\"\n";
    
    //Check that we don't get stuck in a loop
    auto[iter, success] = loaded.insert(lang_name);
    if(!success)
        lang_iter->second->error("Circular language dependency detected");
    
    //Load all its dependencies
    for(const auto& dep : lang_iter->second->all_elements("dependency")){
        if(!load_language(dep.attribute("name").or_error().val(), out_dir, lang_map, loaded, opts))
            lang_iter->second->error("Language dependency \"" + dep.attribute("name").val() + "\" not defined");
    }
    
    //Parse it, knowing that all dependencies are already parsed
    parse_language(lang_iter->second->attribute("path").or_error(), opts);
    if(PRINT_OPT(USE_COMMANDS))
        languages.find(lang_name)->second.generate_commands(*(lang_iter->second), out_dir);
    
    return true;
}

bool latex_highlight::need_new_commands(const std::string& lang_name, const std::string& out_dir){
    std::ifstream in(out_dir + "/" + lang_name + ".lst.sty");
    
    if(!in.good())
        return true;
    
    file_parser parser(in);
    if(!parser.match("% ID: " + get_ID()))
        return true;
    
    return false;
}
      
    

void latex_highlight::parse_default_styles(const std::string& filename, print_options opts){
    dom_element file;
    file.parse_json(filename);
    
    if(PRINT_OPT(DEBUG))
        file.print();
        
    using Style = language::style;
    
    auto text_styles = file.unique_element("JSON-root").unique_element("text-styles").or_error();
    
    for(const dom_element& def : text_styles.all_elements()){
        Style ds;
        
        ds.name = "ds" + def.get_name();
        
        ds.deflt_style = nullptr;
        
#define GET_CONT(NN, DD) def.unique_element(NN).content().or_default(DD)
#define GET_TYPE(NN, DD) def.unique_element(NN).attribute("type").or_default(DD)
        
        ds.colour    = Style::format_colour(GET_CONT("text-color",       "#000000"), def);
        ds.bg_colour = Style::format_colour(GET_CONT("background-color", "#ffffff"), def);
        
        ds.italic        = GET_TYPE("italic",        "false").bool_val();
        ds.bold          = GET_TYPE("bold",          "false").bool_val();
        ds.underline     = GET_TYPE("underline",     "false").bool_val();
        ds.strikethrough = GET_TYPE("strikethrough", "false").bool_val();
        
        default_styles[ds.name] = ds;
        
#undef OBTAIN
    }
}

void latex_highlight::parse_language(const std::string& filename, print_options opts){
    
    if(default_styles.empty()){
        std::cerr << "ERROR: missing style";
        exit(EXIT_FAILURE);
    }
    
    dom_element file;
    file.parse_xml(filename);
    
    if(PRINT_OPT(DEBUG))
        file.print();
    
    const dom_element& defn = file.unique_element("language").or_error();
    
    languages.insert( std::make_pair(
        defn.attribute("name").or_error().val(), 
        language(defn, default_styles, languages, opts)
    ));
}
    
