#include <assert.h>
#include "function_pool.h"

// проверка наличия уже зарезервированого
// имени для предотвращения конфликта имен

bool CFunctionPool::m_is_name(const std::string& name)const
{
    return m_func_parser.is_name(name.begin(),name.end())||
            m_constant_parser.is_name(name.begin(),name.end()) ;
}

//зарегистрировать именованую константу

bool CFunctionPool::RegisterConstant(const std::string& name,real_t val)
{
    return m_constant_parser.add_constant(name,val);
}

//создать и зарегистрировать именованую функцию

parse_error_t CFunctionPool::RegisterFunction(const std::string& name,
                                    const std::vector<std::string>& vars,
                                str_citerator_t begin,str_citerator_t end)
{
    if(m_is_name(name)) return {parse_error_t::name_conflict_id,name};

    fp_expression_ptr_t new_exp(new fp_expression_t(convert_to_float,m_constant_parser,m_func_parser));
    auto pres=new_exp->parse(vars,begin,end);
    if(pres.is_valid())
    {
        m_functions_map.insert({name,new_exp.get()});
        m_func_parser.add_function(name,new_exp.get());
        m_reg_functions.push_back(std::move(new_exp));
    }
    return pres;
}

//создать безымянную функцию

std::pair<parse_error_t,const fp_expression_t*>
CFunctionPool::CreateFunction(const std::vector<std::string>& vars,
                                   str_citerator_t begin,
                                     str_citerator_t end)
{
    fp_expression_ptr_t new_exp(new fp_expression_t(convert_to_float,m_constant_parser,m_func_parser));
    auto pres=new_exp->parse(vars,begin,end);
    if(pres.is_valid())
    {
        m_unreg_functions.push_back(std::move(new_exp));
        return std::make_pair(pres,m_unreg_functions.back().get());
    }
    else
    {
        return std::make_pair(pres,nullptr);
    }
}

// получить именованую функцию

const fp_expression_t* CFunctionPool::GetFunction(const std::string& name)const
{
    auto iter=m_functions_map.find(name);
    return (iter==m_functions_map.end())? iter->second:nullptr;
}

void CFunctionPool::Clear()
{
    m_unreg_functions.clear();
    m_func_parser.clear(); m_constant_parser.clear();
    //порядок удаления m_reg_functions - позже созданные
    // и зависимые функции разрушаются раньше
    while(!m_reg_functions.empty()) m_reg_functions.pop_back();
    m_functions_map.clear();
}
