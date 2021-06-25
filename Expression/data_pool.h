
#ifndef  _data_pool_
#define  _data_pool_

#include "expression_parser.h"
#include "dependency_graph.h"

//#include <iostream>

#include <memory>

using real_t=float;

enum /*class*/ dependency_t:int
{
    constant_id,
    function_id,
    external_id
};

class CConstantPool
{
    using str_citerator=std::string::const_iterator;
    public:
    class constant_t:public CNodeData
    {
        std::string m_name;
        public:
        real_t      m_value;
        private:
        int m_index=-1;
        friend class CConstantPool;
        public:
        constant_t(const std::string&str,real_t real):
        CNodeData(dependency_t::constant_id),
        m_name(str),m_value(real)
        {}
        const std::string&name()const{return m_name;}
        int index()const{return m_index;}
    };
    using constant_ptr_t=std::unique_ptr<constant_t>;
    private:
    struct comparer_t
    {
        using str_citerator=std::string::const_iterator;
        using ipair=std::pair<str_citerator,str_citerator>;
        bool operator()(const constant_ptr_t&_1,const std::string&_2_str)const
        {
            return _1->name()<_2_str;
        }
        bool operator()(const constant_ptr_t&_1,const ipair&pr)const
        {
            return std::lexicographical_compare(_1->name().begin(),_1->name().end(),
                                                pr.first,pr.second);
        }
    };
    std::vector<constant_ptr_t> m_constants; //ordered for quick search
    void m_ResetIndexes();
    public:
    CConstantPool(){}
    const constant_t* Insert(const std::string&str,real_t real);
    void Erase(const constant_t*);
    const constant_t& operator[](std::size_t i)const{return *m_constants[i].get();}
    constant_t& operator[](std::size_t i){return *m_constants[i].get();}
    std::size_t Size()const{return m_constants.size();}
    const constant_t* Find(str_citerator,str_citerator)const;
    template<class R>
    const constant_t* Find(const R&r)const
    {return Find(std::begin(r),std::end(r));}
    bool Empty()const{return m_constants.empty();}
};

class CDataPool;
// functions on floating point numbers
class CFunctionPool
{
    using str_citerator=std::string::const_iterator;
    public:
    using fp_expression_t=expression_t<real_t,arithmetics_fl&(~mod_id)>;
    class function_t:public CNodeData
    {
        std::string m_name;
        fp_expression_t* m_expression;
        std::vector<std::string> m_args;
        std::string m_body;
        int m_index=-1;
        void m_clear()
        {
            if(m_expression!=nullptr)
            {
                delete m_expression;
                m_expression=nullptr;
                m_DetachFromGraph();
            }
        }
        friend class CFunctionPool;
        friend class CDataPool;
        public:
        function_t(const std::string&name,
                   fp_expression_t*_exp,
                   const std::vector<std::string>&args,
                   str_citerator b,str_citerator e):
        CNodeData(dependency_t::function_id),
        m_name(name),m_expression(_exp),m_args(args),m_body(b,e)
        {}
        const std::string&name()const{return m_name;}
        const fp_expression_t&expression()const{return *m_expression;}
        const std::vector<std::string>&args()const{return m_args;}
        const std::string&body()const{return m_body;}
        int index()const{return m_index;}
        bool empty()const{return m_expression==nullptr;}
        ~function_t()
        {
            m_clear();
        }
    };
    using function_ptr_t=std::unique_ptr<function_t>;
    private:
    struct comparer_t
    {
        using str_citerator=std::string::const_iterator;
        using ipair=std::pair<str_citerator,str_citerator>;
        bool operator()(const function_ptr_t&_1,const std::string&_2_str)const
        {
            return _1->name()<_2_str;
        }
        bool operator()(const function_ptr_t&_1,const ipair&pr)const
        {
            return std::lexicographical_compare(_1->name().begin(),_1->name().end(),
                                                pr.first,pr.second);
        }
    };
    std::vector<function_ptr_t> m_functions; //ordered for quick search
    void m_ResetIndexes();
    public:
    CFunctionPool(){}
    const function_t* Insert(const std::string&name,
                      fp_expression_t*,
                      const std::vector<std::string>&,
                      str_citerator body_begin,str_citerator body_end);
    void Erase(const function_t*);
    std::size_t EraseMarked();
    std::size_t Size()const{return m_functions.size();}
    const function_t& operator[](std::size_t i)const{return *m_functions[i].get();}
    function_t& operator[](std::size_t i){return *m_functions[i].get();}
    const function_t* Find(str_citerator,str_citerator)const;
    template<class R>
    const function_t* Find(const R&r)const
    {return Find(std::begin(r),std::end(r));}
    //const function_t* Find(const fp_expression_t*)const;
    bool Empty()const{return m_functions.empty();}
};

