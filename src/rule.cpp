#include "language.hpp"

#include "katelistings_util.hpp"

#define NPOS std::string::npos
#define CONTEXT language::context
#define RULE CONTEXT::rule

//Parse attributes common to all rules
void RULE::parse_common(const dom_element& defn, const language& lang, bool allow_dynamic)
{
    std::string attr = defn.attribute("attribute").nonempty().or_default("");
    attribute = attr.empty() ? nullptr : lang.get_style(attr, defn);
    context   = lang.parse_context_switch(defn.attribute("context").or_default("#stay"), defn);
    
    dynamic         = defn.attribute("dynamic")      .or_default("false").bool_val();
    lookahead       = defn.attribute("lookAhead")    .or_default("false").bool_val();
    first_non_space = defn.attribute("firstNonSpace").or_default("false").bool_val();
    
    column = defn.attribute("column").or_default(std::to_string(NPOS)).uint_val();
    
    if(!allow_dynamic && dynamic)
        defn.error("Parsing rule \"" + defn.get_name() + "\" can not be dynamic");
}

void RULE::clone_common(RULE* clone) const {
    clone->attribute = attribute;
    clone->context   = context;
    clone->dynamic   = dynamic;
    clone->lookahead = lookahead;
    clone->first_non_space = first_non_space;
    clone->column    = column;
}

//Attempt to match rule, delegating actual matching to match_impl for each rule type
//Returns length of match, or NPOS if no match
size_t RULE::match(RULE_MATCH_ARGS, bool leading_space) const {
    if(first_non_space && (!leading_space || std::isspace(buf[pos])))
        return NPOS;
    if(column != NPOS && column != pos)
        return NPOS;
    
    size_t match_len = match_impl(RULE_MATCH_VALS);
    
    return (!lookahead || match_len == NPOS) ? match_len : 0;
}

//Check if definition contains dynamic insertions, and raises error if they are malformed
bool RULE::check_dynamic(const std::string& defn, const dom_element& src){

    bool is_dynamic = false;
    for(size_t i = 0; i < defn.length(); ++i){
        if(defn[i] == '%'){
            if(++i < defn.length() && (std::isdigit(defn[i]) || defn[i] == '%'))
                is_dynamic = true;
            else
                src.error("Malformed dynamic rule: \"" + defn + "\"");
        }
    }
    
    return is_dynamic;
}

//Do all dynamic insertions into a string
std::string RULE::get_dynamic(const std::string& str, const std::smatch& match) {
    
    std::ostringstream ost;
    
    for(size_t i = 0; i < str.length(); ++i){
        if(str[i] == '%'){
            ++i;
            if(str[i] == '%')
                ost << '%';
            else{
                size_t n = str[i] - '0';
                if(n < match.size())
                    ost << match[n];
            }
        }
        else
            ost << str[i];
    }
    
    return ost.str();
}
        
std::unique_ptr<RULE> CONTEXT::detect_char::init(RULE_CTOR_ARGS) {
    parse_common(RULE_CTOR_VALS, true);
    
    chr = defn.attribute("char").or_error();
    
    if(chr.length() != 1 && !(dynamic && check_dynamic(chr, defn))){
        defn.error("Single character expected, got \"" + chr + "\"");
    }

    return std::unique_ptr<RULE>(this);
}
std::unique_ptr<RULE> CONTEXT::detect_char::clone() const {
    detect_char *clone = new detect_char;
    clone_common(clone);
    
    clone->chr = chr;
    
    return std::unique_ptr<RULE>(clone);
}
size_t CONTEXT::detect_char::match_impl(RULE_MATCH_ARGS) const {
    char c;
    
    if(dynamic){
        std::string s = get_dynamic(chr, regex_match);
        if(s.empty())
            c = 0;
        else
            c = s[0];
    }
    else
        c = chr[0];
    
    if(buf[pos] == c)
        return 1;
    else
        return NPOS;
}

