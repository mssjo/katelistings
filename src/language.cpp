#include "language.hpp"

language::language(const dom_element& defn, 
                   const std::unordered_map<std::string, style>& deflt_styles,
                   const std::unordered_map<std::string, language>& languages,
                   print_options opts
                  )

 : case_sensitive(defn.unique_element("general")
                      .element("keywords")
                      .attribute("casesensitive").or_default("true").bool_val()),
    name(         defn.attribute("name").or_error("Unnamed language").val()),    
    empty_lines(  defn.element("general")
                      .element("emptyLines"))
    
{                  
    if(PRINT_OPT(VERBOSE)){
        std::cout << INDENT(1) << "Parsing language \"" << name << "\"";
        
        if(PRINT_OPT(DEBUG))
            std::cout << "...\n";
        else
            std::cout << "\n";
    }
    
    const dom_element& hig = defn.unique_element("highlighting");
    parse_keywords(hig, opts);
    
    const dom_element& dat = hig.unique_element("itemDatas");
    parse_styles(dat, deflt_styles, opts);
        
    const dom_element& con = hig.unique_element("contexts");
    parse_contexts(con, languages, opts);
    
    if(PRINT_OPT(DEBUG))
        std::cout << INDENT(1) << "...done.\n";
}

void language::parse_keywords(const dom_element& list, print_options opts){
    for(const auto& list : list.all_elements("list")){
        
        std::string name = list.attribute("name").or_error("Unnamed keyword list");
        if(keyword_lists.find(name) != keyword_lists.end())
            list.error("Keyword list with name \"" + name + "\" already exists");
        
        if(PRINT_OPT(DEBUG))
            std::cout << INDENT(2) << "Parsing keyword list \"" << name << "\"...\n";
        
        util::keyword_set& keywords = keyword_lists[name];
        for(const auto& item : list.all_elements("item")){
            std::string keyword = item.content().nonempty("Empty keyword");
            
            if(PRINT_OPT(DEBUG))
                std::cout << INDENT(3) << "Keyword: \"" << keyword << "\"\n";
            
            auto[it, success] = keywords.insert(
                case_sensitive ? keyword : util::convert_lowercase(keyword)
            );
            
            // Apparently, duplicate keywords is not a problem -- it is featured in cpp.xml
//             if(!success)
//                 list.error("Keyword \"" + *it + "\" already defined in list \"" + name + "\"");
        }
        
        if(PRINT_OPT(DEBUG))
            std::cout << INDENT(2) << "...done.\n";
    }
}

void language::parse_styles(const dom_element& list, 
                            const std::unordered_map<std::string, style>& deflt_styles,
                            print_options opts)
{
    for(const auto& item : list.all_elements("itemData")){
        style id;
        
        id.name = item.attribute("name").or_error("Unnamed style").nonempty("Empty style name");
        
        if(PRINT_OPT(DEBUG))
            std::cout << INDENT(2) << "Parsing style \"" << id.name << "\"";
        
        if(id.name.substr(0,2) == "ds")
            item.error("Style names beginning with \"ds\" is reserved for default styles");
        if(styles.find(id.name) != styles.end())
            item.error("Style \"" + id.name + "\" already exists");
        
        auto def_iter = deflt_styles.find(item.attribute("defStyleNum").or_error());
        if(def_iter == deflt_styles.end())
            item.error("Default style \"" + item.attribute("defStyleNum").val() + "\" not defined");
        
        id.deflt_style = def_iter->second;
        
        if(PRINT_OPT(DEBUG))
            std::cout <<  " (based on \"" << id.deflt_style->name << "\"):\n";
        
        id.colour    = style::format_colour(item.attribute("color"          )
                                                .or_default(id.deflt_style->colour   ), 
                                            item);
        id.bg_colour = style::format_colour(item.attribute("backgroundColor")
                                                .or_default(id.deflt_style->bg_colour), 
                                            item);
        
        if(PRINT_OPT(DEBUG))
            std::cout << INDENT(3) << "colour " << id.colour << ", background " << id.bg_colour;
        
        id.italic        = item.attribute(              "italic"                         )
                               .or_default(id.deflt_style->italic        ? "true" : "false").bool_val();
        id.bold          = item.attribute(              "bold"                           )
                               .or_default(id.deflt_style->bold          ? "true" : "false").bool_val();
        id.underline     = item.attribute(              "underline"                      )
                               .or_default(id.deflt_style->underline     ? "true" : "false").bool_val();
        id.strikethrough = item.attribute(              "strikethrough"                  )
                               .or_default(id.deflt_style->strikethrough ? "true" : "false").bool_val();
        
        if(PRINT_OPT(DEBUG)){
            if(id.italic)
                std::cout << ", italic";
            if(id.bold)
                std::cout << ", bold";
            if(id.underline)
                std::cout << ", underline";
            if(id.strikethrough)
                std::cout << ", strikethrough";
            
            std::cout << "\n";
        }
                               
        styles[id.name] = id;
    }        
}