class extern_expression_t;
class CDataPool
{
    using str_citerator=std::string::const_iterator;
    using function_t=CFunctionPool::function_t;
    using constant_t=CConstantPool::constant_t;
    using fp_expression_t=CFunctionPool::fp_expression_t;

    mutable CDependencyGraph m_graph;
    mutable std::vector<const function_t*> m_context;
    public:
    CConstantPool m_constant_pool;
    private:
    CFunctionPool m_function_pool;
    struct idetifier_parser_t
    {
        const CDataPool&m_pool;
        mutable std::function<bool(const CNodeData*)> m_dependency_tracker=nullptr;
        bool is_name(str_citerator begin,str_citerator end)const;
        const real_t* find_constant(str_citerator begin,str_citerator end)const;
        const CFunctionPool::fp_expression_t*find_function(str_citerator begin,str_citerator end)const;
    };
    struct number_parser_t
    {
        std::optional<std::pair<real_t,str_citerator>>
        operator()(str_citerator begin,str_citerator end)const
        {
            auto num_end=extract_number(begin,end);
            if(!num_end) return {};
            return std::make_pair(std::stof(std::string{begin,end}),*num_end);
        }

    };
    public:
    class wrapped_expression_t
    {
        fp_expression_t* m_exp=nullptr;
        parse_error_t m_error;
        int m_index=-1;
        wrapped_expression_t(fp_expression_t* p,int i=-1):
        m_exp(p),m_error(parse_error_t::success_id),m_index(i){}
        wrapped_expression_t(parse_error_t error):
        m_error(error){}
        friend class CDataPool;
        public:
        explicit operator bool()const
        {
            return m_error.is_valid();
        }
        const parse_error_t&error()const{return m_error;}
        template<class...args_t>
        real_t operator()(args_t...args)const
        {
            return (*m_exp)(args...);
        }
        int index()const{return m_index;}
    };
    private:
    number_parser_t m_number_parser;
    idetifier_parser_t m_identifier_parser;
    bool m_is_name(const std::string& name)const;
    public:
    CDataPool():
    m_identifier_parser(*this)
    {
    }
    wrapped_expression_t
    RegisterFunction(const std::string& name,
                     const std::vector<std::string>& vars,
                     str_citerator begin,str_citerator end);
    wrapped_expression_t
    RegisterFunction(const std::string& name,
                     const std::vector<std::string>& vars,
                     const std::string& body);

    parse_error_t ReparseFunction(function_t&,
                         const std::vector<std::string>& vars,
                         str_citerator begin,str_citerator end);
    parse_error_t ReparseFunction(function_t&,
                         const std::vector<std::string>& vars,
                         const std::string& body);

    const constant_t* RegisterConstant(const std::string& name,real_t real);
    const constant_t* RegisterConstant(const std::string& name,
                                 const std::string& val);

    extern_expression_t
    CreateFunction(const std::vector<std::string>& vars,
                   str_citerator begin,str_citerator end)const;

    extern_expression_t
    CreateFunction(const std::vector<std::string>& vars,
                   const std::string& body)const;
    const CFunctionPool& Functions()const{return m_function_pool;}
    CFunctionPool&Functions(){return m_function_pool;}
    CConstantPool&Constants(){return m_constant_pool;}
    const CConstantPool& Constants()const{return m_constant_pool;}
    const function_t*FindFunction(const std::string&name)const
    {return m_function_pool.Find(name);}
    const constant_t*FindConstant(const std::string&name)const
    {return m_constant_pool.Find(name);}
    void SetDependencyContext(const function_t*fn);
    void SetDependencyContext(const constant_t*ct);
    const CNodeData* TargetsRoot()const;
    const std::vector<const function_t*>& GetContext()const{return m_context;}
    const std::vector<const function_t*>& TopolocicalSortFuncs()const;
    bool IsExternalContext()const;
    bool DeleteDependencyContext();
    std::optional<real_t> ParseNumber(const std::string&str)const;
    function_ptr_t<real_t,const std::string&> GetNumberParser()const
    {
        return [](const std::string&s){return std::stof(s);};
    }
    void Clear();

    ~CDataPool(){ Clear();}
};

class extern_expression_t:CCopyableNodeData
{
    using fp_expression_t=CFunctionPool::fp_expression_t;
    std::shared_ptr<fp_expression_t> m_exp;
    parse_error_t m_error;

    public:
    extern_expression_t(fp_expression_t* p,
                        CDependencyGraph&,
                        const std::vector<const CNodeData*>&);
    extern_expression_t(parse_error_t pe);
    //extern_expression_t();
    explicit operator bool()const
    {
        return IsLinked()&&m_error.is_valid();
    }
    const parse_error_t&error()const{return m_error;}
    template<class...args_t>
    real_t operator()(args_t...args)const
    {
        return (*m_exp)(args...);
    }
    bool is_variable_used(int i)const
    {return m_exp->is_variable_used(i);}
    friend class CDataPool;
};

#endif

