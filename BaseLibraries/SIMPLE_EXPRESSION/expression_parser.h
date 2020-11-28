
#ifndef  _expression_parser_
#define  _expression_parser_

#include <vector>
#include <map>
#include <string>
#include <functional>
#include <typeinfo>

#include <math.h>
#include <assert.h>

#include "reversed_sequence.h"
#include "parse_error.h"
#include "string_comparer.h"

///************************************************************
///  Обобщенный интерпретатор выражений, с произвольным       *
///  типом переменных и операций (+,-,/,*), поддержкой скобок *
///  и вызовов функций с произвольным числом аргументов.      *
///************************************************************

template<class T>
struct deleter_t
{
    void operator()(T*ptr){delete ptr;}
};

template<class T>
class vector_ptrs_guard_t
{
    const std::vector<T*>& m_vector;
    deleter_t<T> m_deleter;
    public:
    explicit vector_ptrs_guard_t(const std::vector<T*>& vec):m_vector(vec){}
    ~vector_ptrs_guard_t()
    {
        //std::cout<<"delete beginning: "<< m_vector.size()<<std::endl;
        for(T* elem:m_vector) m_deleter(elem);
        //std::cout<<"deleting complete\n";
    }
};

//////////////////////////////////////////////////
///  invokable_with_stack_t
//////////////////////////////////////////////////

/*invokable_with_stack_t - стирание типа
  функций с произвольным количеством аргументов
  одинакового типа- функция снимает со стека вызова
  число элементов равное числу аргументов и
  кладет на стек результат вызова функции.
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
    virtual ~invokable_with_stack_t()
    {
        --m_num;
    }
};

template<class T>
struct deleter_t<invokable_with_stack_t<T>>
{
    void operator()(invokable_with_stack_t<T>* inv)
    {
       // std::cout<<"delete: "<<std::string(typeid(*inv).name())<<std::endl;
        if(!inv->is_expression()) delete inv;
    }
};

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
};




template<class functor_t,class T,class...args_t>
class function_postfix_token_t:public invokable_with_stack_t<T>
{
    functor_t m_functor;
    reversed_sequences<sizeof...(args_t)-1> m_sequences;
    // значение на верхушке стека вызовов становится
    // последним аргументом вызова функции
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
T evaluate_postfix(const std::vector<invokable_with_stack_t<T>*>& postfix,
                   std::vector<T>& stack)
{
    for(const auto& tok_ptr:postfix)
    {
        tok_ptr->call_stack(stack);
    }
    assert(stack.size()==1);
    return stack[0];
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

    /* Действия с токеном и стеками при преобразовании
       из инфиксной формы в постфиксную.Альтернатива-
       ветвление по типу токена в одной функции.
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

// infix_to_postfix - преобразование выражения из инфиксной
// в постфиксную форму с помощью алг дейкстры (стек с приоритетами)

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
        При перемещении оператора во вспомогательный
        стек из стека удаляются все операторы с
        большим или равным приоритетом, а также функции.
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
        Закрывающаяся скобка выталкивает из вспомогательного
        стека все токены до открывающейся скобки
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


///separator_infix_token_t - разделитель аргументов вызова функции,
/// обычно запятая

template<class T>
class separator_infix_token_t:public infix_token_t<T>
{
    public:
    separator_infix_token_t():infix_token_t<T>(infix_token_t<T>::separator_id){}

    /*
        Разделитель выталкивает из вспомогательного
        стека все токены до открывающейся скобки
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
    //арифметика
    plus_id=1,
    minus_id=1<<1,
    mul_id=1<<2,
    div_id=1<<3,
    mod_id=1<<4,
    neg_id=1<<5,
    //сравнения
    equal_id=1<<6,
    not_equal_id=1<<7,
    greater_id=1<<8,
    less_id=1<<9,
    greater_equal_id=1<<10,
    less_equal_id=1<<11,
    //булевы операции
    log_and_id=1<<12,
    log_or_id=1<<13,
    log_not_id=1<<14,
    //побитовые операции
    bit_and_id=1<<15,
    bit_or_id=1<<16,
    bit_not_id=1<<17,
};

// только бинарные операции

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
operation_map_t<T> get_operations_map()
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
///                   Токенизация
/// //////////////////////////////////////////////

template<class T>
using string_map_t=std::map<std::string,T,string_comparer_t>;

template<class T>
using builtin_function_map_t=string_map_t<function_ptr_t<T,T>>;

// Встроеные функции
template<class T>
builtin_function_map_t<T> get_builtin_function_map()
{
    using value_type=std::pair<std::string,function_ptr_t<T,T>>;
    return builtin_function_map_t<T>{
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

// Встроеные константы
template<class T>
string_map_t<T> get_builtin_constant_map()
{
    using value_type=std::pair<std::string,T>;
    return string_map_t<T>{
           value_type("pi",T(3.1415926535897932385L))
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


/* tokenize - преобразование строки [str_beg,str_end) в набор токенов.
  identifier_parser_t - по найденому идентификатору
  возвращает токен (функцию, константу, или переменную).
  number_parser_t - по найденому числу возвращает
  константу типа var_t
*/
template<class var_t,
         class identifier_parser_t,//->infix_token_t<var_t>*
         class number_parser_t>//->infix_token_t<var_t>*
