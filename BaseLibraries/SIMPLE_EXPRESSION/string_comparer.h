
#ifndef  _string_comparer_
#define  _string_comparer_

#include<string>
#include<algorithm>
#include<utility>


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


#endif

