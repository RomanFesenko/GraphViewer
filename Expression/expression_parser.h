
#ifndef  _expression_parser_
#define  _expression_parser_

#include <vector>
#include <map>
#include <optional>
#include <string>
#include <functional>
#include <typeinfo>
#include <iostream>
#include <algorithm>
#include <numbers>

#include <math.h>
#include <assert.h>

#include "reversed_sequence.h"
#include "parse_error.h"
#include "string_comparer.h"
#include "defer.h"

//////////////////////////////////////////////////////////////
/// Generalized expression interpreter, with arbitrary
/// type of variables and operations (+, -, /, *),
/// support for parentheses
/// and function calls with an arbitrary number of arguments.
///////////////////////////////////////////////////////////////

template<class T>
void free_mem(std::vector<T*>&vec_ptrs)
{
    for(auto*ptr:vec_ptrs){delete ptr;}
    vec_ptrs.clear();
}

//////////////////////////////////////////////////
///  invokable_with_stack_t
//////////////////////////////////////////////////

/*invokable_with_stack_t - erase type
  functions with an arbitrary number of arguments
  the same type - the function pops the call from the stack
  number of elements equal to the number of arguments and
  pushes the result of a function call onto the stack.
*/

template<class T>
class invokable_with_stack_t
{
    public:
    static int m_num;
    invokable_with_stack_t()
    {
        ++m_num;
    }
    virtual void call_stack(std::vector<T>&)const{assert(false);};
    virtual int stack_increment()const{assert(false);};
    virtual bool is_expression()const{return false;}
    virtual invokable_with_stack_t<T>* clone()const{assert(false);}
    virtual ~invokable_with_stack_t()
    {
        --m_num;
    }
};

// Specialization for invokable_with_stack_t<T>
template<class T>
void free_mem(std::vector<invokable_with_stack_t<T>*>&postfix)
{
    for(auto*inv:postfix)
    {
        if(!inv->is_expression()) delete inv;
    }
    postfix.clear();
}

template<class T>
inline int invokable_with_stack_t<T>::m_num=0;

template<class T>
class constant_postfix_token_t:public invokable_with_stack_t<T>
{
    const T m_const;
    public:
    explicit constant_postfix_token_t(T value):m_const(value){}
    virtual void call_stack(std::vector<T>&stack)const override
    {
        stack.push_back(m_const);
    }
    virtual int stack_increment()const override
    {
        return 1;
    }
    invokable_with_stack_t<T>* clone()const override
    {
        return new constant_postfix_token_t(m_const);
    }
};

template<class T>
class variable_postfix_token_t:public invokable_with_stack_t<T>
{
    const T* m_var_ptr;
    public:
    explicit variable_postfix_token_t(const T* value_ptr):m_var_ptr(value_ptr){}
    virtual void call_stack(std::vector<T>&stack)const override
    {
        stack.push_back(*m_var_ptr);
    }
    virtual int stack_increment()const override
    {
        return 1;
    }
    invokable_with_stack_t<T>* clone()const override
    {
        return new variable_postfix_token_t(m_var_ptr);
    }
};


template<class functor_t,class T,class...args_t>
class function_postfix_token_t:public invokable_with_stack_t<T>
{
    functor_t m_functor;
    reversed_sequences<sizeof...(args_t)-1> m_sequences;
    // the value at the top of the call stack becomes
    // the last argument of the function call
    template<std::size_t...ints>
    T m_call_stack_impl(std::index_sequence<ints...> seq,const std::vector<T>&stack)const
    {
        auto _back=stack.cend()-1;
        return m_functor((*(_back-ints))...);
    }
    public:
    function_postfix_token_t(functor_t functor):m_functor(functor){}
    virtual void call_stack(std::vector<T>&stack)const override
    {
        T result=m_call_stack_impl(m_sequences,stack);
        if constexpr(sizeof...(args_t)>1)
        {
            stack.erase(stack.end()-sizeof...(args_t)+1,stack.end());
        }
        stack.back()=result;
    }
    virtual int stack_increment()const override
    {
        return 1-static_cast<int>(sizeof...(args_t));
    }
    invokable_with_stack_t<T>* clone()const override
    {
        return new function_postfix_token_t(m_functor);
    }
};


