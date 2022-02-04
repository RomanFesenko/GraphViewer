
#ifndef  _expression_parser_
#define  _expression_parser_

#include <vector>
#include <map>
#include <optional>
#include <string>
#include <functional>
#include <iostream>
#include <algorithm>
#include <numbers>
#include <type_traits>
#include <charconv>


#include <assert.h>

#include "reversed_sequence.h"
#include "string_util.h"

namespace expr{

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
    error_t type()const{return m_type;}
    explicit operator bool()const{return m_type!=success_id;}
    const std::string what()const
    {
        static const char* what_[]=
        {
            "success",
            "lexical error",
            "unknown identifier",
            "number error",
            "parenthesis error",
            "syntax error",
            "name conflict",
            "cyclical dependence",
            "unknown error"
        };
        return what_[m_type];
    }
    std::string detail()const
    {
        return m_detail.empty()? what():what()+(":"+m_detail);
    }
};

namespace detail
{

template<class funct_t,class T>
constexpr std::size_t function_arity()
{
    if constexpr(std::is_invocable_r_v<T,funct_t,T>)
    {
        return 1;
    }
    else if constexpr(std::is_invocable_r_v<T,funct_t,T,T>)
    {
        return 2;
    }
    else if constexpr(std::is_invocable_r_v<T,funct_t,T,T,T>)
    {
        return 3;
    }
    else
    {
        return 0;
    }
}

}

template<class T>
class invokable_with_stack_t
{
    public:
    using value_type=T;
    enum type_t
    {
        constant_id,
        variable_id,
        operation_id,
        open_par_id,
        close_par_id,
        separator_id,
        function_id
    };
    private:
    const type_t m_type;
    protected:
    int          m_stack_inc;
    public:
    invokable_with_stack_t(type_t t,int inc=0):m_type(t),m_stack_inc(inc){}
    type_t  type()const{return m_type;}
    virtual void call_stack(std::vector<T>&)const{assert(false);};
    virtual invokable_with_stack_t* clone()const{assert(false);return nullptr;};
    int stack_increment()const{return  m_stack_inc;}
    virtual ~invokable_with_stack_t(){}
};

template<class T>
class constant_t:public invokable_with_stack_t<T>
{
    const T m_const;
    public:
    explicit constant_t(T value):
    invokable_with_stack_t<T>(invokable_with_stack_t<T>::constant_id,1),m_const(value)
    {}
    virtual void call_stack(std::vector<T>&stack)const override
    {
        stack.push_back(m_const);
    }
    virtual invokable_with_stack_t<T>* clone()const override
    {
        return new constant_t(m_const);
    };
};

template<class T>
struct variable_t:public invokable_with_stack_t<T>
{
    using source_type=T*;
    const T* m_var_ptr;
    explicit variable_t(const T* value_ptr):
    invokable_with_stack_t<T>(invokable_with_stack_t<T>::variable_id,1),m_var_ptr(value_ptr)
    {}
    virtual void call_stack(std::vector<T>&stack)const override
    {
        stack.push_back(*m_var_ptr);
    }
    virtual invokable_with_stack_t<T>* clone()const override
    {
        return new variable_t(m_var_ptr);
    };
};


template<class functor_t,class T,std::size_t arity>
class function_t:public invokable_with_stack_t<T>
{
    functor_t m_functor;
    reversed_sequences<arity-1> m_sequences;
    // the value at the top of the call stack becomes
    // the last argument of the function call
    template<std::size_t...ints>
    T m_call_stack_impl(std::index_sequence<ints...> seq,const std::vector<T>&stack)const
    {
        auto _back=stack.cend()-1;
        return m_functor((*(_back-ints))...);
    }
    public:
    using source_type=functor_t;
    function_t(functor_t functor):
    invokable_with_stack_t<T>(invokable_with_stack_t<T>::function_id,1-static_cast<int>(arity)),
    m_functor(functor)
    {}
    virtual void call_stack(std::vector<T>&stack)const override
    {
        assert(!stack.empty());
        T result=m_call_stack_impl(m_sequences,stack);
        if constexpr(arity>1)
        {
            stack.erase(stack.end()-arity+1,stack.end());
        }
        stack.back()=result;
    }
    virtual invokable_with_stack_t<T>* clone()const override
    {
        return new function_t(m_functor);
    };
};