std::unique_ptr<RULE> CONTEXT::detect_2_chars::init(RULE_CTOR_ARGS) {
    parse_common(RULE_CTOR_VALS, false);
    
    chr0 = defn.attribute("char").or_error().char_val();
    chr1 = defn.attribute("char1").or_error().char_val();

    return std::unique_ptr<RULE>(this);
}
std::unique_ptr<RULE> CONTEXT::detect_2_chars::clone() const {
    detect_2_chars *clone = new detect_2_chars;
    clone_common(clone);
    
    clone->chr0 = chr0;
    clone->chr1 = chr1;

    return std::unique_ptr<RULE>(clone);
}
size_t CONTEXT::detect_2_chars::match_impl(RULE_MATCH_ARGS) const {
    if(pos+1 < buf.length() && buf[pos] == chr0 && buf[pos+1] == chr1)
        return 2;
    else
        return NPOS;
}

std::unique_ptr<RULE> CONTEXT::any_char::init(RULE_CTOR_ARGS) {
    parse_common(RULE_CTOR_VALS, false);
    
    str = defn.attribute("String").or_error();

    return std::unique_ptr<RULE>(this);
}
std::unique_ptr<RULE> CONTEXT::any_char::clone() const {
    any_char *clone = new any_char;
    clone_common(clone);
    
    clone->str = str;

    return std::unique_ptr<RULE>(clone);
}
size_t CONTEXT::any_char::match_impl(RULE_MATCH_ARGS) const {
    if( str.find(buf[pos]) != NPOS)
        return 1;
    else
        return NPOS;
}

std::unique_ptr<RULE> CONTEXT::string_detect::init(RULE_CTOR_ARGS) {
    parse_common(RULE_CTOR_VALS, true);
    
    str = defn.attribute("String").or_error();
    ins = defn.attribute("insensitive").or_default("false").bool_val();
    
    check_dynamic(str, defn);

    return std::unique_ptr<RULE>(this);
}
std::unique_ptr<RULE> CONTEXT::string_detect::clone() const {
    string_detect *clone = new string_detect;
    clone_common(clone);
    
    clone->str = str;
    clone->ins = ins;

    return std::unique_ptr<RULE>(clone);
}
size_t CONTEXT::string_detect::match_impl(RULE_MATCH_ARGS) const {
    
//     std::cout << "\t\tMatching string \"" + str + "\"\n";
    
    std::string string;
    if(dynamic)
        string = get_dynamic(str, regex_match);
    else
        string = str;
    
    if(ins){
        if(util::lowercase(buf.substr(pos, string.length())) == util::lowercase(string))
            return string.length();
    }
    else{
        if(buf.compare(pos, string.length(), string) == 0)
            return string.length();
    }
    
    return NPOS;
}

std::unique_ptr<RULE> CONTEXT::word_detect::init(RULE_CTOR_ARGS) {
    parse_common(RULE_CTOR_VALS, false);
    
    str = defn.attribute("String").or_error();
    ins = defn.attribute("insensitive").or_default("false").bool_val();

    return std::unique_ptr<RULE>(this);
}
std::unique_ptr<RULE> CONTEXT::word_detect::clone() const {
    word_detect *clone = new word_detect;
    clone_common(clone);
    
    clone->str = str;
    clone->ins = ins;

    return std::unique_ptr<RULE>(clone);
}
size_t CONTEXT::word_detect::match_impl(RULE_MATCH_ARGS) const {
    //Check for word boundary
    if( util::word_char(buf, pos-1) )
        return NPOS;
    if( util::word_char(buf, pos + str.length()) )
        return NPOS;
    
    //Check for match, like string_detect
    if(ins){
        if(util::lowercase(buf.substr(pos, str.length())) == util::lowercase(str))
            return str.length();
    }
    else{
        if(buf.compare(pos, str.length(), str) == 0)
            return str.length();
    }
    
    return NPOS;
}

