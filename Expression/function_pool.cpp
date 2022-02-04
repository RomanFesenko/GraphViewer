#include <assert.h>
#include <numbers>
#include <math.h>
//#include <iostream>

#include "function_pool.h"

namespace rg=std::ranges;

CFunctionPool::CFunction::CFunction(function_data_t*d,bool is_register):
m_data(d,[is_register](const function_data_t*data){if(data&&!is_register) delete data;}),
m_is_register(is_register)
{
}

CFunctionPool::CFunctionPool()
{
    // set builtin functions and constants
    using fptr_t=real_t(*)(real_t);
    auto add_function=[this](fptr_t ftr,const char*name)
    {
        function_data_t*fdata=new function_data_t;
        fdata->expr=ftr;
        fdata->name=name;
        fdata->is_buildin=true;
        fdata->node=m_dependency_graph.add_node(node_data_t(fdata));
        bool ins_res=m_Insert(m_buildin_functions,fdata);
        assert(ins_res);
    };

    auto add_constant=[this](real_t real,const char*name)
    {
        constant_data_t*cdata=new constant_data_t;
        cdata->name=name;
        cdata->value=real;
        cdata->is_buildin=true;
        cdata->node=m_dependency_graph.add_node(node_data_t(cdata));
        bool ins_res=m_Insert(m_buildin_constants,cdata);
        assert(ins_res);
    };
    add_function(std::sin,"sin");
    add_function(std::cos,"cos");
    add_function(std::tan,"tan");
    add_function(std::abs,"abs");
    add_function(std::exp,"exp");
    add_function(std::sqrt,"sqrt");
    add_function(std::asin,"asin");
    add_function(std::acos,"acos");
    add_function(std::log,"log");

    add_constant(std::numbers::pi_v<real_t>,"pi");
    add_constant(std::numbers::e_v<real_t>,"e");
}

expr::invokable_with_stack_t<CFunctionPool::real_t>* CFunctionPool::m_IdenMap(str_iterator_t b,str_iterator_t e)const
{
    if(auto*ptr=m_Find(m_buildin_constants,b,e))
    {
        m_nodes_cache.push_back(ptr->node);
        return new expr::constant_t(ptr->value);
    }
    if(auto*ptr=m_Find(m_constants,b,e))
    {
        m_nodes_cache.push_back(ptr->node);
        return new expr::constant_t(ptr->value);
    }
    if(auto*ptr=m_Find(m_buildin_functions,b,e))
    {
        m_nodes_cache.push_back(ptr->node);
        return new expr::function_ref_t(&ptr->expr);
    }
    if(auto*ptr=m_Find(m_functions,b,e))
    {
        m_nodes_cache.push_back(ptr->node);
        return new expr::function_ref_t(&ptr->expr);
    }
    return nullptr;
}

// CFunctionPool::CConstant

CFunctionPool::CConstant CFunctionPool::CreateConstant(const std::string&str,real_t real)
{
    if(IsIdentifier(str)) return nullptr;
    constant_data_t*cdata=new constant_data_t;
    cdata->name=str;
    cdata->is_buildin=false;
    cdata->value=real;
    cdata->node=m_dependency_graph.add_node(node_data_t(cdata));
    m_Insert(m_constants,cdata);
    return cdata;
}

CFunctionPool::CConstant CFunctionPool::FindConstant(str_citerator b,str_citerator e)const
{
    if(auto*cdata=m_Find(m_constants,b,e)) return cdata;
    return m_Find(m_buildin_constants,b,e);
}

bool CFunctionPool::EraseConstant(CConstant const_)
{
    return m_EraseData(m_constants,const_.m_data);
}

void CFunctionPool::DependentFunctions(CConstant parent,std::vector<CFunction>&funcs)const
{
    funcs.clear();
    if(!parent) return;
    m_dependency_graph.traverse_childs(parent.m_data->node,m_nodes_cache);
    for(auto desc:m_nodes_cache)
    {
        assert(desc.data().is_function);
        funcs.push_back(CFunction(desc.data().get<function_data_t>(),true));
    }
}

// CFunctionPool::CFunction

CFunctionPool::CFunction
CFunctionPool::CreateFunction(const std::vector<std::string>& args,
                              str_citerator begin,str_citerator end,
                              parse_error_t&error)const
{
    using namespace std::placeholders;
    auto fdata=std::make_unique<function_data_t>();
    m_nodes_cache.clear();
    error=fdata->expr.parse(args,begin,end,op_flag,std::bind(&CFunctionPool::m_IdenMap,this,_1,_2));
    if(error) return CFunction(nullptr,false);
    fdata->is_buildin=false;
    fdata->args=args;
    fdata->name="";
    fdata->body=std::string(begin,end);
    return CFunction(fdata.release(),false);
}