template<class T>
int max_stack_depth(const std::vector<invokable_with_stack_t<T>*>& output)
{
    int in_stack=0;
    int max_depth=0;
    for(const auto& tok_ptr:output)
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

///////////////////////////////////////////////////////////
///                      infix_token_t
///////////////////////////////////////////////////////////

template<class T>
class infix_token_t
{
    public:
    static int m_num;
    enum infix_type
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
    const infix_type m_type;
    public:
    explicit infix_token_t(infix_type type):m_type(type)
    {
        ++m_num;
    }
    virtual ~infix_token_t()
    {
        --m_num;
    }
    infix_type type()const{return m_type;}

    /* Actions with token and stacks during conversion
       from infix to postfix. Alternative-
       branching by token type in one function.
     */
    virtual void infix_to_postfix(std::vector<infix_token_t<T>*>&postfix_stack,
                                  std::vector<infix_token_t<T>*>&aux_stack){assert(false);};
    virtual int priority()const{assert(false);};
    virtual invokable_with_stack_t<T>* get_postfix_token()const{assert(false);};
};

template<class T>
inline int infix_token_t<T>::m_num=0;

template<class T>
bool check_parenthesis(const std::vector<infix_token_t<T>*>& infix)
{
    int delta=0;
    for(const auto& token:infix)
    {
        typename infix_token_t<T>::infix_type type=token->type();
        if(type==infix_token_t<T>::open_par_id)
        {
            ++delta;
        }
        else if(type==infix_token_t<T>::close_par_id)
        {
            --delta;
        }
        else{}
    }
    return delta==0;
}

// infix_to_postfix - converting an expression from an infix
// to postfix form using alg Dijkstra (priority stack)

template<class T>
void infix_to_postfix(const std::vector<infix_token_t<T>*>& input,std::vector<invokable_with_stack_t<T>*>& output)
{
    std::vector<infix_token_t<T>*> aux_stack;
    std::vector<infix_token_t<T>*> posfix_result;
    output.clear();
    for(const auto&  inf_tok:input)
    {
        inf_tok->infix_to_postfix(posfix_result,aux_stack);
    }
    while(!aux_stack.empty())
    {
        posfix_result.push_back(aux_stack.back());
        aux_stack.pop_back();
    }
   /* for(const auto&inf_tok:posfix_result)
    {
        switch(inf_tok->type())
        {
            case infix_token_t<T>::constant_id:
            std::cout<<"constant\n";
            break;

            case infix_token_t<T>::variable_id:
            std::cout<<"variable\n";
            break;

            case infix_token_t<T>::operation_id:
            std::cout<<"operation\n";
            break;

            case infix_token_t<T>::function_id:
            std::cout<<"function\n";
            break;

            default:assert(false);
        }
    }*/
    for(const auto&  inf_tok:posfix_result)
    {
        output.push_back(inf_tok->get_postfix_token());
    }
}

template<class T>
class constant_infix_token_t:public infix_token_t<T>
{
    const T m_const;
    public:
    explicit constant_infix_token_t(T value):
    infix_token_t<T>(infix_token_t<T>::constant_id),m_const(value){}

    virtual void infix_to_postfix(std::vector<infix_token_t<T>*>&postfix_stack,
                                  std::vector<infix_token_t<T>*>&aux_stack) override
    {
        postfix_stack.push_back(this);
    }
    virtual invokable_with_stack_t<T>* get_postfix_token()const override
    {
        return new constant_postfix_token_t(m_const);
    }
};

template<class T>
class variable_infix_token_t:public infix_token_t<T>
{
    const T* m_var_ptr;
    public:
    explicit variable_infix_token_t(const T* value):
    infix_token_t<T>(infix_token_t<T>::variable_id),m_var_ptr(value){}
    virtual void infix_to_postfix(std::vector<infix_token_t<T>*>&postfix_stack,
                                  std::vector<infix_token_t<T>*>&aux_stack) override
    {
        postfix_stack.push_back(this);
    }
    virtual invokable_with_stack_t<T>* get_postfix_token()const override
    {
        return new variable_postfix_token_t(m_var_ptr);
    }
};

template<class functor_t,class T>
class operation_infix_token_t:public infix_token_t<T>
{
    functor_t m_functor;
    int m_priority;
    public:
    explicit operation_infix_token_t(functor_t functor,int priority):
    infix_token_t<T>(infix_token_t<T>::operation_id),m_functor(functor),
    m_priority(priority){}

    /*
            When the operator moves to the auxiliary stack
            all operators are removed from the stack with
            of greater or equal priority as well as function.
    */
    virtual void infix_to_postfix(std::vector<infix_token_t<T>*>&postfix_stack,
                                  std::vector<infix_token_t<T>*>&aux_stack) override
    {
        while(!aux_stack.empty())
        {
            auto back=aux_stack.back();
            if(back->type()==infix_token_t<T>::operation_id)
            {
                if(back->priority()>=m_priority)
                {
                    postfix_stack.push_back(back);
                    aux_stack.pop_back();
                }
                else {break;}
            }
            else if(back->type()==infix_token_t<T>::function_id)
            {
                postfix_stack.push_back(back);
                aux_stack.pop_back();
            }
            else
            {
                break;
            }
        }
        aux_stack.push_back(this);
    }
    virtual int priority()const{return m_priority;}
    virtual invokable_with_stack_t<T>* get_postfix_token()const override
    {
        return new function_postfix_token_t<functor_t,T,T,T>(m_functor);
    }
};

template<class T>
class open_par_infix_token_t:public infix_token_t<T>
{
    public:
    open_par_infix_token_t():infix_token_t<T>(infix_token_t<T>::open_par_id){}
    virtual void infix_to_postfix(std::vector<infix_token_t<T>*>&postfix_stack,
                                  std::vector<infix_token_t<T>*>&aux_stack) override
    {
        aux_stack.push_back(this);
    }
};

template<class T>
class close_par_infix_token_t:public infix_token_t<T>
{
    public:
    close_par_infix_token_t():infix_token_t<T>(infix_token_t<T>::close_par_id){}
    /*
          The closing parenthesis pops out of the subsidiary
          stack all tokens up to the opening parenthesis
     */
    virtual void infix_to_postfix(std::vector<infix_token_t<T>*>&postfix_stack,
                                  std::vector<infix_token_t<T>*>&aux_stack) override
    {
        while(true)
        {
            assert(!aux_stack.empty());
            if(aux_stack.back()->type()!=infix_token_t<T>::open_par_id)
            {
                postfix_stack.push_back(aux_stack.back());
                aux_stack.pop_back();
            }
            else
            {
                break;
            }
        }
        aux_stack.pop_back();
    }
};


/// separator_infix_token_t - function call argument separator,
/// usually a comma

template<class T>
class separator_infix_token_t:public infix_token_t<T>
{
    public:
    separator_infix_token_t():infix_token_t<T>(infix_token_t<T>::separator_id){}

    /*
          The separator pushes out of the auxiliary
          stack all tokens up to the opening parenthesis
     */
    virtual void infix_to_postfix(std::vector<infix_token_t<T>*>&postfix_stack,
                                  std::vector<infix_token_t<T>*>&aux_stack) override
    {
        while(true)
        {
            assert(!aux_stack.empty());
            if(aux_stack.back()->type()!=infix_token_t<T>::open_par_id)
            {
                postfix_stack.push_back(aux_stack.back());
                aux_stack.pop_back();
            }
            else
            {
                break;
            }
        }
    }
};

template<class functor_t,class T,class...args_t>
class function_infix_token_t:public infix_token_t<T>
{
    functor_t m_functor;
    public:
    explicit function_infix_token_t(functor_t functor):
    infix_token_t<T>(infix_token_t<T>::function_id),m_functor(functor){}
    virtual void infix_to_postfix(std::vector<infix_token_t<T>*>&postfix_stack,
                                  std::vector<infix_token_t<T>*>&aux_stack) override
    {
        aux_stack.push_back(this);
    }
    virtual invokable_with_stack_t<T>* get_postfix_token()const override
    {
        return new function_postfix_token_t<functor_t,T,args_t...>(m_functor);
    }
};

template<class T>
class custom_function_infix_token_t:public infix_token_t<T>
{
    invokable_with_stack_t<T>* m_functor;
    public:
    custom_function_infix_token_t(invokable_with_stack_t<T>* functor):
    infix_token_t<T>(infix_token_t<T>::function_id),
    m_functor(functor){}
    virtual void infix_to_postfix(std::vector<infix_token_t<T>*>&postfix_stack,
                                  std::vector<infix_token_t<T>*>&aux_stack) override
    {
        aux_stack.push_back(this);
    }
    virtual invokable_with_stack_t<T>* get_postfix_token()const override
    {
        return m_functor;
    }
};

enum operation_t:unsigned int
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

const unsigned int arithmetics_fl=plus_id|
                                 minus_id|
                                   mul_id|
                                   div_id|
                                   mod_id;

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

template<class return_t,class...args_list>
using function_ptr_t=return_t(*)(args_list...);

template<class T>
struct op_function_t
{
    function_ptr_t<T,const T&,const T&> m_func_ptr;
    int m_priority;
};

template<class T>
using operation_map_t=std::map<char,op_function_t<T>>;

#define insert_op(op,pr) {result.insert({'op',\
op_function_t<T>{[](const T& a,const T& b){return a op b;},pr}});}
template<class T,unsigned int flag>
constexpr operation_map_t<T> get_operations_map()
{
    operation_map_t<T> result;
    //арифметика
    if constexpr(flag&plus_id)
    {
        result.insert({'+',op_function_t<T>{[](const T& a,const T& b){return a + b;},1}});
    }
    if constexpr(flag&minus_id)
    {
        result.insert({'-',op_function_t<T>{[](const T& a,const T& b){return a - b;},1}});
    }
    if constexpr(flag&mul_id)
    {
        result.insert({'*',op_function_t<T>{[](const T& a,const T& b){return a * b;},2}});
    }
    if constexpr(flag&div_id)
    {
        result.insert({'/',op_function_t<T>{[](const T& a,const T& b){return a/b;},2}});
    }
    if constexpr(flag&mod_id)
    {
        result.insert({'%',op_function_t<T>{[](const T& a,const T& b){return a%b;},2}});
    }
    return result;
}