void language::parse_contexts(const dom_element& list,
                              const std::unordered_map<std::string, language>& languages,
                              print_options opts)
{
    
    std::deque< std::pair<std::string, util::cref_ptr<dom_element>> > todo;
    
    bool first = true;
    
    for(const auto& def : list.all_elements("context")){
        std::string con_name = def.attribute("name").or_error("Unnamed context");
        
        auto [it, success] = contexts.insert( std::make_pair(con_name, context()) );
        
        if(!success)
            def.error("Context with name \"" + it->first + "\" already exists");
        
        todo.push_back( std::make_pair(con_name, util::cref_ptr<dom_element>(def)) );
        
        if(first){
            first = false;
            default_context = it->second;
        }
    }
    
    //Marks the start of a circular dependence
    std::string first_postponed;
    std::unordered_set<std::string> done;
    
    //Simple but inefficient dependency resolution:
    //we loop around the todo list until everything is done        
    while(!todo.empty()){
        auto& [name, def] = todo.front();
        
        
        bool no_deps = true;
        for(const auto& incl : def->all_elements("IncludeRules")){
            std::string spec = incl.attribute("context").or_error();
            
            //So far, forward references to other languages are not supported
            if(spec.find("##") != std::string::npos)
                continue;
            if(done.find(spec) != done.end())
                continue;
            
            if(PRINT_OPT(DEBUG))
                std::cout << INDENT(3) << "Context \"" << name << "\" depends on \"" << spec << "\", postponing...\n";
            if(spec == first_postponed)
                def->error("Circular IncludeRules dependency detected");
            no_deps = false;
            break;
        }
        
        if(no_deps){
            first_postponed = "";
            
            //At this point, we are sure that the context only depends on
            //contexts that are already done
            
            if(PRINT_OPT(DEBUG))
                std::cout << INDENT(3) << "Parsing context \"" << name << "\"\n";
            
            contexts[name].parse(*def, *this, languages, opts);
            
            done.insert(name);
        }
        else{
            //Postpone until dependencies are resolved
            if(first_postponed.empty())
                first_postponed = name;
                    
            todo.push_back( todo.front() );
        }
        
        todo.pop_front();
    }
}

language::context_switch 
language::parse_context_switch(const std::string& def, const dom_element& src) const {
         
    context_switch con_sw;
           
    if(def.empty()){
        con_sw.target = default_context;
        return con_sw;
    }
    
    std::string err_prefix = "In context switch \"" + def + "\":\n\t";
    
    if(def.substr(0,5) == "#stay"){
        if(def.length() > 5)
            src.error(err_prefix + "\"#stay\" may not be combined with other context-switch specifications");
        
        con_sw.target = nullptr;        
        return con_sw;
    }
    
    std::string context = "";
    for(size_t pos = 0; pos < def.length();){
        if(def[pos] == '#'){
            if(pos+3 >= def.length() || def.substr(pos+1, 3) != "pop")
                src.error(err_prefix + "Malformed context-switch, \"#pop\" expected");
            
            ++con_sw.pops;
            
            pos+=4;
        }
        else if(def[pos] == '!'){
            context = def.substr(pos+1); 
            if(context.empty())
                src.error(err_prefix + "Expected context name after '!'");
            break;
        }
        else if(pos == 0){
            context = def;
            break;
        }
        else
            src.error(err_prefix + "Final context-switch must be preceded by '!'");
    }
    
    if(context.empty())
        con_sw.target = nullptr;
    else{
        auto it = contexts.find(context);
        if(it == contexts.end())
            src.error(err_prefix + "Undefined context: \"" + context + "\"");
        
        con_sw.target = it->second;
    }
    
    return con_sw;
}

util::cref_ptr<language::style> 
language::get_style(const std::string& def, const dom_element& src) const {
    if(def.empty())
        src.error("Empty style reference");
    
    auto it = styles.find(def);
    if(it == styles.end())
        src.error("Style \"" + def + "\" not defined");
    
    return it->second;
}

