#include <assert.h>
#include "data_pool.h"
//#include <iostream>

namespace rg=std::ranges;

///////////////////////////////////////////
///     CConstantPool
////////////////////////////////////////////

void CConstantPool::m_ResetIndexes()
{
    int i=0;
    for(auto&el:m_constants){el->m_index=i++;}
}

const CConstantPool::constant_t*
CConstantPool::Insert(const std::string&str,real_t real)
{
    auto iter=std::lower_bound(m_constants.begin(),
                              m_constants.end(),str,comparer_t{});
    if(iter!=m_constants.end()&&(*iter)->m_name==str) {return nullptr;}
    iter=m_constants.insert(iter,std::make_unique<constant_t>(str,real));
    m_ResetIndexes();
    return iter->get();
}

void CConstantPool::Erase(const constant_t*ct)
{
    m_constants.erase(m_constants.begin()+ct->m_index);
    m_ResetIndexes();
}


const CConstantPool::constant_t*
CConstantPool::Find(str_citerator b,str_citerator e)const
{
    auto iter=std::lower_bound(m_constants.begin(),m_constants.end(),
                              std::make_pair(b,e),comparer_t{});
    if(iter==m_constants.end()||
        !std::equal((*iter)->m_name.begin(),(*iter)->m_name.end(),b,e))
    {
        return nullptr;
    }
    return iter->get();
}

///////////////////////////////////////////
///     CFunctionPool
////////////////////////////////////////////

void CFunctionPool::m_ResetIndexes()
{
    int i=0;
    for(auto&el:m_functions){el->m_index=i++;}
}

const CFunctionPool::function_t*
CFunctionPool::Insert(const std::string&name,
                      fp_expression_t*expr,
                      const std::vector<std::string>&args,
                      str_citerator b,str_citerator e)
{
    auto iter=std::lower_bound(m_functions.begin(),m_functions.end(),
                              name,comparer_t{});
    if(iter!=m_functions.end()&&(*iter)->m_name==name) {return nullptr;}
    iter=m_functions.insert(iter,std::make_unique<function_t>(name,expr,args,b,e));
    m_ResetIndexes();
    return iter->get();
}

void CFunctionPool::Erase(const function_t*fn)
{
    m_functions.erase(fn->m_index+m_functions.begin());
    m_ResetIndexes();
}

std::size_t CFunctionPool::EraseMarked()
{
    std::size_t was=Size();
    std::erase_if(m_functions,[](auto&ptr){return ptr->empty();});
    return was-Size();
}

const CFunctionPool::function_t*
CFunctionPool::Find(str_citerator b,str_citerator e)const
{
    auto iter=std::lower_bound(m_functions.begin(),m_functions.end(),
              std::make_pair(b,e),comparer_t{});
    if(iter==m_functions.end()||
        !std::equal((*iter)->m_name.begin(),(*iter)->m_name.end(),b,e))
    {
        return nullptr;
    }
    return iter->get();
}
/*
const CFunctionPool::function_t*
CFunctionPool::Find(const fp_expression_t*exp)const
{
    auto iter=rg::find(m_functions,exp,[](auto&ptr){return ptr->m_expression;});
    return (iter!=m_functions.end())? iter->get():nullptr;
}
*/

///////////////////////////////////////////
///     CDataPool::idetifier_parser_t
////////////////////////////////////////////

bool CDataPool::idetifier_parser_t::
is_name(str_citerator begin,str_citerator end)const
{
    return m_pool.m_function_pool.Find(begin,end)!=nullptr||
           m_pool.m_constant_pool.Find(begin,end)!=nullptr;
}

const real_t* CDataPool::idetifier_parser_t::
find_constant(str_citerator begin,str_citerator end)const
{
    const auto* const_ptr=m_pool.m_constant_pool.Find(begin,end);
    if(const_ptr&&m_dependency_tracker(const_ptr))
    {
        return &const_ptr->m_value;
    }
    return nullptr;
}

const CFunctionPool::fp_expression_t* CDataPool::idetifier_parser_t::
find_function(str_citerator begin,str_citerator end)const
{
    const function_t* func_ptr=m_pool.m_function_pool.Find(begin,end);
    if(func_ptr)
    {
        m_dependency_tracker(func_ptr);
        return &func_ptr->expression();
    }
    return nullptr;
}

///////////////////////////////////////////
///     CDataPool
////////////////////////////////////////////

