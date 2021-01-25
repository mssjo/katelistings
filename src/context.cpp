#include "language.hpp"

#define CONTEXT language::context

enum rule_type {
    any_char, 
    detect_char, detect_2_chars, detect_spaces, detect_identifier,
    rfloat, rint,
    hlc_oct, hlc_hex, hlc_string_char, hlc_char,
    string_detect, word_detect, range_detect,
    keyword, reg_expr,
    line_continue,
    INCLUDE_RULES
};

static const std::unordered_map<std::string, rule_type> rule_map(
    {
        {"AnyChar",             rule_type::any_char            },
        {"DetectChar",          rule_type::detect_char         },
        {"Detect2Chars",        rule_type::detect_2_chars      },
        {"DetectSpaces",        rule_type::detect_spaces       },
        {"DetectIdentifier",    rule_type::detect_identifier   },
        {"Float",               rule_type::rfloat              },
        {"HlCOct",              rule_type::hlc_oct             },
        {"HlCHex",              rule_type::hlc_hex             },
        {"HlCStringChar",       rule_type::hlc_string_char     },
        {"HlCChar",             rule_type::hlc_char            },
        {"Int",                 rule_type::rint                },
        {"keyword",             rule_type::keyword             },
        {"LineContinue",        rule_type::line_continue       },
        {"StringDetect",        rule_type::string_detect       },
        {"WordDetect",          rule_type::word_detect         },
        {"RegExpr",             rule_type::reg_expr            },
        {"RangeDetect",         rule_type::range_detect        },
        {"IncludeRules",        rule_type::INCLUDE_RULES       }
    });
    

void CONTEXT::parse(const dom_element& defn, const language& lang,
                    const std::unordered_map<std::string, language>& languages,
                    print_options opts)
{    
    
    name          = defn.attribute("name").or_error();
    
    attribute     = lang.get_style(           defn.attribute("attribute").or_error(), 
                                              defn);
    end_context   = lang.parse_context_switch(defn.attribute("lineEndContext").or_error(), 
                                              defn);
    empty_context = lang.parse_context_switch(defn.attribute("lineEmptyContext").or_default("#stay"), 
                                              defn);
    
    fallthrough = defn.attribute("fallthrough").or_default("false").bool_val();
    if(fallthrough)
        fall_context = lang.parse_context_switch(defn.attribute("fallthroughContext").or_error(), 
                                                 defn);
    
#define RULE_CASE(NN)                                                   \
        case rule_type::NN:                                             \
        rules.push_back( (new NN)->init(rule, lang) );                  \
        break;                                                          \
        
    for(const dom_element& rule : defn.all_elements()){
        
        auto match = rule_map.find(rule.get_name());
        
        if(match == rule_map.end())
            rule.error("Unknown rule type: \"" + rule.get_name() + "\"");
    
        switch(match->second){
            RULE_CASE(any_char)
            RULE_CASE(detect_char) 
            RULE_CASE(detect_2_chars) 
            RULE_CASE(detect_spaces) 
            RULE_CASE(detect_identifier)
            RULE_CASE(rfloat)
            RULE_CASE(hlc_oct) 
            RULE_CASE(hlc_hex) 
            RULE_CASE(hlc_string_char) 
            RULE_CASE(hlc_char)
            RULE_CASE(rint)
            RULE_CASE(keyword)
            RULE_CASE(line_continue)
            RULE_CASE(string_detect)
            RULE_CASE(word_detect)
            RULE_CASE(reg_expr)
            RULE_CASE(range_detect)
            
            case INCLUDE_RULES:
                include_rules(rule, lang, languages, opts);
                break;
                
        }
    
    }
#undef RULE_CASE  
}

CONTEXT::context(dom_element::const_query& empty_lines) : context("<empty line>") {
    for(const dom_element& empty_line : empty_lines.all_elements("emptyLine")){
//         std::cout << "Adding empty-line rule \"" << empty_line.attribute("regexpr").or_error().val() << "\"\n";
        rules.push_back(std::make_unique<reg_expr>(empty_line.attribute("String").or_error()));
    }
}