/// //////////////////////////////////////////////
///                   Tokenization
/// //////////////////////////////////////////////

template<class T>
using string_map_t=std::map<std::string,T,string_comparer_t>;

template<class T>
using builtin_function_map_t=string_map_t<function_ptr_t<T,T>>;

// Built-in functions
template<class T>
constexpr builtin_function_map_t<T> get_builtin_function_map()
{
    using value_type=std::pair<std::string,function_ptr_t<T,T>>;
    return builtin_function_map_t<T>
    {
           value_type("abs",abs),
           value_type("sqrt",sqrt),
           value_type("exp",exp),
           value_type("log",log),
           value_type("cos",cos),
           value_type("sin",sin),
           value_type("tan",tan),
           value_type("asin",asin),
           value_type("acos",acos),
           value_type("atan",atan)
    };
}


// Built-in constants
template<class T>
constexpr string_map_t<T> get_builtin_constant_map()
{
    namespace num=std::numbers;
    using value_type=std::pair<std::string,T>;
    return string_map_t<T>
    {
         value_type("pi",num::pi_v<T>),
         value_type("e",num::e_v<T>)
    };
}



inline bool is_digit(char sym)
{
    return (sym>='0')&&(sym<='9');
}

inline bool is_number_char(char sym)
{
    return is_digit(sym)||sym=='.';
}