bool CDataPool::m_is_name(const std::string& name)const
{
    return m_identifier_parser.is_name(name.begin(),name.end())||
           fp_expression_t::m_builtin_constant.find(name)!=
           fp_expression_t::m_builtin_constant.end()||
           fp_expression_t::m_builtin_function.find(name)!=
           fp_expression_t::m_builtin_function.end();
}

// Create a function with control dependencies

CDataPool::wrapped_expression_t
CDataPool::RegisterFunction(const std::string& name,
                            const std::vector<std::string>& vars,
                            str_citerator begin,str_citerator end)
{
    if(m_is_name(name)) return  parse_error_t(parse_error_t::name_conflict_id,name);

    std::vector<const CNodeData*>parents;
    auto dep_tracker=[&parents](const CNodeData*data)
    {
        parents.push_back(data);
        return true;
    };
    m_identifier_parser.m_dependency_tracker=dep_tracker;
    auto new_exp=std::make_unique<fp_expression_t>();
    auto pr=new_exp->parse(vars,begin,end,m_identifier_parser,m_number_parser);
    if(!pr.is_valid())
    {
        return pr;
    }
    const function_t*f_ptr=m_function_pool.Insert(name,new_exp.get(),vars,begin,end);
    assert(f_ptr);
    //set dependencies
    rg::sort(parents);
    auto [first,end_erase]=rg::unique(parents);
    parents.erase(first,end_erase);
    m_graph.AddNode(f_ptr);
    for(auto* ptr:parents)
    {
        f_ptr->AddParent(ptr);
    }
    return {new_exp.release(),f_ptr->index()};
}

CDataPool::wrapped_expression_t
CDataPool::RegisterFunction(const std::string& name,
                            const std::vector<std::string>& vars,
                            const std::string& body)
{
    return RegisterFunction(name,vars,body.begin(),body.end());
}



parse_error_t
CDataPool::ReparseFunction(function_t&func,
                           const std::vector<std::string>& vars,
                           str_citerator begin,str_citerator end)
{
    m_graph.AllChilds(&func);
    std::vector<const CNodeData*>parents;
    const function_t* cycle=nullptr;
    auto dep_tracker=[&parents,this,&cycle](const CNodeData*ptr)
    {
        if(m_graph.IsInTargets(ptr))//may be cyclical dependencies
        {
            assert(ptr->m_type==dependency_t::function_id);
            cycle=static_cast<const function_t*>(ptr);
            return false;
        }
        parents.push_back(ptr);
        return true;
    };
    m_identifier_parser.m_dependency_tracker=dep_tracker;
    auto pr=func.m_expression->parse(vars,begin,end,
                                     m_identifier_parser,
                                     m_number_parser);
    if(!pr.is_valid())
    {
        return (cycle)?
                parse_error_t(parse_error_t::cyclical_dependence_id,cycle->name())
                :pr;
    }
    func.m_args=vars;
    func.m_body=std::string(begin,end);
    //set new dependencies
    func.DetachParents();
    rg::sort(parents);
    auto [first,end_erase]=rg::unique(parents);
    parents.erase(first,end_erase);
    for(auto* ptr:parents){func.AddParent(ptr);}
    return pr;
}


parse_error_t
CDataPool::ReparseFunction(function_t&f,
                         const std::vector<std::string>& vars,
                         const std::string& body)
{
    return ReparseFunction(f,vars,body.begin(),body.end());
}

const CDataPool::constant_t*
CDataPool::RegisterConstant(const std::string& name,real_t real)
{
    if(m_is_name(name)) return nullptr;
    auto ptr=m_constant_pool.Insert(name,real);
    if(ptr==nullptr) return nullptr;
    m_graph.AddNode(ptr);
    return ptr;
}

const CDataPool::constant_t*
CDataPool::RegisterConstant(const std::string& name,
                                 const std::string&str_val)
{
    if(m_is_name(name)) return nullptr;
    auto val=m_number_parser(str_val.begin(),str_val.end());
    return RegisterConstant(name,val->first);
}


std::optional<real_t> CDataPool::ParseNumber(const std::string&str)const
{
    auto val=m_number_parser(str.begin(),str.end());
    if(!val) return {};
    return val->first;
}

bool CDataPool::IsExternalContext()const
{
    return rg::find(m_context,nullptr)!=m_context.end();
}

//Find all functions dependent on the given function

void CDataPool::SetDependencyContext(const function_t*fn)
{
    m_graph.AllChilds(fn);
    m_context.clear();
    for(auto t:m_graph.Targets())
    {
        m_context.push_back(static_cast<const function_t*>(t));
    }
    if(!m_context.empty()) m_context.pop_back();
}