void language::highlight(std::istream& in, std::ostream& out, print_options opts) const {
      
    std::string buf;
    size_t pos = 0;
    std::smatch new_match;
    
    if(!std::getline(in, buf))
        return;
    if(PRINT_OPT(ECHO_INPUT))
        std::cout << buf << std::endl;
    
    context_stack stack(default_context);
    
    bool leading_space = true;
    bool normal_output = false;
    size_t rbraces;
    
    for(;;){
        
        //Handle empty lines
        if(pos == 0 && stack.curr_context().empty_line(buf, stack, empty_lines)){
            out << std::endl;
            if(!std::getline(in, buf))
                break;
            if(PRINT_OPT(ECHO_INPUT))
                std::cout << buf << std::endl;
            
            continue;
        }
        
        //Handle end-of-line
        if(pos >= buf.length()){
            if(normal_output){
                normal_output = false;
                out << std::string(rbraces, '}');
            }
            
            stack.curr_context().end_of_line(stack);
            
            out << std::endl;
            pos = 0;
            if(!std::getline(in, buf))
                //Terminate highlighting on EOF
                break;
            if(PRINT_OPT(ECHO_INPUT))
                std::cout << buf << std::endl;
            
            leading_space = true;
            continue;
        }
        
        //Try to apply rules
        auto[match_len, attr] = stack.curr_context().apply_rules(buf, pos, leading_space, stack);
        
        //Rules exhausted without a match: print character normally
        if(match_len == std::string::npos){
//             std::cout << "No rule matches\n";
            if(!normal_output){
                normal_output = true;
                rbraces = latex_format(out, *stack.curr_context().get_attribute(), PRINT_OPT(USE_COMMANDS));
            }
            
            leading_space = !latex_escape(out, buf[pos]) && leading_space;
            ++pos;
        }
        //Non-empty (non-lookahead) match
        else if(match_len > 0){
            if(normal_output){
                normal_output = false;
                out << std::string(rbraces, '}');
            }
            
//             std::cout << "Rule matches\n";
            rbraces = latex_format(out, attr ? *attr : *stack.curr_context().get_attribute(), PRINT_OPT(USE_COMMANDS));
                    
            leading_space = !latex_escape(out, buf, pos, match_len) && leading_space;
            
            out << std::string(rbraces, '}');
            pos += match_len;
        }
        else{
            //Empty match: do nothing; all switching etc. is already taken care of
        }
    }   
}

size_t language::latex_format(std::ostream& out, const language::style& st, bool use_commands) const {
    
    if(use_commands){
        name_command(out, st.name);
        out << "{";
        return 1;
    }
    
    size_t braces = 0;
    if(st.bg_colour != "FFFFFF"){
        ++braces;
        out << "\\colorbox[HTML]{" << st.bg_colour << "}{";
    }
    
    ++braces;
    out << "\\textcolor[HTML]{" << st.colour << "}{"; 
    
    if(st.bold){
        ++braces;
        out << "\\textbf{";
    }
    if(st.italic){
        ++braces;
        out << "\\textit{";
    }
    if(st.underline){
        ++braces;
        out << "\\underline{";
    }
    if(st.strikethrough){
        ++braces;
        out << "\\sout{";
    }
    
    return braces;    
}

bool language::latex_escape(std::ostream& out, char ch){
    switch(ch){
        case '\\':  out << "\\textbackslash{}";  return true;
        case '{':   out << "\\{";           return true;
        case '}':   out << "\\}";           return true;
        case 0:
        case '\f':
        case '\v':
        case '\r':                          return false;
        case '\t':
        case '\n':
        case ' ':   out << ch;              return false;
        default:    out << ch;              return true;
    }
}
bool language::latex_escape(std::ostream& out, const std::string& str, size_t pos, size_t len){
    bool only_space = true;
    for(size_t i = pos; i < pos+len; ++i)
        only_space = !latex_escape(out, str[i]) && only_space;
    
    return !only_space;
}

void language::generate_commands(const dom_element& deps, const std::string& out_dir) const {
    std::ostringstream name_esc;
    name_escape(name_esc, name);
    std::string filename = out_dir + name_esc.str() + ".lst.sty";
    std::cout << "Generating LaTeX commands to " << filename << "\n";
    std::ofstream out(filename);
    
    out << "% ID: " << get_ID() << "\n"
        << "\\NeedsTeXFormat{LaTeX2e}\n"
        << "\\ProvidesPackage{" << name << ".lst}\n"
        << "\n";
        
    for(const auto& dep : deps.all_elements("dependency"))
        out << "\\RequirePackage{" << dep.attribute("name").or_error().val() << ".lst}\n";
    out << "\n";
    
    for(const auto& [sty_name, sty] : styles){
        out << "\\newcommand{";
        name_command(out, sty_name);
        out << "}[1]{\\texttt{";
        size_t br = latex_format(out, sty, false);
        out << "#1" << std::string(br, '}') << "}}\n";
    }
}

void language::name_command(std::ostream& out, const std::string& sty_name) const {
    out << '\\';
    name_escape(out, name);
    name_escape(out, sty_name);
}

void language::name_escape(std::ostream& out, const std::string& name) {
    for(size_t i = 0; i < name.length(); ++i){
        if(std::isalpha(name[i]))
            out << name[i];
        else{
            switch(name[i]){
                case '0':
                    out << "Zero";  break;
                case '1':
                    out << "One";   break;
                case '2':
                    out << "Two";   break;
                case '3':
                    out << "Three"; break;
                case '4':
                    out << "Four";  break;
                case '5':
                    out << "Five";  break;
                case '6':
                    out << "Six";   break;
                case '7':
                    out << "Seven"; break;
                case '8':
                    out << "Eight"; break;
                case '9':
                    out << "Nine";  break;
                case '+':
                    out << "X";     break;
                case '#':
                    out << "Sharp"; break;
            }
        }
    }
}
    
    
    