CFunctionPool::CFunction
CFunctionPool::CreateFunction(const std::vector<std::string>& vars,
                              str_citerator begin,str_citerator end)const
{
    parse_error_t error;
    return CreateFunction(vars,begin,end,error);
}

CFunctionPool::CFunction
CFunctionPool::CreateAndRegisterFunction(const std::string& name,
                                         const std::vector<std::string>& args,
                                         str_citerator begin,str_citerator end,
                                         parse_error_t&error)
{
    using namespace std::placeholders;
    auto fdata=std::make_unique<function_data_t>();
    m_nodes_cache.clear();
    error=fdata->expr.parse(args,begin,end,op_flag,std::bind(&CFunctionPool::m_IdenMap,this,_1,_2));
    if(error) return CFunction(nullptr,false);;

    fdata->is_buildin=false;
    fdata->args=args;
    fdata->name=name;
    fdata->body=std::string(begin,end);

    fdata->node=m_dependency_graph.add_node(node_data_t(fdata.get()));
    m_Insert(m_functions,fdata.get());
    for(auto node:m_nodes_cache)
    {
        m_dependency_graph.set_link(node,fdata->node);
    }
    return CFunction(fdata.release(),true);
}

CFunctionPool::CFunction
CFunctionPool::CreateAndRegisterFunction(const std::string& name,
                              const std::vector<std::string>& vars,
                              str_citerator begin,str_citerator end)
{
    parse_error_t error;
    return CreateAndRegisterFunction(name,vars,begin,end,error);
}


CFunctionPool::CFunction CFunctionPool::FindFunction(str_citerator b,str_citerator e)const
{
    if(auto*fdata=m_Find(m_functions,b,e)) return CFunction(fdata,true);
    return CFunction(m_Find(m_buildin_functions,b,e),true);
}

void CFunctionPool::DependentFunctions(CFunction parent,std::vector<CFunction>&funcs)const
{
    funcs.clear();
    if(!parent) return;
    m_dependency_graph.traverse_childs(parent.m_data->node,m_nodes_cache);
    for(auto desc:m_nodes_cache)
    {
        assert(desc.data().is_function);
        funcs.push_back(CFunction(desc.data().get<function_data_t>(),true));
    }
}

bool CFunctionPool::EraseFunction(CFunction func_)
{
    return m_EraseData(m_functions,func_.m_data.get());
}

expr::parse_error_t
CFunctionPool::ReparseFunction(CFunction func,const std::vector<std::string>& args,
                               str_citerator begin,str_citerator end)
{
    using namespace std::placeholders;
    assert(func);
    assert(args.size()==func.Arity());
    m_nodes_cache.clear();
    auto err=func.m_data->expr.parse(args,begin,end,op_flag,std::bind(&CFunctionPool::m_IdenMap,this,_1,_2));
    if(err) return err;
    m_dependency_graph.detach_parents(func.m_data->node);
    for(auto node:m_nodes_cache)
    {
        m_dependency_graph.set_link(node,func.m_data->node);
    }
    return err;
}

void CFunctionPool::Clear()
{
    std::vector<node_descriptor> for_delete;
    auto deleter=[this,&for_delete](node_descriptor node)
    {
        if(node.data().is_function)
        {
            auto*fdata=node.data().get<function_data_t>();
            if(fdata->is_buildin) return;
            //std::cout<<"delete function\n";
            delete fdata;
        }
        else
        {
            auto*cdata=node.data().get<constant_data_t>();
            if(cdata->is_buildin) return;
            //std::cout<<"delete constant\n";
            delete cdata;
        }
        for_delete.push_back(node);
    };
    m_dependency_graph.topological_sort(fn_output_iterator_t(deleter));
    assert(for_delete.size()==Functions()+Constants());
    for(auto node:for_delete)
    {
        m_dependency_graph.remove_node(node);
    }
    m_functions.clear();
    m_constants.clear();
}

bool CFunctionPool::IsIdentifier(str_citerator b,str_citerator e)const
{
    if(m_Find(m_constants,b,e))         return true;
    if(m_Find(m_buildin_constants,b,e)) return true;
    if(m_Find(m_functions,b,e))         return true;
    return m_Find(m_buildin_functions,b,e);
}

void CFunctionPool::TopologicalSortFunctions(std::vector<CFunction>&funcs)const
{
    funcs.clear();
    auto inserter=[this,&funcs](node_descriptor node)
    {
        if(node.data().is_function)
        {
            auto*fdata=node.data().get<function_data_t>();
            if(!fdata->is_buildin) funcs.push_back(CFunction(fdata,true));
        }
    };
    m_dependency_graph.topological_sort(fn_output_iterator_t(inserter));
}

CFunctionPool::~CFunctionPool()
{
    // delete registered functions and constants
    Clear();
    // delete buildin functions and constants
    m_dependency_graph.clear();
    for(auto*ptr:m_buildin_constants) delete ptr;
    for(auto*ptr:m_buildin_functions) delete ptr;
}