std::unique_ptr<RULE> CONTEXT::reg_expr::init(RULE_CTOR_ARGS) {
    parse_common(RULE_CTOR_VALS, true);
    
    str = defn.attribute("String").or_error();
    ins = defn.attribute("insensitive").or_default("false").bool_val();
    
    if(dynamic)
        check_dynamic(str, defn);

    return std::unique_ptr<RULE>(this);
}
std::unique_ptr<RULE> CONTEXT::reg_expr::clone() const {
    reg_expr *clone = new reg_expr;
    clone_common(clone);
    
    clone->str = str;
    clone->ins = ins;

    return std::unique_ptr<RULE>(clone);
}
size_t CONTEXT::reg_expr::match_impl(RULE_MATCH_ARGS) const {
//     std::cout << "\t\ttrying to match \"" << str << "\" against \"" << buf.substr(pos) << "\"\n";
    
    try{
        std::regex regex = std::regex(dynamic ? get_dynamic(str, regex_match) : str, 
                                    ins ? (std::regex::ECMAScript | std::regex::icase) : std::regex::ECMAScript
                                );
        //TODO: force regex to match starting at pos
        std::string match_str = buf.substr(pos);
        if(regex_search(match_str, new_match, regex) && new_match.position() == 0)
            return new_match.length();
        else
            return NPOS;
        
    } catch(const std::regex_error&){
        std::cout << "Malformed regex: \"" << str << "\"\n";
        return NPOS;
    }
}

std::unique_ptr<RULE> CONTEXT::keyword::init(RULE_CTOR_ARGS) {
    parse_common(RULE_CTOR_VALS, false);
    
    ins = !lang.case_sensitive;
    
    std::string key = defn.attribute("String").or_error();
    list_name = key;
    auto it = lang.keyword_lists.find(key);
    
    if(it == lang.keyword_lists.end())
        defn.error("Undefined keyword list \"" + key + "\"");
    
    keywords = &(it->second);

    return std::unique_ptr<RULE>(this);
}
std::unique_ptr<RULE> CONTEXT::keyword::clone() const {
    keyword *clone = new keyword;
    clone_common(clone);
    
    clone->list_name = list_name;
    clone->keywords = keywords;

    return std::unique_ptr<RULE>(clone);
}
size_t CONTEXT::keyword::match_impl(RULE_MATCH_ARGS) const {
    return keywords->match(buf, pos);
}

std::unique_ptr<RULE> CONTEXT::rint::init(RULE_CTOR_ARGS) {
    parse_common(RULE_CTOR_VALS, false);

    return std::unique_ptr<RULE>(this);
}
std::unique_ptr<RULE> CONTEXT::rint::clone() const {
    rint *clone = new rint;
    clone_common(clone);
    
    return std::unique_ptr<RULE>(clone);
}
size_t CONTEXT::rint::match_impl(RULE_MATCH_ARGS) const {
    //Hand-coded regex \b[0-9]+
    
    if(util::word_char(buf, pos-1) || pos+1 >= buf.length() || !std::isdigit(buf[pos]))
        return NPOS;
    
    size_t len = 1;
    while( pos+len < buf.length() && std::isdigit(buf[pos+len])  )
        ++len;
    
    return len;
}

std::unique_ptr<RULE> CONTEXT::rfloat::init(RULE_CTOR_ARGS) {
    parse_common(RULE_CTOR_VALS, false);

    return std::unique_ptr<RULE>(this);
}
std::unique_ptr<RULE> CONTEXT::rfloat::clone() const {
    rfloat *clone = new rfloat;
    clone_common(clone);
    
    return std::unique_ptr<RULE>(clone);
}
size_t CONTEXT::rfloat::match_impl(RULE_MATCH_ARGS) const {
    //Hand-coded regex (\b[0-9]+\.[0-9]*|\.[0-9]+)([eE][-+]?[0-9]+)?
    
    size_t len;
    
    if(util::word_char(buf, pos-1))
        return NPOS;    
    
    if(buf[pos] == '.'){
        if( pos+1 >= buf.length() || !std::isdigit(buf[pos+1]) )
            return NPOS;
        
        len=2;
    }
    else{
        if( pos+2 >= buf.length() || !std::isdigit(buf[pos]) )
            return NPOS;
        
        len = 1;
        while( pos+len < buf.length() && std::isalnum(buf[pos+len])  )
            ++len;
        
        if(buf[pos+len] != '.')
            return NPOS;
        
        ++len;
    }
    while( pos+len < buf.length() && std::isdigit(buf[pos+len])  )
        ++len;
    
    size_t elen = 1;
    if( pos+len+elen < buf.length() && (buf[pos+len] == 'e' || buf[pos+len] == 'E') ){
        if(buf[pos+len+elen] == '+' || buf[pos+len+elen] == '-')
            ++elen;
        
        if( pos+len+elen >= buf.length() || !std::isdigit(buf[pos+len+elen]) )
            return len;
        
        ++elen;
        while( pos+len+elen < buf.length() && std::isdigit(buf[pos+len+elen]) )
            ++elen;
    }
    
    return len;
}