//Find all functions dependent on the given constant

void CDataPool::SetDependencyContext(const constant_t*ct)
{
    m_graph.AllChilds(ct);
    m_context.clear();
    for(int i=0;i<m_graph.Targets().size()-1;i++)
    {
        m_context.push_back(static_cast<const function_t*>(m_graph.Targets()[i]));
    }
}

const CNodeData* CDataPool::TargetsRoot()const
{
    if(m_graph.Targets().empty()) return nullptr;
    const CBaseNodeData*data=m_graph.Targets().back();
    assert(data&&data->m_type!=dependency_t::external_id);
    return static_cast<const CNodeData*>(data);
}

//Remove function or constant with all dependencies

bool CDataPool::DeleteDependencyContext()
{
    const auto&targets=m_graph.Targets();
    if(IsExternalContext()) return false;
    for(auto fn:m_context)
    {
        assert(fn);
        const_cast<function_t*>(fn)->m_clear();
    }
    auto erased=m_function_pool.EraseMarked();
    assert(erased==targets.size()-1);

    // delete root
    const CNodeData* some=TargetsRoot();
    if(!some) return true;
    if(some->m_type==dependency_t::function_id)
    {
        m_function_pool.Erase(static_cast<const function_t*>(some));
    }
    else
    {
        m_constant_pool.Erase(static_cast<const constant_t*>(some));
    }
    m_context.clear();
    return true;
}

void CDataPool::Clear()
{
    m_graph.TopologicalSort();
    for(auto target:m_graph.Targets())
    {
        assert(target);
        if(auto t=target->m_type;t==dependency_t::function_id)
        {
            //std::cout<<"delete function:"<<some->cast<function_t>()->name()<<'\n';
            m_function_pool.Erase(static_cast<const function_t*>(target));
        }
        else if(t==dependency_t::constant_id)
        {
            //std::cout<<"delete constant:"<<some->cast<constant_t>()->name()<<'\n';
            m_constant_pool.Erase(static_cast<const constant_t*>(target));
        }
        else// not deleted external function
        {
            assert(false);
        }
    }
    m_context.clear();
    assert(m_graph.Empty());
    assert(m_function_pool.Empty());
    assert(m_constant_pool.Empty());
}

//TopolocicalSortFuncs - m_context[0] is leaf of
//dependencies tree

const std::vector<const CDataPool::function_t*>&
CDataPool::TopolocicalSortFuncs()const
{
  m_context.clear();
  m_graph.TopologicalSort();
  for(const auto*target:m_graph.Targets())
  {
      if(!target) continue;// external expression
      if(target->m_type==dependency_t::constant_id) continue;
      assert(target->m_type==dependency_t::function_id);
      m_context.push_back(static_cast<const function_t*>(target));
  }
  return m_context;
}

//create unnamed function as leaf of dependencies tree

extern_expression_t
CDataPool::CreateFunction(const std::vector<std::string>& vars,
                          str_citerator begin,str_citerator end)const
{
    std::vector<const CNodeData*>parents;
    auto dep_tracker=[&parents](const CNodeData*_void)
    {
        parents.push_back(_void);
        return true;
    };
    m_identifier_parser.m_dependency_tracker=dep_tracker;
    auto new_exp=std::make_unique<fp_expression_t>();
    auto pr=new_exp->parse(vars,begin,end,m_identifier_parser,m_number_parser);
    if(!pr.is_valid())
    {
        return extern_expression_t(pr);
    }
    return extern_expression_t(new_exp.release(),m_graph,parents);
}

extern_expression_t
CDataPool::CreateFunction(const std::vector<std::string>& vars,
                          const std::string&body) const
{
    return CreateFunction(vars,body.begin(),body.end());
}


// extern_expression_t

/*extern_expression_t::extern_expression_t():
CNodeData(dependency_t::external_id)
{}*/

extern_expression_t::extern_expression_t(parse_error_t pe):
CCopyableNodeData(dependency_t::external_id),
m_error(pe)
{

}

extern_expression_t::extern_expression_t(fp_expression_t* p,
                                         CDependencyGraph&graph,
                                         const std::vector<const CNodeData*>& deps):
CCopyableNodeData(dependency_t::external_id),
m_error(parse_error_t::success_id)
{
    m_exp.reset(p);
    graph.AddNode(this);
    for(auto parent:deps)
    {
        assert(parent->m_type!=dependency_t::external_id);
        AddParent(parent);
    }
}