template<class T>
class function_ref_t:public invokable_with_stack_t<T>
{
    invokable_with_stack_t<T>*m_ref;
    public:
    using source_type=invokable_with_stack_t<T>*;
    function_ref_t(invokable_with_stack_t<T>* ref):
    invokable_with_stack_t<T>(invokable_with_stack_t<T>::function_id,ref->stack_increment()),m_ref(ref)
    {}
    virtual void call_stack(std::vector<T>&stack)const override
    {
        m_ref->call_stack(stack);
    }
    virtual invokable_with_stack_t<T>* clone()const override
    {
        return new function_ref_t(m_ref);
    };
};

template<class T>
class base_operation_t:public invokable_with_stack_t<T>
{
    int m_priority;
    public:
    explicit base_operation_t(int pr):
    invokable_with_stack_t<T>(invokable_with_stack_t<T>::operation_id,-1),
    m_priority(pr)
    {
    }
    int priority()const{return m_priority;}
};

template<class functor_t,class T>
class operation_t:public base_operation_t<T>
{
    functor_t m_functor;//std::plus,std::minus,....
    int m_priority;
    public:
    using source_type=functor_t;
    explicit operation_t(functor_t functor,int priority):
    base_operation_t<T>(priority),m_functor(functor)
    {
    }
    virtual void call_stack(std::vector<T>&stack)const override
    {
        T result=m_functor(*(stack.end()-2),stack.back());
        stack.pop_back();
        stack.back()=result;
    }
    virtual invokable_with_stack_t<T>* clone()const override
    {
        return new operation_t(m_functor,m_priority);
    };
};


template<class T>
int max_stack_depth(const std::vector<invokable_with_stack_t<T>*>& postfix)
{
    int in_stack=0;
    int max_depth=0;
    for(const auto& tok_ptr:postfix)
    {
        in_stack+=tok_ptr->stack_increment();
        if(in_stack<=0) return -1;
        if(in_stack>max_depth){max_depth=in_stack;}
    }
    return (in_stack==1)? max_depth:-1;
}

template<class T>
void evaluate_postfix(const std::vector<invokable_with_stack_t<T>*>& postfix,
                      std::vector<T>& stack)
{
    for(const auto& tok_ptr:postfix)
    {
        tok_ptr->call_stack(stack);
    }
}

template<class T>
bool check_parenthesis(const std::vector<invokable_with_stack_t<T>*>& infix)
{
    using invokable_t=invokable_with_stack_t<T>;
    int delta=0;
    for(const auto& token:infix)
    {
        if(token->type()==invokable_t::open_par_id)
        {
            ++delta;
        }
        else if(token->type()==invokable_t::close_par_id)
        {
            --delta;
        }
    }
    return delta==0;
}

// infix_to_postfix - converting an expression from an infix
// to postfix form using alg Dijkstra (priority stack)

template<class T>
void infix_to_postfix(std::vector<invokable_with_stack_t<T>*>& infix)
{
    using invoke_t=invokable_with_stack_t<T>;
    using iterator=std::vector<invokable_with_stack_t<T>*>::iterator;
    std::vector<invokable_with_stack_t<T>*> aux_stack;
    iterator postfix_end=infix.begin();
    auto postfix_push_back=[&postfix_end](invoke_t*inv)
    {
        *postfix_end=inv;
        ++postfix_end;
    };

    int priority;
    for(const auto&  inf_tok:infix)
    {
        switch(inf_tok->type())
        {
            case invoke_t::constant_id:
            case invoke_t::variable_id:
            postfix_push_back(inf_tok);
            break;

            case invoke_t::operation_id:
            priority=static_cast<const base_operation_t<T>*>(inf_tok)->priority();
            while(!aux_stack.empty())
            {
                auto back=aux_stack.back();
                if(back->type()==invoke_t::operation_id)
                {
                    if(static_cast<base_operation_t<T>*>(back)->priority()>=priority)
                    {
                        postfix_push_back(back);
                        aux_stack.pop_back();
                    }
                    else break;
                }
                else if(back->type()==invoke_t::function_id)
                {
                    postfix_push_back(back);
                    aux_stack.pop_back();
                }
                else break;
            }
            aux_stack.push_back(inf_tok);
            break;

            case invoke_t::open_par_id:
            case invoke_t::function_id:
            aux_stack.push_back(inf_tok);
            break;

            case invoke_t::close_par_id:
            while(true)
            {
                assert(!aux_stack.empty());
                if(aux_stack.back()->type()!=invoke_t::open_par_id)
                {
                    postfix_push_back(aux_stack.back());
                    aux_stack.pop_back();
                }
                else
                {
                    delete aux_stack.back();
                    aux_stack.pop_back();
                    break;
                }
            }
            delete inf_tok;
            break;

            case invoke_t::separator_id:
            while(true)
            {
                assert(!aux_stack.empty());
                if(aux_stack.back()->type()!=invoke_t::open_par_id)
                {
                    postfix_push_back(aux_stack.back());
                    aux_stack.pop_back();
                }
                else
                {
                    break;
                }
            }
            delete inf_tok;
            break;

            default:assert(false);
        }

    }
    infix.erase(postfix_end,infix.end());
    std::reverse(aux_stack.begin(),aux_stack.end());
    infix.insert(infix.end(),aux_stack.begin(),aux_stack.end());
}