void CONTEXT::include_rules(const dom_element& defn, const language& lang,
                   const std::unordered_map<std::string, language>& languages,
                   print_options opts){
        
    std::string spec = defn.attribute("context").or_error();
    bool incl_attr = defn.attribute("includeAttrib").or_default("false").bool_val();
    
    size_t sep = spec.find("##");
    
    util::cref_ptr<language> src_lang;
    if(sep != std::string::npos){
        std::string lang_name = spec.substr(sep+2);
        auto lang_iter = languages.find(lang_name);
        
        if(lang_iter == languages.end())
            defn.error("Language \"" + lang_name + "\" not defined");
        
        src_lang = lang_iter->second;
    }
    else
        src_lang = lang;
    
    util::cref_ptr<context> src_con;
    
    std::string con_name = spec.substr(0, sep);
    if(con_name.empty())
        src_con = src_lang->default_context;
    else{
        auto con_iter = src_lang->contexts.find(con_name);
        if(con_iter == src_lang->contexts.end())
            defn.error("Context \"" + con_name + "\" not defined in language \"" + src_lang->name + "\"");
        
        src_con = con_iter->second;
    }
    
    if(PRINT_OPT(DEBUG))
        std::cout << INDENT(4) << "Including rules from context \"" << con_name 
                               << "\" in language \"" << src_lang->name << "\"\n";;
            
    
    for(const std::unique_ptr<rule>& rule_ptr : src_con->rules){
        auto new_rule = rule_ptr->clone();
        
        //Redirects attributes to use the destination language 
        //(otherwise they remain pointing to the source language)
        if(incl_attr && src_lang != lang){
            std::string attr = new_rule->attribute ? new_rule->attribute->name : src_con->attribute->name;
            
            new_rule->attribute = lang.get_style(attr, defn);
        }
    
        rules.push_back( std::move(new_rule) );
    }
}

bool CONTEXT::empty_line(const std::string& buf, context_stack& stack, const context& empty_lines) const {
   
    std::smatch match;
    if(!buf.empty()){
        auto[match_len, rule_ptr] = empty_lines.apply_rules(buf, 0, true, stack, match);
        
        if(match_len = std::string::npos)
            return false;
    }
    
    stack.switch_context(empty_context, stack.curr_match());
    return true;
    
}
void CONTEXT::end_of_line(context_stack& stack) const {
    stack.switch_context(end_context, stack.curr_match());
}

std::pair< size_t, util::cref_ptr<language::style> > 
CONTEXT::apply_rules(const std::string& buf, size_t pos, bool leading_space, context_stack& stack) const {
    
    std::smatch match;
    auto[match_len, rule] = apply_rules(buf, pos, leading_space, stack, match, stack.curr_match());
    
    if(match_len == std::string::npos)
        return std::make_pair( match_len, nullptr );
    
    stack.switch_context( rule->context, match );
    return std::make_pair( match_len, rule->attribute );
}


std::pair< size_t, util::cref_ptr<CONTEXT::rule> > 
CONTEXT::apply_rules(const std::string& buf, size_t pos, bool leading_space, context_stack& stack,
                     std::smatch& new_match, const std::smatch& old_match) const 
{
    for(const std::unique_ptr<rule>& rule : rules){
        size_t match_len = rule->match(buf, pos, old_match, new_match, leading_space);
        
        if(match_len != std::string::npos){
            return std::make_pair( match_len, util::cref_ptr<CONTEXT::rule>(*rule) );
        }
    }
    
    if(fallthrough){
        stack.switch_context(fall_context, stack.curr_match());
        return stack.curr_context().apply_rules(buf, pos, leading_space, stack, new_match, old_match);
    }
    
    return std::make_pair( std::string::npos, nullptr );
}
