#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <iostream>
#include <stack>
#include <string>
#include <list>
#include <unordered_map>
#include <utility>
#include <memory>
#include <regex>

#include "dom.hpp"
#include "keyword_set.hpp"
#include "ref_ptr.hpp"

#include "print_options.hpp"

#include "unistd.h"
std::string get_ID();

#define RULE_CTOR_ARGS const dom_element& defn, const language& lang
#define RULE_CTOR_VALS defn, lang
#define RULE_MATCH_ARGS const std::string& buf, size_t pos, const std::smatch& regex_match, std::smatch& new_match
#define RULE_MATCH_VALS buf, pos, regex_match, new_match

using namespace DOM;

class language{
    
    std::string name;
            
public:
    struct style {
        std::string name;
        
        util::cref_ptr<style> deflt_style;
        
        std::string colour;
        std::string bg_colour;
        
        bool italic, bold, underline, strikethrough;
        
        static std::string format_colour(const std::string& col, const dom_element& defn);
    };
        
private:

    class context;
    
    struct context_switch {
        size_t pops;
        util::cref_ptr<context> target;
        
        context_switch() : pops(0), target(nullptr) {}
    };
    
    class context_stack {
        std::stack< std::pair<util::cref_ptr<context>, const std::smatch> > stack;
        
    public:
        
        const context& curr_context() const { return *(stack.top().first); }
        const std::smatch& curr_match() const { return stack.top().second; }
        
        void switch_context(const context_switch& con_sw, const std::smatch& new_match = std::smatch());
        
        explicit context_stack(const util::cref_ptr<context>& def) : stack( {{def, std::smatch()}} ) 
        {}
    };  //context_stack
    
    class context{
                        
        std::string name;
        util::cref_ptr<style> attribute;
        
        context_switch end_context;
        context_switch empty_context;
        context_switch fall_context;
        
        bool fallthrough;
        
    public:
        
        class rule {
            
        protected:
            
            bool dynamic;
            bool lookahead;
            bool first_non_space;
            
            size_t column;
            
            rule() 
            : attribute(nullptr), context(), 
              dynamic(false), lookahead(false), first_non_space(false),
              column(std::string::npos) 
            {}
            
            virtual size_t match_impl(RULE_MATCH_ARGS) const = 0;
            
            void parse_common(RULE_CTOR_ARGS, bool allow_dynamic);
            void clone_common(rule* clone) const;
            
            static bool check_dynamic(const std::string& str, const dom_element& defn);
            static std::string get_dynamic(const std::string& str, const std::smatch& match);

            
        public:
            
            util::cref_ptr<style> attribute;
            context_switch context;
            
            size_t match(RULE_MATCH_ARGS, bool leading_space) const;
            
            virtual std::string name() const = 0;
            
            virtual std::unique_ptr<rule> init(RULE_CTOR_ARGS) = 0;
            virtual std::unique_ptr<rule> clone() const  = 0;
        
        };  //rule
        
//Delegate all subclasses of rule to separate file for convenience
#include "rules.hpp"

    private:
        std::list< std::unique_ptr<rule> > rules;

        void include_rules(const dom_element& defn, const language& lang,
                           const std::unordered_map<std::string, language>& languages,
                           print_options opts);
        
        
        std::pair< size_t, util::cref_ptr<rule> > 
        apply_rules(const std::string& buf, size_t pos, bool leading_space, context_stack& stack,
                    std::smatch& new_match, const std::smatch& old_match = std::smatch()) const;
        
    public:
        context(const std::string n = "") 
        : name(n), attribute(nullptr), 
          end_context(), empty_context(), fall_context(), fallthrough(false), 
          rules() {};
        explicit context(dom_element::const_query& empty_lines);        
        
        void parse(const dom_element& defn, const language& lang,
                   const std::unordered_map<std::string, language>& languages,
                   print_options opts);
        
        bool empty_line(const std::string& buf, context_stack& stack, const context& empty_lines) const;
        void end_of_line(context_stack& stack) const;
        
        util::cref_ptr<style> get_attribute() const { return attribute; }
        const std::string&     get_name() const { return name; }
        
        
        std::pair< size_t, util::cref_ptr<style> > 
        apply_rules(const std::string& buf, size_t pos, bool leading_space, context_stack& stack) const;
        
    };  //context   
    
    friend class context;
    
    std::unordered_map<std::string, util::keyword_set> keyword_lists;
    std::unordered_map<std::string, context> contexts;
    std::unordered_map<std::string, style> styles;
    
    bool case_sensitive;
    context empty_lines;
    util::cref_ptr<context> default_context;
    
    void parse_keywords(const dom_element& list, print_options opts);
    void parse_styles(const dom_element& list, 
                      const std::unordered_map<std::string, style>& deflt_styles, 
                      print_options opts);
    void parse_contexts(const dom_element& list,
                        const std::unordered_map<std::string, language>& languages,
                        print_options opts);
    
    void name_command(std::ostream& out, const std::string& sty_name) const;
    static void name_escape(std::ostream& out, const std::string& name);
    
    util::cref_ptr<style> get_style     (const std::string& defn, const dom_element& src) const;
    context_switch  parse_context_switch(const std::string& defn, const dom_element& src) const;    
    
public:
    void generate_commands(const dom_element& deps, const std::string& out_dir) const;

    language(const dom_element& defn, 
             const std::unordered_map<std::string, style>& deflt_styles,
             const std::unordered_map<std::string, language>& languages,
             print_options opts);
    
    void highlight(std::istream& in, std::ostream& out, print_options opts) const;
    
    const std::string& get_name() const { return name; }
    
    size_t latex_format(std::ostream& out, const style& attr, bool use_commands = false) const;
    static bool latex_escape(std::ostream& out, char ch);
    static bool latex_escape(std::ostream& out, const std::string& str, size_t pos, size_t len);
    
};  //language

#endif