enum operation_type:unsigned int
{
    //arithmetic
    plus_id=1,
    minus_id=1<<1,
    mul_id=1<<2,
    div_id=1<<3,
    mod_id=1<<4,
    neg_id=1<<5,
    //comparisons
    equal_id=1<<6,
    not_equal_id=1<<7,
    greater_id=1<<8,
    less_id=1<<9,
    greater_equal_id=1<<10,
    less_equal_id=1<<11,
    //boolean operations
    log_and_id=1<<12,
    log_or_id=1<<13,
    log_not_id=1<<14,
    //bitwise operations
    bit_and_id=1<<15,
    bit_or_id=1<<16,
    bit_not_id=1<<17,
};

// only binary operations



const unsigned int float_arithmetics_fl=plus_id|minus_id|mul_id|div_id;



const unsigned int compare_fl=   equal_id|
                             not_equal_id|
                               greater_id|
                                  less_id|
                         greater_equal_id|
                            less_equal_id;

const unsigned int boolean_fl= log_and_id|
                                log_or_id;

const unsigned int bits_fl=    bit_and_id|
                                bit_or_id;


/////////////////////////////////////////////////
///                   Tokenization
/////////////////////////////////////////////////

template<class T>
auto make_constants_parser(const std::vector<std::string>&idens,const std::vector<T>&sources)
{
    using iterator=std::string::const_iterator;
    std::map<std::string,T,sutil::string_comparer_t> m_map;
    assert(idens.size()==std::size(sources));
    for(decltype(idens.size()) i=0;i<idens.size();++i)
    {
        m_map.insert({idens[i],sources[i]});
    }
    return [map=std::move(m_map)](iterator b,iterator e)
    {
        auto value=map.find(std::pair{b,e});
        return (value!=map.end())? new constant_t<T>(value->second):nullptr;
    };
}

template<class func_t,class value_t>
auto make_functions_parser(const std::vector<std::string>&idens,const std::vector<func_t>&sources)
{
    using iterator=std::string::const_iterator;
    std::map<std::string,func_t,sutil::string_comparer_t> m_map;
    assert(idens.size()==std::size(sources));
    for(decltype(idens.size()) i=0;i<idens.size();++i)
    {
        m_map.insert({idens[i],sources[i]});
    }
    return [map=std::move(m_map)](iterator b,iterator e)->invokable_with_stack_t<value_t>*
    {
        auto value=map.find(std::pair{b,e});
        if(value!=map.end())
        {
            return new function_t<func_t,value_t,detail::function_arity<func_t,value_t>()>(value->second);
        }
        else
        {
            return nullptr;
        }
    };
}

template<class T>
struct default_number_parser
{
    using iterator=std::string::const_iterator;
    std::optional<std::pair<T,iterator>> operator()(iterator b,iterator e) const
    {
        T value;
        auto [ptr, ec]{std::from_chars(&*b,&*e,value)};
        if(ec==std::errc())
        {
            return std::pair{value, b+(ptr-&*b)};
        }
        else
        {
            return {};
        }
    }
};

/* tokenize - converting the string [str_beg, str_end) into a set of tokens.
  identifier_parser_t - by the found identifier
  returns a token (function, constant, or variable).
  number_parser_t - returns by the found number
  a constant of type var_t
*/
template<class var_t,
         class identifier_parser_t,//->invokable_with_stack_t<var_t>*
         class number_parser_t>    //->std::optional<std::pair<var_t,str_iterator_t>>