inline bool is_letter(char sym)
{
    return ((sym>='a')&&(sym<='z'))||((sym>='A')&&(sym<='Z'));
}

inline bool is_iden_begin(char sym)
{
    return is_letter(sym)||sym=='_';
}

inline bool is_iden_end(char sym)
{
    return !(is_iden_begin(sym)||is_digit(sym));
}


inline bool is_ignore(char sym)
{
    return sym==' '||sym=='\n'||sym=='\t';
}

template<class str_iterator_t>
std::optional<str_iterator_t>
extract_number(str_iterator_t beg,str_iterator_t end)
{
    std::size_t num_point=0;
    for(;beg!=end;++beg)
    {
        if (is_digit(*beg)) {}
        else if(*beg=='.')
        {
            num_point++;
            if(num_point>1) return {};
        }
        else {break;}
    }
    return beg;
}

/* tokenize - converting the string [str_beg, str_end) into a set of tokens.
  identifier_parser_t - by the found identifier
  returns a token (function, constant, or variable).
  number_parser_t - returns by the found number
  a constant of type var_t
*/
template<class var_t,
         class identifier_parser_t,//->infix_token_t<var_t>*
         class number_parser_t>//->infix_token_t<var_t>*
parse_error_t
tokenize(typename std::string::const_iterator str_beg,
         typename std::string::const_iterator str_end,
         const operation_map_t<var_t>& oper_map,
         identifier_parser_t& iden_parser,
         number_parser_t& num_parser,
         std::vector<infix_token_t<var_t>*>&infix)
{
    using op_functor_t=function_ptr_t<var_t,const var_t&,const var_t&>;
    using str_iterator_t=std::string::const_iterator;
    for(auto caret=str_beg;caret<str_end;)
    {
        if(is_ignore(*caret))
        {
            ++caret;
        }
        else if(auto iter=oper_map.find(*caret);iter!=oper_map.end())
        {
            //unary plus or minus
            if((*caret=='+'||*caret=='-')&&(caret==str_beg||
                                           *(caret-1)=='('||
                                           *(caret-1)==','))
            {
                infix.push_back(new constant_infix_token_t<var_t>(0));
            }
            infix.push_back(new operation_infix_token_t<op_functor_t,var_t>(iter->second.m_func_ptr,
                                                                            iter->second.m_priority));
            ++caret;
        }
        else if(*caret=='(')
        {
            infix.push_back(new open_par_infix_token_t<var_t>);
            ++caret;
        }
        else if(*caret==')')
        {
            infix.push_back(new close_par_infix_token_t<var_t>);
            ++caret;
        }
        else if(*caret==',')
        {
            infix.push_back(new separator_infix_token_t<var_t>);
            ++caret;
        }
        else if(is_iden_begin(*caret))
        {
            str_iterator_t end_iden=std::find_if(caret,str_end,is_iden_end);
            infix_token_t<var_t>* tok=iden_parser.find(caret,end_iden);
            if(tok==nullptr)
            {
                return {parse_error_t::unknown_identifier_id,std::string(caret,end_iden)};
            }
            infix.push_back(tok);
            caret=end_iden;
        }
        else if(is_number_char(*caret))
        {
            std::optional<std::pair<var_t,str_iterator_t>> pair=num_parser(caret,str_end);
            if(!pair)
            {
                return {parse_error_t::unknown_identifier_id,std::string(caret,pair->second)};
            }
            infix.push_back(new constant_infix_token_t(pair->first));
            caret=pair->second;
        }
        else
        {
            return {parse_error_t::unknown_identifier_id,std::string(caret,caret+1)};
        }
    }
    return {parse_error_t::success_id,""};
}

