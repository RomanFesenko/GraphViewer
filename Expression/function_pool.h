
#ifndef  _function_pool_
#define  _function_pool_

#include <memory>

#include "expression_parser.h"
#include "dependency_graph.h"

struct constant_t;
struct function_t;


class CFunctionPool
{
    using real_t=float;
    using str_citerator=std::string::const_iterator;
    class constant_data_t;
    class function_data_t;
    struct node_data_t
    {
        void* data;
        bool  is_function;
        node_data_t(constant_data_t*cdata):data(cdata),is_function(false){}
        node_data_t(function_data_t*fdata):data(fdata),is_function(true){}
        template<class T>
        T* get(){return reinterpret_cast<T*>(data);}
    };
    using function_t=expr::function<real_t>;
    using graph_t=dag<node_data_t>;
    using node_descriptor=graph_t::node_descriptor;
    using str_iterator_t=std::string::const_iterator;
    struct constant_data_t
    {
        constant_data_t(){}
        bool            is_buildin;
        real_t          value;
        std::string     name;
        node_descriptor node;

        int             index=-1;
    };
    struct function_data_t
    {
        function_data_t(){}
        bool                     is_buildin;
        function_t               expr;
        std::vector<std::string> args;
        std::string              name;
        std::string              body;
        node_descriptor          node;
        int                      index=-1;
    };
    struct comparer_t
    {
        using str_citerator=std::string::const_iterator;
        using ipair=std::pair<str_citerator,str_citerator>;
        template<class data_t>
        bool operator()(const data_t*data,const std::string&str)const
        {
            return data->name<str;
        }
        template<class data_t>
        bool operator()(const data_t*data,const ipair&str)const
        {
            return std::lexicographical_compare(data->name.begin(),data->name.end(),
                                                str.first,str.second);
        }
        template<class data_t>
        bool equal(const data_t*data,const std::string&str)const
        {
            return std::equal(data->name.begin(),data->name.end(),str.begin(),str.end());
        }
        template<class data_t>
        bool equal(const data_t*data,const ipair&str)const
        {
            return std::equal(data->name.begin(),data->name.end(),str.first,str.second);
        }
    };
    const unsigned op_flag=expr::float_arithmetics_fl;

    graph_t                       m_dependency_graph;
    std::vector<constant_data_t*> m_constants;
    std::vector<function_data_t*> m_functions;
    std::vector<constant_data_t*> m_buildin_constants;
    std::vector<function_data_t*> m_buildin_functions;
    mutable std::vector<node_descriptor>  m_nodes_cache;

    template<class vector_t>
    static bool m_Insert( vector_t&vector,typename vector_t::value_type data)
    {
        comparer_t comp;
        auto iter=std::lower_bound(vector.begin(),vector.end(),data->name,comp);
        if(iter!=vector.end()&&comp.equal(*iter,data->name)) return false;
        data->index=iter-vector.begin();
        iter=vector.insert(iter,data);
        for(++iter;iter!=vector.end();++iter)
        {
            ++(*iter)->index;
        }
        return true;
    }

    template<class vector_t>
    bool m_EraseData(vector_t&vector,typename vector_t::value_type data)
    {
        if(!data||data->is_buildin) return false;
        assert(vector[data->index]==data);
        vector[data->index]=nullptr;

        std::vector<node_descriptor> for_delete;
        m_dependency_graph.traverse_childs(data->node,for_delete);
        for(auto node:for_delete)
        {
            auto data=node.data();
            assert(data.is_function);
            vector[data.get<function_data_t>()->index]=nullptr;
            delete data.get<function_data_t>();
            m_dependency_graph.remove_node(node);
        }
        m_dependency_graph.remove_node(data->node);
        delete data;

        // erase marked datas
        vector.erase(std::remove(vector.begin(),vector.end(),nullptr),vector.end());
        for(decltype(vector.size()) i=0;i<vector.size();++i)
        {
            vector[i]->index=i;
        }
        return true;
    }

    template<class vector_t>
    static typename vector_t::value_type m_Find(vector_t&vector,str_iterator_t b,str_iterator_t e)
    {
        comparer_t comp;
        auto iter=std::lower_bound(vector.begin(),vector.end(),std::pair{b,e},comp);
        return (iter!=vector.end()&&comp.equal(*iter,std::pair{b,e}))? *iter:nullptr;
    }
    expr::invokable_with_stack_t<real_t>* m_IdenMap(str_iterator_t b,str_iterator_t e)const;

