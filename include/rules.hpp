#ifndef RULES_H
#define RULES_H

#define RULE context::rule

#define CTOR_AND_IMPL(NN) \
    virtual std::unique_ptr<RULE> init( RULE_CTOR_ARGS );         \
    virtual std::unique_ptr<RULE> clone() const;                  \
    virtual size_t match_impl( RULE_MATCH_ARGS ) const;           \
    virtual std::string name() const { return #NN "[" + INFO + "]"; }

    
struct detect_char : public RULE {
    std::string chr;
    
#define INFO chr
    
    CTOR_AND_IMPL(detect_char)
};

struct detect_2_chars : public RULE {
    char chr0, chr1;
    
#undef INFO
#define INFO std::string(1,chr0) + std::string(1,chr1)
    
    CTOR_AND_IMPL(detect_2_chars)
};

struct any_char : public RULE {
    std::string str;
    
#undef INFO
#define INFO str
    
    CTOR_AND_IMPL(any_char)
};

struct string_detect : public RULE {
    std::string str;
    bool ins;
    
    CTOR_AND_IMPL(string_detect)
};

struct word_detect : public RULE {
    std::string str;
    bool ins;
    
    CTOR_AND_IMPL(word_detect)
};

struct reg_expr : public RULE {
    std::string str;
    bool ins;
    
    CTOR_AND_IMPL(reg_expr)
    
    //Special constructor for standalone regex
    reg_expr(const std::string& regexp = "") 
    : RULE(), str(regexp), ins(false)
    {}
};

struct keyword : public RULE {
    std::string list_name;
    util::cref_ptr<util::keyword_set> keywords;
    
    bool ins;
    
#undef INFO
#define INFO list_name
    
    CTOR_AND_IMPL(keyword)
};

#undef INFO
#define INFO std::string()

struct rint : public RULE {
    CTOR_AND_IMPL(rint)
};
struct rfloat : public RULE {
    CTOR_AND_IMPL(rfloat)
};
struct hlc_oct : public RULE {
    CTOR_AND_IMPL(hlc_oct)
};
struct hlc_hex : public RULE {
    CTOR_AND_IMPL(hlc_hex)
};
struct hlc_string_char : public RULE {
    CTOR_AND_IMPL(hlc_string_char)
};
struct hlc_char : public RULE {
   CTOR_AND_IMPL(hlc_char)
};

struct range_detect : public RULE {
    char chr0, chr1;
#undef INFO
#define INFO std::string(1,chr0) + "..." + std::string(1,chr1)
    
    CTOR_AND_IMPL(range_detect)
};

struct line_continue : public RULE {
    char chr;
    
#undef INFO
#define INFO std::string(1,chr)
    
    CTOR_AND_IMPL(line_continue)
};

#undef INFO
#define INFO std::string()

struct detect_spaces : public RULE {
    CTOR_AND_IMPL(detect_spaces)
};
struct detect_identifier : public RULE {
    CTOR_AND_IMPL(detect_identifier)
};

#undef INFO

#undef CTOR_AND_IMPL
#undef RULE

#endif