template<class value_t,class value_parser_t>
class number_parser_t
{
    value_parser_t m_value_parser;
    using str_citerator=std::string::const_iterator;
    public:
    using stonum_t=value_parser_t;

    number_parser_t(value_parser_t vpr):m_value_parser(vpr){}
    std::pair<infix_token_t<value_t>*,str_citerator> find(str_citerator begin,str_citerator end)const
    {
        auto num_end=extract_number(begin,end);
        if(!num_end) return {nullptr,end};
        return {new constant_infix_token_t<value_t>(m_value_parser(std::string(begin,*num_end))),*num_end};
    }
    stonum_t get_parser()const{return m_value_parser;}
};

///expression_t
// op_flag - supported operations

template<class T,unsigned int op_flag>
class expression_t:public invokable_with_stack_t<T>
{
    using str_citerator=std::string::const_iterator;
    using self_t=expression_t<T,op_flag>;

    mutable std::vector<T> m_variables;
    mutable std::vector<char> m_is_used;//f(x,y,z)=x+x;

    std::size_t m_stack_depth=-1;
    std::vector<invokable_with_stack_t<T>*> m_postfix;
    mutable std::vector<T> m_variable_stack;

    //external_parser_t::is_name()->bool
    //                 ::find_function()->const expression_t*
    //                 ::find_constant()->const T*
    template<class external_parser_t>
    struct identifier_parser_t
    {
        const external_parser_t&    m_external_parser;
        const string_map_t<T>& m_builtin_constant;
        const builtin_function_map_t<T>&m_builtin_function;

