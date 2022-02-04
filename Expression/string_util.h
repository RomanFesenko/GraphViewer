
#ifndef  _string_util_
#define  _string_util_

#include<string>
#include<type_traits>
#include<cstring>
#include<algorithm>
#include<utility>


namespace sutil{

template<class range_t>
auto str_begin(const range_t& str)
{
    using type=std::remove_cv_t<std::remove_reference_t<range_t>>;
    if constexpr(std::is_same_v<type,char*>||std::is_same_v<type,const char*>)
    {
        return str;
    }
    else
    {
        return std::begin(str);
    }
}

template<class range_t>
auto str_end(const range_t& str)
{
    using type=std::remove_cv_t<std::remove_reference_t<range_t>>;
    if constexpr(std::is_same_v<type,char*>||std::is_same_v<type,const char*>)
    {
        return str + std::strlen(str);
    }
    else
    {
        return std::end(str);
    }
}

struct string_comparer_t
{
    using is_transparent=int;
    using str_citerator=std::string::const_iterator;
    using ipair=std::pair<str_citerator,str_citerator>;
    bool operator()(const std::string&str1,const std::string&str2)const
    {
        return str1<str2;
    }
    bool operator()(const ipair&pr,const std::string&str)const
    {
        return std::lexicographical_compare(pr.first,pr.second,
                                            str.begin(),str.end());
    }
    bool operator()(const std::string&str,const ipair&pr)const
    {
        return std::lexicographical_compare(str.begin(),str.end(),
                                            pr.first,pr.second);
    }
};


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
str_iterator_t extract_number(const str_iterator_t beg,const str_iterator_t end)
{
    str_iterator_t current=beg;
    std::size_t num_point=0;
    for(;current!=end;++current)
    {
        if (is_digit(*current)) {}
        else if(*current=='.')
        {
            num_point++;
            if(num_point>1) return beg;
        }
        else {break;}
    }
    return current;
}

template<class iter1_t,class iter2_t>
iter1_t find_identifier(iter1_t input_begin,iter1_t input_end,
                        iter2_t iden_begin,iter2_t iden_end)
{
    while(true)
    {
        auto finded=std::search(input_begin,input_end,iden_begin,iden_end);
        if(finded==input_end) return input_end;
        if(finded!=input_begin&&is_iden_begin(finded[-1]))
        {
            input_begin=finded+1;
            continue;
        }
        auto finded_end=finded+(iden_end-iden_begin);
        if(finded_end!=input_end&&is_iden_begin(*finded_end))
        {
            input_begin=finded+1;
            continue;
        }
        return finded;
    }
}

template<class r1_t,class r2_t>
auto find_identifier(const r1_t&r1,const r2_t&r2)
{
   return  find_identifier(str_begin(r1),str_end(r1),str_begin(r2),str_end(r2));
}


}// sutil

#endif