parse_error_t
tokenize(typename std::string::const_iterator str_beg,
         typename std::string::const_iterator str_end,
         const operation_map_t<var_t>& oper_map,
         identifier_parser_t iden_parser,
         number_parser_t num_parser,
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
            // унарный плюс или минус
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
            std::pair<infix_token_t<var_t>*,str_iterator_t> pair=num_parser.find(caret,str_end);
            if(pair.first==nullptr)
            {
                return {parse_error_t::unknown_identifier_id,std::string(caret,pair.second)};
            }
            infix.push_back(pair.first);
            caret=pair.second;
        }
        else
        {
            return {parse_error_t::unknown_identifier_id,std::string(caret,caret+1)};
        }
    }
    return {parse_error_t::success_id,""};
}

///function_parser_t
/*
    По заданому идентификатору возвращает функцию
    или нуллпоинтер.
*/

template<class T>
class function_parser_t
{
    builtin_function_map_t<T> m_builtin;
    using function_map_t=string_map_t<invokable_with_stack_t<T>*>;
    using str_iterator_t=std::string::const_iterator;
    function_map_t m_added;
    public:
    function_parser_t():m_builtin(get_builtin_function_map<T>()){}
    function_map_t& added_function(){return m_added;}
    const function_map_t& added_function()const{return m_added;}
    infix_token_t<T>* find(str_iterator_t begin,str_iterator_t end)const
    {
        assert(is_iden_begin(*begin));
        auto ipair=std::make_pair(begin,end);
        if(auto iter=m_builtin.find(ipair);iter!=m_builtin.end())
        {
            //std::cout<<"builtin function: "<<iden<<std::endl;
            return new function_infix_token_t<function_ptr_t<T,T>,T,T>(iter->second);
        }
        else if(auto iter=m_added.find(ipair);iter!=m_added.end())
        {
            //std::cout<<"added function: "<<iden<<std::endl;
            return new custom_function_infix_token_t<T>(iter->second);
        }
        else
        {
            return nullptr;
        }
    }
    // зарегистрировать функцию
    bool add_function(const std::string&iden,invokable_with_stack_t<T>* func)
    {
        if(m_added.find(iden)!=m_added.end()) return false;
        m_added.insert({iden,func});
        return true;
    }
    bool is_name(str_iterator_t begin,str_iterator_t end)const
    {
        auto ipair=std::make_pair(begin,end);
        return m_builtin.find(ipair)!=m_builtin.end()||
               m_added.find(ipair)!= m_added.end();
    }
    void clear(){m_added.clear();}
};

///constant_parser_t
/*
    По заданому идентификатору возвращает константу
    или нуллпоинтер.
*/

template<class T>
class constant_parser_t
{
    string_map_t<T> m_constant_map;
    using str_iterator_t=std::string::const_iterator;
    public:
    constant_parser_t()/*:m_constant_map(get_builtin_constant_map<T>())*/{}
    infix_token_t<T>* find(str_iterator_t begin,str_iterator_t end)const
    {
        auto ipair=std::make_pair(begin,end);
        if(auto iter=m_constant_map.find(ipair);iter!=m_constant_map.end())
        {
            //std::cout<<"constant: "<<iden<<std::endl;
            return new constant_infix_token_t<T>(iter->second);
        }
        return nullptr;
    }
    bool add_constant(const std::string&iden,T const_)
    {
        if(m_constant_map.find(iden)!=m_constant_map.end()) return false;
        m_constant_map.insert({iden,const_});
        return true;
    }
    bool is_name(str_iterator_t begin,str_iterator_t end)const
    {
        auto ipair=std::make_pair(begin,end);
        return m_constant_map.find(ipair)!=m_constant_map.end();
    }
    void clear(){m_constant_map.clear();}
};

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

///expression_t - класс выражения
// op_flag - поддерживаемые операции

template<class T,class value_parser_t,unsigned int op_flag>
class expression_t:public invokable_with_stack_t<T>
{
    using str_citerator=std::string::const_iterator;
    mutable std::vector<T> m_variable;

    number_parser_t<T,value_parser_t> m_number_parser;
    const constant_parser_t<T>& m_constant_parser;
    const operation_map_t<T> m_operation_map;
    const function_parser_t<T>& m_function_parser;