        string_map_t<const T*> m_variable_map;
        std::function<void(const T*)> m_use_signal;

        identifier_parser_t(const external_parser_t&ep,
                            const string_map_t<T>&bc,
                            const builtin_function_map_t<T>&bf):
        m_external_parser(ep),
        m_builtin_constant(bc),
        m_builtin_function(bf)
        {}
        infix_token_t<T>* find(str_citerator begin,str_citerator end)const
        {
            auto ipair=std::make_pair(begin,end);
            //argument of function
            if(auto iter=m_variable_map.find(ipair);iter!=m_variable_map.end())
            {
                if(m_use_signal) m_use_signal(iter->second);
                return new variable_infix_token_t<T>(iter->second);
            }
            // builtin constant
            if(auto _const=m_builtin_constant.find(ipair);
                    _const!=m_builtin_constant.end())
            {
                return new constant_infix_token_t(_const->second);
            }
            // builtin function
            if(auto iter=m_builtin_function.find(ipair);
                    iter!=m_builtin_function.end())
            {
                return new function_infix_token_t<function_ptr_t<T,T>,T,T>(iter->second);
            }
            // added constant
            if(const T* _var=m_external_parser.find_constant(begin,end);
               _var!=nullptr)
            {
                return new variable_infix_token_t<T>(_var);
            }
            // added function
            if(const self_t* _exp=m_external_parser.find_function(begin,end);
               _exp!=nullptr)
            {
                return new custom_function_infix_token_t<T>(const_cast<self_t*>(_exp));
            }
            //unknown identifier
            return nullptr;
        }
        bool is_name(str_citerator begin,str_citerator end)const
        {
            return m_external_parser.is_name(begin,end)||
                   m_variable_map.find(std::make_pair(begin,end))!=m_variable_map.end()||
                   m_builtin_constant.find(std::make_pair(begin,end))!=m_builtin_constant.end()||
                   m_builtin_function.find(std::make_pair(begin,end))!=m_builtin_function.end();
        }
    };