std::unique_ptr<RULE> CONTEXT::hlc_oct::init(RULE_CTOR_ARGS) {
    parse_common(RULE_CTOR_VALS, false);

    return std::unique_ptr<RULE>(this);
}
std::unique_ptr<RULE> CONTEXT::hlc_oct::clone() const {
    hlc_oct *clone = new hlc_oct;
    clone_common(clone);
    
    return std::unique_ptr<RULE>(clone);
}
size_t CONTEXT::hlc_oct::match_impl(RULE_MATCH_ARGS) const {
    //Hand-coded regex \b0[0-7]+
    
    if(util::word_char(buf, pos-1) || pos+2 >= buf.length() || buf[pos] != '0' 
        || buf[pos+1] < '0' || buf[pos+1] > '7')
        return NPOS;
    
    size_t len = 2;
    while( pos+len < buf.length() && buf[pos+len] >= '0' && buf[pos+len] <= '7' )
        ++len;
    
    return len;
}

std::unique_ptr<RULE> CONTEXT::hlc_hex::init(RULE_CTOR_ARGS) {
    parse_common(RULE_CTOR_VALS, false);

    return std::unique_ptr<RULE>(this);
}
std::unique_ptr<RULE> CONTEXT::hlc_hex::clone() const {
    hlc_hex *clone = new hlc_hex;
    clone_common(clone);
    
    return std::unique_ptr<RULE>(clone);
}
size_t CONTEXT::hlc_hex::match_impl(RULE_MATCH_ARGS) const {
    //Hand-coded regex \b0[0-7]+
    
    if(util::word_char(buf, pos-1) || pos+2 >= buf.length() || buf[pos] != '0' 
        || buf[pos+1] < '0' || buf[pos+1] > '7')
        return NPOS;
    
    size_t len = 2;
    while( pos+len < buf.length() && buf[pos+len] >= '0' && buf[pos+len] <= '7' )
        ++len;
    
    return len;
}

//Auxiliary function to hlc_char and hlc_string_char
size_t hlc_char_match(const std::string& buf, size_t pos){
    //Matches all C character escape sequences (including \e)
    
    if(pos+2 >= buf.length() || buf[pos] != '\\')
        return NPOS;
    
    size_t len;
    switch(buf[pos+1]){
        case 'a':
        case 'b':
        case 'e':
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case 'v':
        case '\"':
        case '\'':
        case '?':
        case '\\':
            return 2;
            
        case 'x':
            len = 0;
            while(pos+2+len < buf.length() && std::isxdigit(buf[pos+2+len]))
                ++len;
            
            if(len > 0)
                return len + 2;
            else
                return NPOS;
            
        case 'u': len = 4;
            if(false){  //Yay, evil case fallthrough hacking!
        case 'U': len = 8;
            }
            for(size_t i = 0; i < len; i++){
                if(!std::isxdigit(buf[pos+2+i]))
                    return NPOS;
            }
            return len + 2;
            
        default:
            for(len = 0; len <= 3; ++len){
                if(buf[pos+1+len] < '0' || buf[pos+1+len] > '8')
                    break;
            }
            
            if(len > 0)
                return len + 1;
            else
                return NPOS;
    }
}

std::unique_ptr<RULE> CONTEXT::hlc_string_char::init(RULE_CTOR_ARGS) {
    parse_common(RULE_CTOR_VALS, false);

    return std::unique_ptr<RULE>(this);
}
std::unique_ptr<RULE> CONTEXT::hlc_string_char::clone() const {
    hlc_string_char *clone = new hlc_string_char;
    clone_common(clone);
    
    return std::unique_ptr<RULE>(clone);
}
size_t CONTEXT::hlc_string_char::match_impl(RULE_MATCH_ARGS) const {
    return hlc_char_match(buf, pos);
}

