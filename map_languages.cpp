#include <list>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "dom.hpp"

using namespace DOM;

bool check_extension(const dom_element& src, std::string& ext){
    if(ext.empty())
        src.error("Empty extension");
    if(ext[0] != '.')
        src.error("Extension must begin with a dot");
    
    for(size_t i = 1; i < ext.length(); ++i){
        if(i == ext.length()-1 && ext[i] == '*'){
            ext = ext.substr(0, ext.length() - 1);
            return true;
        }
        
        if(!std::isalnum(ext[i]) && std::string("._-+()").find(ext[i]) == std::string::npos)
        {
            src.error("Invalid character in file extension: '" + ext.substr(i,1) + "'");
        }
    }
    
    return false;
        
}

struct ext_data {
    std::string ext;
    bool glob;
    std::map<int, std::list<std::string>, std::greater<int>> assocs;
};

int main(int argc, char** argv){
    std::unordered_map< std::string, std::string > lang_paths;
    std::unordered_map< std::string, std::unordered_set< std::string > > lang_dependencies;
    std::unordered_map< std::string, ext_data > extensions;
   
    dom_element lang_file;
        
    for(size_t i = 1; i < argc; i++){
        std::string path = std::string(argv[i]);
        
        std::cout << "Scanning " << path << "\n";
        
        lang_file.parse_xml(path);
//         std::cout << "Parsed " << path << "\n";
        
//         lang_file.print();
        
        const dom_element& lang = lang_file.unique_element("language").or_error();
        
        std::string name = lang.attribute("name").or_error();
        
        auto name_iter = lang_paths.find(name);
        if(name_iter != lang_paths.end())
            lang_file.error("Name clash: Language \"" + name + "\" already defined");
        else
            lang_paths[name] = path;
        
        std::string ext = lang.attribute("extensions").or_default("");
        
        for(
            size_t beg = ext.find('.', 0), end = ext.find_first_of(",;", beg);
            beg < ext.length(); 
            beg = ext.find('.', end), end = ext.find_first_of(",;", beg)
        ){       
            std::string e = ext.substr(beg, end - beg);
            int prio = lang.attribute("priority").or_default("0").int_val();  
            
            ext_data& ed = extensions[e];
            
            if(ed.assocs.empty()){
                ed.glob = check_extension(lang, e);
                ed.ext = e;
            }
            
            ed.assocs[prio].push_back(name);                
        
            std::cout << "    Associating extension \"" << e << "\" with language \"" << name << "\" at priority " << prio << "\n";
        }
        
        for(const auto& context : lang.unique_element("highlighting")
                                      .unique_element("contexts").or_error()
                                      .all_elements("context")
           ){
            for(const auto& incl : context.all_elements("IncludeRules")){
                std::string src = incl.attribute("context").or_error();
                
                size_t dblhash = src.find("##");
                if(dblhash == std::string::npos)
                    continue;
                
                lang_dependencies[name].insert( src.substr(dblhash+2) );
            }
        }
        
    }
    
    //Temporary manual printing until
    //TODO: programmatic dom creation
    
    std::ofstream out("language_map.xml");
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n\n";
    
    out << "<!-- This is the language and extension map for katelistings. -->\n\n";
    
    out << "<language-map>\n";
    out << "    <languages>\n";
    for(const auto& [name, path] : lang_paths){
        out << "        <language name=\"" << name << "\" path=\"" << path << "\">\n";
        for(const auto& dep : lang_dependencies[name]){
            auto it = lang_paths.find(dep);
            
            out << "            <dependency name=\"" << dep << "\"";
            
            if(it == lang_paths.end()){
                std::cerr << "\n"
                          << "WARNING: Language \"" + name + "\" includes rules from \"" + dep + "\",\n"
                          << "         but no such language exists.\n";
                          
                out << " valid=\"false\"";
            }
            else
                out << " path=\"" << it->second << "\"";
            
            out << " />\n";
        }
        out << "        </language>\n";
    }
    out << "    </languages>\n";
    out << "    <extensions>\n";
    for(const auto& [e, ed] : extensions){
        out << "        <extension string=\"" << ed.ext << "\"";
        if(ed.glob)
            out << " glob=\"true\"";
        out << " >\n";
            
        for(const auto& [prio, langs] : ed.assocs){
            for(const auto& lang : langs)
                out << "            <association priority=\"" << prio << "\" language=\"" << lang << "\"/>\n";
        }
        out << "        </extension>\n";
    }
    out << "    </extensions>\n";
    out << "</language-map>\n";
    
    out.flush();
    
    std::cout << "\n"
              << "Results written to language_map.xml.\n"
              << "Katelistings data setup is now complete.\n";
    
    return EXIT_SUCCESS;
}
