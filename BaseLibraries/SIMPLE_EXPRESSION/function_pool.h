
#ifndef  _function_pool_
#define  _function_pool_

#include "expression_parser.h"
//#include <iostream>

#include <memory>


using real_t=float;
inline real_t convert_to_float(const std::string&str){return std::stof(str);}

// функции на числах с плавающей запятой
using fp_expression_t=
expression_t<real_t,function_ptr_t<real_t,const std::string&>,arithmetics_fl&(~mod_id)>;

class CFunctionPool
{
    using fp_expression_ptr_t=std::unique_ptr<fp_expression_t>;
    using str_citerator_t=std::string::const_iterator;
    std::vector<fp_expression_ptr_t> m_reg_functions;
    std::vector<fp_expression_ptr_t> m_unreg_functions;

    std::map<std::string,const fp_expression_t*> m_functions_map;

    constant_parser_t<real_t> m_constant_parser;
    function_parser_t<real_t> m_func_parser;

    bool m_is_name(const std::string& name)const;

    public:
    parse_error_t RegisterFunction(const std::string& name,
                       const std::vector<std::string>& vars,
                  str_citerator_t begin,str_citerator_t end);
    bool RegisterConstant(const std::string& name,real_t real);

    std::pair<parse_error_t,const fp_expression_t*>
    CreateFunction(const std::vector<std::string>& vars,
                                   str_citerator_t begin,
                                     str_citerator_t end);
    const fp_expression_t* GetFunction(const std::string& name)const;
    fp_expression_t::stonum_t GetNumberParser()const
    {
        return convert_to_float;
    }

    void Clear();
    CFunctionPool(){m_reg_functions.reserve(100);}
    ~CFunctionPool(){/*std::cout<<"fpool delete\n";*/Clear();}
};

#endif

