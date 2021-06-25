
#ifndef  _parse_error_
#define  _parse_error_

#include<string>
#include<map>

class parse_error_t
{
    public:
    enum error_t
    {
        success_id,
        // lexical analysis errors
        lexical_error_id,
        unknown_identifier_id,
        number_error_id,
        // infix errors
        parenthesis_error_id,
        // postfix errors
        syntax_error_id,
        // name conflict
        name_conflict_id,
        // cyclical dependence
        cyclical_dependence_id,
        unknown_error_id
    };
    private:
    error_t m_type;
    std::string m_detail;
    public:
    parse_error_t(error_t error=unknown_error_id,const std::string&str=""):
    m_type(error),m_detail(str){}
    bool is_valid()const{return m_type==success_id;}
    error_t type()const{return m_type;}
    const std::string detail()const;
};

const std::map<parse_error_t::error_t,std::string> parse_error_map=
{
    {parse_error_t::success_id,"success"},
    {parse_error_t::lexical_error_id,"lexical error"},
    {parse_error_t::unknown_identifier_id,"unknown identifier"},
    {parse_error_t::number_error_id,"number error"},
    {parse_error_t::parenthesis_error_id,"parenthesis error"},
    {parse_error_t::syntax_error_id,"syntax error"},
    {parse_error_t::name_conflict_id,"name conflict"},
    {parse_error_t::cyclical_dependence_id,"cyclical dependence"},
    {parse_error_t::unknown_error_id,"unknown error"}
};

inline const std::string parse_error_t::detail()const
{
    if(!m_detail.empty())
    {
        return parse_error_map.find(m_type)->second+":"+m_detail;
    }
    return parse_error_map.find(m_type)->second;
}

#endif