    public:
    inline static const string_map_t<T>  m_builtin_constant=get_builtin_constant_map<T>();
    inline static const builtin_function_map_t<T> m_builtin_function=get_builtin_function_map<T>();
    inline static const operation_map_t<T> m_operation_map=get_operations_map<T,op_flag>();
    expression_t(){}
    expression_t(const expression_t&)=delete;
    expression_t& operator=(const expression_t&)=delete;

    virtual void call_stack(std::vector<T>&stack)const override
    {
        auto copy_beg=stack.end()-m_variables.size();
        std::copy(copy_beg,stack.end(),m_variables.begin());
        stack.erase(copy_beg,stack.end());
        evaluate_postfix(m_postfix,stack);
    }
    virtual int stack_increment()const override
    {
        return 1-m_variables.size();
    }
    virtual bool is_expression()const override{return true;}

    //parsing WITH saving the current state
    //in case of parse failure
    template<class iden_parser_t,//->infix_token_t<var_t>*
             class number_parser_t>//->infix_token_t<var_t>*
    parse_error_t parse(const std::vector<std::string>& vars,
                        str_citerator begin,str_citerator end,
                        iden_parser_t& external_,
                        number_parser_t& number_parser)
    {
        identifier_parser_t iden_paser(external_,
                                       m_builtin_constant,
                                       m_builtin_function);

        std::vector<T> variable_for_new(vars.size());
        std::vector<char> new_is_used(vars.size(),0);
        iden_paser.m_use_signal=[&](const T*var)
        {
            int index=var-&variable_for_new[0];
            assert(index>=0&&index<variable_for_new.size());
            new_is_used[index]=1;
        };

        int i=0;
        for(const auto&str:vars)
        {
            if(iden_paser.is_name(str.begin(),str.end())) return {parse_error_t::name_conflict_id,str};
            iden_paser.m_variable_map.insert({str,&variable_for_new[i++]});
        }

        std::vector<infix_token_t<T>*> infix;
        std::vector<invokable_with_stack_t<T>*> new_postfix;

        scope_guard_t defer_del_infix([&infix](){free_mem(infix);});
        scope_guard_t defer_del_postfix([&new_postfix](){free_mem(new_postfix);});

        ///tokenize to infix form
        auto result=tokenize(begin,end,m_operation_map,
                             iden_paser,number_parser,infix);

        //std::cout<<"tokenize complete\n";
        if(!result.is_valid())
        {
            return result;
        }
        if(!check_parenthesis(infix))
        {
            return {parse_error_t::parenthesis_error_id,""};
        }

        //transform from infix to postfix form
        infix_to_postfix(infix,new_postfix);

        // determine the maximum depth of the call stack
        // to avoid allocating memory for new ones
        // values ​​at the stage of interpretation
        std::size_t new_stack_depth;
        if(new_stack_depth=max_stack_depth(new_postfix);new_stack_depth==-1)
        {
            return {parse_error_t::syntax_error_id,""};
        }
        // parsing correct,accept changings
        m_variables=std::move(variable_for_new);
        m_is_used=std::move(new_is_used);
        m_stack_depth=new_stack_depth;
        m_variable_stack.reserve(m_stack_depth);
        free_mem(m_postfix);
        m_postfix=std::move(new_postfix);
        new_postfix.clear();
        return {parse_error_t::success_id,""};
    }
    std::size_t arity()const{return m_variables.size();}

    template<class...args_t>
    T operator()(args_t...args)const
    {
        assert(arity()==sizeof...(args_t));
        m_variables={(args)...};
        m_variable_stack.clear();
        evaluate_postfix(m_postfix,m_variable_stack);
        assert(m_variable_stack.size()==1);
        return m_variable_stack[0];
    }
    bool is_variable_used(int i)const{return m_is_used[i];}
    ~expression_t()
    {
        free_mem(m_postfix);
    }
};

#endif