parse_error_t
parse_expression(typename std::string::const_iterator str_beg,typename std::string::const_iterator str_end,
                 unsigned            op_flag,
                 identifier_parser_t iden_parser,
                 number_parser_t     num_parser,
                 std::vector<invokable_with_stack_t<var_t>*>&infix)
{
    using namespace sutil;
    using str_iterator_t=std::string::const_iterator;
    using invoke_t=invokable_with_stack_t<var_t>;
    infix.clear();
    auto result=[&infix](parse_error_t err)
    {
        if(err) for(auto*tok:infix) delete tok;
        return err;
    };
    for(auto caret=str_beg;caret<str_end;)
    {
        if(is_ignore(*caret))
        {
            ++caret;
        }
        else if(*caret=='+'&&(op_flag|plus_id))
        {
            if(caret==str_beg||*(caret-1)=='('||*(caret-1)==',')
            {
                infix.push_back(new constant_t<var_t>(0));
            }
            infix.push_back(new operation_t<std::plus<var_t>,var_t>(std::plus<var_t>(),1));
            ++caret;
        }
        else if(*caret=='-'&&(op_flag|minus_id))
        {
            if(caret==str_beg||*(caret-1)=='('||*(caret-1)==',')
            {
                infix.push_back(new constant_t<var_t>(0));
            }
            infix.push_back(new operation_t<std::minus<var_t>,var_t>(std::minus<var_t>(),1));
            ++caret;
        }
        else if(*caret=='*'&&(op_flag|mul_id))
        {
            infix.push_back(new operation_t<std::multiplies<var_t>,var_t>(std::multiplies<var_t>(),2));
            ++caret;
        }
        else if(*caret=='/'&&(op_flag|div_id))
        {
            infix.push_back(new operation_t<std::divides<var_t>,var_t>(std::divides<var_t>(),2));
            ++caret;
        }
        else if(*caret=='(')
        {
            infix.push_back(new invoke_t(invoke_t::open_par_id));
            ++caret;
        }
        else if(*caret==')')
        {
            infix.push_back(new invoke_t(invoke_t::close_par_id));
            ++caret;
        }
        else if(*caret==',')
        {
            infix.push_back(new invoke_t(invoke_t::separator_id));
            ++caret;
        }
        else if(is_iden_begin(*caret))
        {
            str_iterator_t end_iden=std::find_if(caret,str_end,is_iden_end);
            invoke_t* tok=iden_parser(caret,end_iden);
            if(tok==nullptr)
            {
                return result({parse_error_t::unknown_identifier_id,std::string(caret,end_iden)});
            }
            infix.push_back(tok);
            caret=end_iden;
        }
        else if(is_number_char(*caret))
        {
            std::optional<std::pair<var_t,str_iterator_t>> pair=num_parser(caret,str_end);
            if(!pair)
            {
                return result({parse_error_t::number_error_id,std::string(caret,caret+1)});
            }
            infix.push_back(new constant_t<var_t>(pair->first));
            caret=pair->second;
        }
        else
        {
            return result({parse_error_t::unknown_identifier_id,std::string(caret,caret+1)});
        }
    }
    if(!check_parenthesis(infix))
    {
        return result(parse_error_t::parenthesis_error_id);
    }
    infix_to_postfix(infix);
    if(max_stack_depth(infix)==-1)
    {
        return result(parse_error_t::syntax_error_id);
    }
    return result(parse_error_t::success_id);
}


template<class header_t,class...tail_t>
constexpr auto concat_parsers(header_t header,tail_t...tail)
{
    using iterator=std::string::const_iterator;
    if constexpr(sizeof...(tail_t)==0)
    {
        return header;
    }
    else
    {
        /*constexpr*/ auto concat_tail=concat_parsers(tail...);
        return [header,concat_tail](iterator b,iterator e)
        {
            auto* invokable=header(b,e);
            return (invokable!=nullptr)? invokable:concat_tail(b,e);
        };
    }
}


///function

template<class T>
class function:public invokable_with_stack_t<T>
{
    using str_citerator=std::string::const_iterator;
    using self_t=function<T>;

    std::vector<invokable_with_stack_t<T>*> m_postfix;
    mutable std::vector<T>                  m_args;
    mutable std::vector<T>                  m_call_stack;
    constexpr static  auto                  m_default_identifier_parser=[](str_citerator,str_citerator){return nullptr;};