std::unique_ptr<RULE> CONTEXT::hlc_char::init(RULE_CTOR_ARGS) {
    parse_common(RULE_CTOR_VALS, false);

    return std::unique_ptr<RULE>(this);
}
std::unique_ptr<RULE> CONTEXT::hlc_char::clone() const {
    hlc_char *clone = new hlc_char;
    clone_common(clone);
    
    return std::unique_ptr<RULE>(clone);
}
size_t CONTEXT::hlc_char::match_impl(RULE_MATCH_ARGS) const {
    if(pos+2 >= buf.length() || buf[pos] != '\'')
        return NPOS;
    if(buf[pos+1] != '\\' && buf[pos+2] == '\'')
        return 3;
    else{
        size_t len = hlc_char_match(buf, pos+1);
        
        if(len == NPOS || pos+1+len+1 >= buf.length() || buf[pos+1+len+1] != '\'')
            return len;
        
        else
            return len+2;
    }
}

std::unique_ptr<RULE> CONTEXT::range_detect::init(RULE_CTOR_ARGS) {
    parse_common(RULE_CTOR_VALS, false);
    
    chr0 = defn.attribute("char").or_error().char_val();
    chr1 = defn.attribute("char1").or_error().char_val();

    return std::unique_ptr<RULE>(this);
}
std::unique_ptr<RULE> CONTEXT::range_detect::clone() const {
    range_detect *clone = new range_detect;
    clone_common(clone);
    
    clone->chr0 = chr0;
    clone->chr1 = chr1;
    
    return std::unique_ptr<RULE>(clone);
}
size_t CONTEXT::range_detect::match_impl(RULE_MATCH_ARGS) const {
    if(buf[pos] != chr0)
        return NPOS;
    
    for(size_t len = 1; pos+len < buf.length(); ++len){
        if(buf[pos+len] == chr1)
            return len+1;
    }
    return NPOS;
}

std::unique_ptr<RULE> CONTEXT::line_continue::init(RULE_CTOR_ARGS) {
    parse_common(RULE_CTOR_VALS, false);
    
    chr = defn.attribute("char").or_default("\\").char_val();

    return std::unique_ptr<RULE>(this);
}
std::unique_ptr<RULE> CONTEXT::line_continue::clone() const {
    line_continue *clone = new line_continue;
    clone_common(clone);
    
    clone->chr = chr;
    
    return std::unique_ptr<RULE>(clone);
} 
size_t CONTEXT::line_continue::match_impl(RULE_MATCH_ARGS) const {
    if(pos == buf.length() - 1 && buf[pos] == chr)
        return 1;
    else
        return NPOS;
}

std::unique_ptr<RULE> CONTEXT::detect_spaces::init(RULE_CTOR_ARGS) {
    parse_common(RULE_CTOR_VALS, false);

    return std::unique_ptr<RULE>(this);
}
std::unique_ptr<RULE> CONTEXT::detect_spaces::clone() const {
    detect_spaces *clone = new detect_spaces;
    clone_common(clone);
   
    return std::unique_ptr<RULE>(clone);
}
size_t CONTEXT::detect_spaces::match_impl(RULE_MATCH_ARGS) const {
    //Hand-coded regex \s+
    
    if(!std::isspace(buf[pos]))
        return NPOS;
    
    size_t len = 1;
    while(pos+len < buf.length() && std::isspace(buf[pos+len]))
        ++len;
    return len;
}

std::unique_ptr<RULE> CONTEXT::detect_identifier::init(RULE_CTOR_ARGS) {
    parse_common(RULE_CTOR_VALS, false);

    return std::unique_ptr<RULE>(this);
}
std::unique_ptr<RULE> CONTEXT::detect_identifier::clone() const {
    detect_identifier *clone = new detect_identifier;
    clone_common(clone);
    
    return std::unique_ptr<RULE>(clone);
}
size_t CONTEXT::detect_identifier::match_impl(RULE_MATCH_ARGS) const {
    //Hand-coded regex [a-zA-Z_][a-zA-Z0-9_]*
   
    if(!std::isalpha(buf[pos]) && buf[pos] != '_')
        return NPOS;
    
    size_t len = 1;
    while(pos+len < buf.length() && (std::isalnum(buf[pos+len]) || buf[pos+len] == '_'))
        ++len;
    return len;
}


