
#ifndef  _parse_config_
#define  _parse_config_


#include "plot_builder.h"
#include "../BaseLibraries/SIMPLE_EXPRESSION/function_pool.h"
#include "../BaseLibraries/GEOMETRY/primitives.h"

using str_citerator=std::string::const_iterator;
using point3D_t=Point3D<float>;


struct parse_config_result_t
{
    parse_error_t m_parse_result;
    int m_index_error_string;

    parse_config_result_t():
    m_parse_result(parse_error_t::unknown_error_id,"")
    {}

    parse_config_result_t(parse_error_t pet,int _int):
    m_parse_result(pet),m_index_error_string(_int)
    {}

    std::vector<std::pair<animate_chars_t,plot3D_chars_t>> m_plots;
    bool is_valid()const{return m_parse_result.is_valid();}
};

parse_config_result_t ParseConfig(std::string&file,CFunctionPool&);
std::string ErrorReport(const parse_config_result_t& pcr);

#endif

