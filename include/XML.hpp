#ifndef XML_H
#define XML_H

#include <cctype>
#include <iostream>
#include <fstream>
#include <list>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "cref_ptr.hpp"

namespace XML {
    
class xml_element {
    using _ptr = std::unique_ptr<xml_element>;
    using _list = std::list<_ptr>;
    using _indir_list = std::list< util::cref_ptr<xml_element> >;
    using _indir_map = std::unordered_map<std::string, _indir_list>;
    using _str_map = std::unordered_map<std::string, std::string>;
    
    friend class query;
    struct parse_data;
    
public:
    struct source_info {
        std::string filename;
        size_t line;
        size_t column;
        
        explicit source_info(const parse_data& pd) 
        : filename(pd.filename), line(pd.line), column(pd.col) {}
        
        source_info()
        : filename(""), line(0), column(-1) {}
        
        bool exists() const { return !filename.empty() && line > 0; }
    };
    
    class query {
    public:
        enum query_type {
            ELEMENTS, ATTRIBUTE, CONTENTS
        };
        
    private:
        
        std::string query_str;
        util::cref_ptr<xml_element> target;
        
        query_type type;
        
        enum {
            VALID, EMPTY, INVALID, DEFAULTED
        } validity;
        
        class query_iterator {
        public:
            using iterator_category = std::forward_iterator_tag;    
            using value_type = xml_element;                            
            using difference_type = ptrdiff_t;                      
            using pointer = xml_element*;                            
            using reference = xml_element&;      
            
        private:
            
        xml_element::_indir_list::const_iterator iter;
            
        public:
            query_iterator(xml_element::_indir_list::const_iterator begin) : iter(begin) {}
            
            query_iterator& operator++() {
                ++iter;
                return *this;
            }
            
            const xml_element& operator* () const {
                return **iter;
            }
            
            friend bool operator== (const query_iterator& a, const query_iterator& b)
            { return a.iter == b.iter; }
            friend bool operator!= (const query_iterator& a, const query_iterator& b)
            { return a.iter != b.iter; }
        };
        
        void validate();
        
    public:
        
        query(const xml_element& target, const std::string& query, query_type type)
        : target(target), query_str(query), type(type), validity(VALID) { validate(); }
        
        query& all_elements();
        query& elements(const std::string& name);
        query& attribute(const std::string& attr);
        query& contents();
        
        query& non_empty(const std::string& err = "");
        const xml_element& unique(const std::string& err = "");
        
        query& or_error(const std::string& err = "");
        query& or_default(const std::string& def);
                
        std::string val() const;
        
        std::string string_val() const { return val(); }
        bool bool_val() const;
        char char_val() const;
        long int int_val() const;
        size_t uint_val() const;
        double float_val() const;
        
        operator std::string() const { return val(); }
        
        bool is_valid() const { return validity != INVALID; }
        
        query_iterator begin() const;
        query_iterator end() const;
    };
    
private:
    struct parse_data {
        const std::string filename;
        std::ifstream in;
        
        std::string buf;
        
        size_t line;
        size_t col;
        
        std::unordered_map<std::string, std::string> entities;
        
        parse_data() = delete;
        parse_data(const parse_data&) = default;
        parse_data(parse_data&&) = default;
        
        void seek(char ch, bool allow_newline, const std::string& err = "");
        void seek_any_of(const std::string& str, bool allow_newline, const std::string& err = "");
        void seek_not_of(const std::string& str, bool allow_newline, const std::string& err = "");
        
        std::string parse_string(const std::string& str) const;
        bool parse_string(std::ostringstream& ost, size_t mark = 0) const;
        bool parse_string(const std::string& str, std::ostringstream& ost, 
                                size_t begin = 0, size_t end = std::string::npos) const;
                                
                                
        void error(const std::string& message) const;
        void error(const std::string& message, size_t alt_col);
        void error(bool show_context, const std::string& message) const;
        
        static void show_error_context(const std::string& buf, size_t col);
    
        
        parse_data(const std::string& filename, bool error)
        : filename(filename), in(filename), buf(""), line(0), col(0) {
            if(error && !in)
                throw std::runtime_error("File not found: " + filename);
        };
        
        void load_default_entities();
        
        static std::pair<parse_data, bool> replicate(const source_info& source);
        
    private:
        enum seek_style {
            CHAR, CHARS, NOT_CHARS
        };
        void seek_impl(const std::string& str, bool allow_newline, const std::string& err, seek_style style);
    };
    
private:
        
    std::string _name;
    std::string _content;
    source_info _source;
    
    _list elem_list;
    _indir_map elem_map; 
    
    _str_map attributes;
    
    void add_element(_ptr&& sub);
    
    enum tag_type {
        START, PROLOGUE,
        XML_DECL, DOCTYPE, ENTITY, ELEMENT,
        OPEN, CLOSE, EMPTY,
        COMMENT
    };
    void parse_xml(parse_data& pd, tag_type context);
    tag_type parse_xml_tag(parse_data& pd, tag_type context);
    static std::pair<std::string, std::string> parse_xml_keyval(parse_data& pd, 
                                                                bool equal_sign = true, bool parse = true);
    void parse_xml_doctype(parse_data& pd);
    void parse_xml_entity(parse_data& pd);
    
    void parse_json_object(parse_data& pd);
    void parse_json_array(parse_data& pd);
    void parse_json_value(parse_data& pd, const std::string& tag);
                
    static void validate_name(parse_data& pd, const std::string& name);
    
    
public:
    
    xml_element() : _name(""), _content(""), _source(), elem_list(), elem_map(), attributes() {};
    virtual ~xml_element() = default;
    xml_element(const xml_element&) = delete;
    xml_element(xml_element&&) = default;
    xml_element& operator= (const xml_element&) = delete;
    xml_element& operator= (xml_element&&) = default;
    
    /** @brief parses an XML file into a tree rooted at this node. */
    void parse_xml(const std::string& filename);
    /** @brief parses a JSON file, converting it into XML structure. */
    void parse_json(const std::string& filename);
    
    void error(const std::string& message) const;
    static void error(const source_info& source, const std::string& message);
    
    void print() const;
    void print(std::ostream& out, size_t indent = 0) const;
    static void unparse_string(std::ostream& out, const std::string& str);
    
    const std::string& name()    const { return _name;    }
    const source_info& source()  const { return _source;  }
    
    query all_elements() const;
    query elements(const std::string& name) const;
    query attribute(const std::string& attr) const;
    query contents() const;
};

};  //namespace


#endif