    void m_clear()
    {
        for(auto*ptr:m_postfix) delete ptr;
        m_args.clear();
        m_call_stack.clear();
    }
    void m_copy(const function&other)
    {
        m_args=other.m_args;
        m_call_stack=other.m_call_stack;
        for(auto*token:other.m_postfix)
        {
            m_postfix.push_back(token->clone());
            if(token->type()==invokable_with_stack_t<T>::variable_id)
            {
                std::ptrdiff_t i=static_cast<variable_t<T>*>(token).m_var_ptr-&other.m_args[0];
                assert(i>=0&&i<m_args.size());
                static_cast<variable_t<T>*>(m_postfix.back()).m_var_ptr=&m_args[i];
            }
        }
        this->m_stack_inc=other.m_stack_inc;
    }
    template<class functor_t>
    void m_copy(const functor_t&other)
    {
        constexpr auto arity=detail::function_arity<functor_t,T>();
        static_assert(arity);
        m_args.resize(arity);
        for(std::size_t i=0;i<arity;++i)
        {
            m_postfix.push_back(new variable_t(&m_args[i]));
        }
        m_postfix.push_back(new function_t<functor_t,T,arity>(other));
        m_call_stack.reserve(m_args.size());
        this->m_stack_inc=1-arity;
    }
    public:
    function():invokable_with_stack_t<T>(invokable_with_stack_t<T>::function_id,0){}
    function(const function&other)
    {
        m_copy(other);
    }
    function& operator=(const function&other)
    {
        if(this==&other) return *this;
        m_clear();
        m_copy(other);
        return *this;
    }
    function(function&&other)=default;
    function& operator=(function&&other)
    {
        if(this==&other) return *this;
        m_clear();
        m_postfix=std::move(other.m_postfix);
        m_args=std::move(other.m_args);
        m_call_stack=std::move(other.m_call_stack);
        return *this;
    }
    template<class functor_t>
    function(const functor_t&other)
    {
        m_copy(other);
    }
    template<class functor_t>
    function& operator=(const functor_t&other)
    {
        m_clear();
        m_copy(other);
        return *this;
    }

    virtual void call_stack(std::vector<T>&stack)const override
    {
        auto copy_beg=stack.end()-m_args.size();
        std::copy(copy_beg,stack.end(),m_args.begin());
        stack.erase(copy_beg,stack.end());
        for(const auto invokable:m_postfix)
        {
            invokable->call_stack(stack);
        }
    }
    template<class...args_t>
    T operator()(args_t...args)const
    {
        assert(arity()==sizeof...(args_t));
        m_args={(args)...};
        m_call_stack.clear();
        //m_call_stack.reserve(m_call_stack_depth);
        for(const auto invokable:m_postfix)
        {
            invokable->call_stack(m_call_stack);
        }
        assert(m_call_stack.size()==1);
        return m_call_stack[0];
    }
    //parsing WITH saving the current state
    //in case of parse failure
    template<class iden_parser_t,   //->invokable_with_stack_t<var_t>*
             class number_parser_t=default_number_parser<T>> //->std::optional<std::pair<var_t,str_iterator_t>>
    parse_error_t parse(const std::vector<std::string>& vars,
                        str_citerator begin,str_citerator end,
                        unsigned            op_flag,// op_flag - supported operations
                        iden_parser_t iden_parser=m_default_identifier_parser,
                        number_parser_t number_parser=default_number_parser<T>{})
    {
        std::vector<invokable_with_stack_t<T>*> postfix;
        std::vector<T>                  args(vars.size());
        auto args_parser=[&](str_citerator b,str_citerator e)->invokable_with_stack_t<T>*
        {
            for(decltype(vars.size()) i=0;i<vars.size();++i)
            {
                if(std::equal(b,e,vars[i].begin(),vars[i].end()))
                {
                    return new variable_t(&args[i]);
                }
            }
            return nullptr;
        };
        auto error=parse_expression(begin,end,op_flag,concat_parsers(args_parser,iden_parser),number_parser,postfix);
        if(error)
        {
            return error;
        }
        m_clear();

        m_postfix=std::move(postfix);
        m_args=std::move(args);
        this->m_stack_inc=1-vars.size();
        m_call_stack.reserve(max_stack_depth(m_postfix));
        return error;
    }
    template<class iden_parser_t=decltype(m_default_identifier_parser),//->invokable_with_stack_t<var_t>*
             class number_parser_t=default_number_parser<T>>           //->std::optional<std::pair<var_t,str_iterator_t>>
    parse_error_t parse(const std::vector<std::string>& vars,const std::string&body,
                        unsigned            op_flag,// op_flag - supported operations
                        iden_parser_t iden_parser=m_default_identifier_parser,
                        number_parser_t number_parser=default_number_parser<T>{})
    {
        return parse(vars,body.begin(),body.end(),op_flag,iden_parser,number_parser);
    }
    std::size_t arity()const{return m_args.size();}
    explicit operator bool()const{return !m_postfix.empty();}
    ~function()
    {
        m_clear();
    }
};

}// expr

#endif