    public:
    using parse_error_t=expr::parse_error_t;
    class CConstant
    {
        constant_data_t* m_data=nullptr;
        CConstant(constant_data_t*d=nullptr):m_data(d){}
        public:
        CConstant(){}
        const std::string& Name()const{return m_data->name;}
        real_t             Value()const{return m_data->value;}
        void               SetValue(real_t r){m_data->value=r;}
        bool               IsBuildin()const{return m_data->is_buildin;}
        explicit operator bool()const{return m_data!=nullptr;}
        friend class CFunctionPool;
    };
    class CFunction
    {
        std::shared_ptr<function_data_t> m_data=nullptr;
        bool                             m_is_register=false;
        CFunction(function_data_t*d,bool);
        public:
        CFunction(){}
        const std::string& Name()const{return m_data->name;}
        bool               IsBuildin()const{return m_data->is_buildin;}
        const std::string& Body()const{return m_data->body;}
        const std::vector<std::string>& Args()const{return m_data->args;}
        auto               Arity()const{return m_data->args.size();}
        bool  IsRegister()const{return m_is_register;}
        explicit operator bool()const{return m_data!=nullptr;}
        template<class...args_t>
        real_t operator()(args_t...args)const
        {
            return (m_data->expr)(args...);
        }
        friend class CFunctionPool;
    };
    CFunctionPool();
    bool IsIdentifier(str_citerator begin,str_citerator end)const;
    bool IsIdentifier(const std::string&str)const{return IsIdentifier(str.begin(),str.end());}
    // Create function
    CFunction
    CreateFunction(const std::vector<std::string>& vars,
                   str_citerator begin,str_citerator end,
                   parse_error_t&error)const;

    CFunction
    CreateFunction(const std::vector<std::string>& vars,
                   str_citerator begin,str_citerator end)const;

    CFunction
    CreateFunction(const std::vector<std::string>& vars,const std::string&body,
                   parse_error_t&error)const
    {
        return CreateFunction(vars,body.begin(),body.end(),error);
    }

    CFunction
    CreateFunction(const std::vector<std::string>& vars,const std::string&body)const
    {
        return CreateFunction(vars,body.begin(),body.end());
    }
    // Create and register function
    CFunction
    CreateAndRegisterFunction(const std::string& name,
                              const std::vector<std::string>& vars,
                              str_citerator begin,str_citerator end,
                              parse_error_t&error);

    CFunction
    CreateAndRegisterFunction(const std::string& name,
                              const std::vector<std::string>& vars,
                              str_citerator begin,str_citerator end);

    CFunction
    CreateAndRegisterFunction(const std::string& name,
                              const std::vector<std::string>& vars,
                              const std::string&body,
                              parse_error_t&error)
    {
        return CreateAndRegisterFunction(name,vars,body.begin(),body.end(),error);
    }
    CFunction
    CreateAndRegisterFunction(const std::string& name,
                              const std::vector<std::string>& vars,const std::string&body)
    {
        return CreateAndRegisterFunction(name,vars,body.begin(),body.end());
    }
    // Reparse function
    parse_error_t ReparseFunction(CFunction,
                                  const std::vector<std::string>& vars,
                                  str_citerator begin,str_citerator end);
    parse_error_t ReparseFunction(CFunction f ,
                                  const std::vector<std::string>& vars,
                                  const std::string& body)
    {
        return ReparseFunction(f,vars,body.begin(),body.end());
    }
    // Find function
    CFunction FindFunction(str_citerator,str_citerator)const;
    CFunction FindFunction(const std::string&r)const{return FindFunction(std::begin(r),std::end(r));}
    void      DependentFunctions(CFunction parent,std::vector<CFunction>&dep)const;
    bool      EraseFunction(CFunction);
    auto      Functions()const{return m_functions.size();}
    CFunction Function(int i)const{return CFunction(m_functions[i],true);}

    auto      BuildinFunctions()const{return m_buildin_functions.size();}
    CFunction BuildinFunction(int i)const{return CFunction(m_buildin_functions[i],true);}
    void      TopologicalSortFunctions(std::vector<CFunction>&)const;
    //   Constants
    CConstant CreateConstant(const std::string&str,real_t real);
    CConstant FindConstant(str_citerator b,str_citerator e)const;
    CConstant FindConstant(const std::string&str)const{return FindConstant(str.begin(),str.end());}
    void      DependentFunctions(CConstant parent,std::vector<CFunction>&dep)const;
    bool      EraseConstant(CConstant);
    auto      Constants()const{return m_constants.size();}
    CConstant Constant(int i)const{return CConstant(m_constants[i]);}

    auto      BuildinConstants()const{return m_buildin_constants.size();}
    CConstant BuildinConstant(int i)const{return CConstant(m_buildin_constants[i]);}

    void Clear();
    ~CFunctionPool();
};

#endif