    std::vector<invokable_with_stack_t<T>*> m_postfix;
    mutable std::vector<T> m_variable_stack;

    struct identifier_parser_t
    {
        const constant_parser_t<T>& m_constant_parser;
        const function_parser_t<T>& m_function_parser;

        string_map_t<const T*> m_variable_map;

        identifier_parser_t(decltype(m_constant_parser) constant_parser,
                            decltype(m_function_parser) function_parser):
        m_constant_parser(constant_parser),
        m_function_parser(function_parser)
        {
        }
        infix_token_t<T>* find(str_citerator begin,str_citerator end)const
        {
            auto ipair=std::make_pair(begin,end);
            //переменная
            if(auto iter=m_variable_map.find(ipair);iter!=m_variable_map.end())
            {
                //std::cout<<"variable: "<<std::string(begin,iden_end)<<std::endl;
                return new variable_infix_token_t<T>(iter->second);
            }
            // константа
            if(auto _const=m_constant_parser.find(begin,end);_const!=nullptr) return _const;
            //функция или ошибка
            return m_function_parser.find(begin,end);
        }
        bool is_name(str_citerator begin,str_citerator end)const
        {
            return m_constant_parser.is_name(begin,end)||
                   m_function_parser.is_name(begin,end)||
                   m_variable_map.find({begin,end})!=m_variable_map.end();
        }
    };

    public:

    using stonum_t=value_parser_t;

    expression_t(value_parser_t num_parser,
                 const constant_parser_t<T>& const_parser,
                 const function_parser_t<T>& func_parser):
    m_number_parser(num_parser),
    m_constant_parser(const_parser),
    m_operation_map(get_operations_map<T,op_flag>()),
    m_function_parser(func_parser)
    {
        m_variable.reserve(10);
    }
    expression_t(const expression_t&)=delete;
    expression_t& operator=(const expression_t&)=delete;

    virtual void call_stack(std::vector<T>&stack)const override
    {
        std::copy(stack.end()-m_variable.size(),stack.end(),m_variable.begin());
        stack.erase(stack.end()-m_variable.size()+1,stack.end());
        stack.back()=evaluate_postfix(m_postfix,m_variable_stack);
        m_variable_stack.clear();
        //std::cout<<"call_stack:"<<stack.back()<<std::endl;
    }
    virtual int stack_increment()const override
    {
        return 1-m_variable.size();
    }
    virtual bool is_expression()const override{return true;}

    parse_error_t parse(const std::vector<std::string>& vars,
                                         str_citerator begin,
                                          str_citerator end)
    {
        identifier_parser_t iden_paser(m_constant_parser,
                                      m_function_parser);
        for(const auto&str:vars)
        {
            if(iden_paser.is_name(str.begin(),str.end())) return {parse_error_t::name_conflict_id,str};
            m_variable.resize(m_variable.size()+1);
            iden_paser.m_variable_map.insert({str,&m_variable.back()});
        }

        std::vector<infix_token_t<T>*> infix;
        vector_ptrs_guard_t<infix_token_t<T>> inf_guard(infix);

        ///токенизация в инфиксную фориу
        auto result=tokenize(begin,end,m_operation_map,
                             iden_paser,m_number_parser,infix);

        //std::cout<<"tokenize complete\n";
        if(!result.is_valid())
        {
            return result;
        }
        if(!check_parenthesis(infix))
        {
            return {parse_error_t::parenthesis_error_id,""};
        }

        //преобразование из инфиксной в постфиксную форму
        infix_to_postfix(infix,m_postfix);
        //std::cout<<"infix_to_postfix complete\n";

        // определение максимальной глубины стека вызова
        // чтобы избежать выделения памяти под новые
        // значения на этапе интерпретации
        if(int depth=max_stack_depth(m_postfix);depth==-1)
        {
            {
                vector_ptrs_guard_t<invokable_with_stack_t<T>> delete_(m_postfix);
            }
            m_postfix.clear();
            return {parse_error_t::syntax_error_id,""};
        }
        else
        {
            m_variable_stack.reserve(depth);
        }
        return {parse_error_t::success_id,""};
    }
    std::size_t variables()const{return m_variable.size();}
    stonum_t get_number_parser()const{return m_number_parser.get_parser();}

    T evaluate_with_stack(const std::vector<T>&stack)const
    {
        std::vector aux(stack);
        call_stack(aux);
        assert(aux.size()==1);
        return aux.back();
    }
    ~expression_t()
    {
        //std::cout<<"deleted: "<<name()<<std::endl;
        vector_ptrs_guard_t<invokable_with_stack_t<T>> delete_(m_postfix);
    }
};

#endif

