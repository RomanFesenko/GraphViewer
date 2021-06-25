#include <assert.h>
#include "test_parsing.h"
#include "../expression_parser.h"


#include  "test_common.h"
#include  <algorithm>
#include  <limits>


using real_t=double;
bool real_eq(real_t a,real_t b)
{
    static const real_t eps_10=std::numeric_limits<real_t>::epsilon()*10;
    return abs(a-b)<eps_10;
}

// floating point functions
using fp_expression_t=
expression_t<real_t,arithmetics_fl&(~mod_id)>;

const real_t const_a=1.4;
const real_t const_b=2.1;
const real_t const_c=6.9;

fp_expression_t gl_exp,gl_exp_1,gl_exp_2,gl_exp_res;

struct range_t
{
    real_t min;
    real_t max;
    real_t inc;
};

template<class F1,class F2>
real_t accumulate_error(const F1&f1,const F2&f2,range_t rg)
{
    real_t res=0;
    for(real_t i=rg.min;i<rg.max;i+=rg.inc)
    {
        res+=abs(f1(i)-f2(i));
    }
    return res;
}

template<class F1,class F2>
real_t accumulate_error(const F1&f1,const F2&f2,
                        range_t rg1,range_t rg2)// for any more ranges?..
{
    real_t res=0;
    for(real_t i=rg1.min;i<rg1.max;i+=rg1.inc)
    {
        for(real_t j=rg2.min;j<rg2.max;j+=rg2.inc)
        {
            res+=abs(f1(i,j)-f2(i,j));
        }
    }
    return res;
}

template<class F1,class F2>
real_t accumulate_error(const F1&f1,const F2&f2,
                        range_t rg1,range_t rg2,range_t rg3)// for any more ranges?..
{
    real_t res=0;
    for(real_t i=rg1.min;i<rg1.max;i+=rg1.inc)
    {
        for(real_t j=rg2.min;j<rg2.max;j+=rg2.inc)
        {
            for(real_t k=rg3.min;k<rg3.max;k+=rg3.inc)
            {
                res=std::max(res,abs(f1(i,j,k)-f2(i,j,k)));
            }
        }
    }
    return res;
}

struct iden_parser_t
{
    using str_iterator=std::string::const_iterator;
    string_map_t<real_t>  m_constant;
    string_map_t<fp_expression_t*> m_expression;
    bool is_name(str_iterator b,str_iterator e)const
    {
        auto pr=std::make_pair(b,e);
        return m_constant.find(pr)!=m_constant.end()||
               m_expression.find(pr)!=m_expression.end();
    }
    const real_t*find_constant(str_iterator b,str_iterator e)const
    {
        auto iter=m_constant.find(std::make_pair(b,e));
        if(iter==m_constant.end()) return nullptr;
        return &(iter->second);
    }
    const fp_expression_t*find_function(str_iterator b,str_iterator e)const
    {
        auto iter=m_expression.find(std::make_pair(b,e));
        if(iter==m_expression.end()) return nullptr;
        return iter->second;
    }
};
iden_parser_t gl_iden_parser;


std::optional<std::pair<real_t,std::string::const_iterator>>
static num_parser(std::string::const_iterator begin,
                  std::string::const_iterator end)
{
    auto num_end=extract_number(begin,end);
    if(!num_end) return {};
    return std::make_pair(std::stod(std::string{begin,end}),*num_end);
}

void test_parsing()
{
    gl_iden_parser.m_constant.insert({"const_a",const_a});
    gl_iden_parser.m_constant.insert({"const_b",const_b});
    gl_iden_parser.m_constant.insert({"const_c",const_c});
    {
        std::vector<std::string> vars={"x"};
        std::string body="1+2*x";
        auto check=[](real_t x){return 1+2*x;};
        auto pr=gl_exp.parse(vars,body.begin(),body.end(),
                            gl_iden_parser,num_parser);
        assert(pr.is_valid());
        real_t error=accumulate_error(check,gl_exp,{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        std::vector<std::string> vars={"x"};
        std::string body="(1+2)*x";
        auto check=[](real_t x){return (1+2)*x;};
        auto pr=gl_exp.parse(vars,body.begin(),body.end(),
                             gl_iden_parser,num_parser);
        assert(pr.is_valid());
        real_t error=accumulate_error(check,gl_exp,{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        std::vector<std::string> vars={"x","y"};
        std::string body="(y+2)*x";
        auto check=[](real_t x,real_t y){return (y+2)*x;};
        auto pr=gl_exp.parse(vars,body.begin(),body.end(),
                             gl_iden_parser,num_parser);
        assert(pr.is_valid());
        real_t error=accumulate_error(check,gl_exp,{0,1,0.1},{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        std::vector<std::string> vars={"s","t","time"};
        std::string body="1 / (0.3 + s) * cos (4.0 * time-4.0 * s)";
        auto check=[](real_t s,real_t t,real_t time)
        {
            return  1/ (0.3 + s) * cos (4.00 * time-4.0 * s);
        };
        auto pr=gl_exp.parse(vars,body.begin(),body.end(),
                             gl_iden_parser,num_parser);
        assert(pr.is_valid());
        real_t error=accumulate_error(check,gl_exp,{0,5,0.5},
                                                    {0,3,0.1},
                                                    {0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        std::vector<std::string> vars={"x","y"};
        std::string body="(exp(1.567*y)+2*y)*x+cos(x-y*sin(x))/(1.8+x*x)";
        auto check=[](real_t x,real_t y)
        {
            return (exp(1.567*y)+2*y)*x+cos(x-y*sin(x))/(1.8+x*x);
        };
        auto pr=gl_exp.parse(vars,body.begin(),body.end(),
                             gl_iden_parser,num_parser);
        assert(pr.is_valid());
        real_t error=accumulate_error(check,gl_exp,{0,1,0.1},{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        std::vector<std::string> vars={"x","y"};
        std::string body="(exp(const_b*y)+const_a*y)*x+     \
                            cos(x-y*sin(x))/(const_c+x*x)";
        auto check=[](real_t x,real_t y)
        {
            return (exp(const_b*y)+const_a*y)*x+
                    cos(x-y*sin(x))/(const_c+x*x);
        };
        auto pr=gl_exp.parse(vars,body.begin(),body.end(),
                             gl_iden_parser,num_parser);
        assert(pr.is_valid());
        real_t error=accumulate_error(check,gl_exp,{0,1,0.1},{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        std::string body_0="sqrt(x*x+y*y+0.1)";
        auto pr_0=gl_exp.parse({"x","y"},body_0.begin(),body_0.end(),
                                gl_iden_parser,num_parser);
        assert(pr_0.is_valid());
        assert(gl_iden_parser.m_expression.insert({"piph",&gl_exp}).second);

        std::string body_1="sin(x)/piph(x,y)";
        auto pr_1=gl_exp_1.parse({"x","y"},body_1.begin(),body_1.end(),
                                 gl_iden_parser,num_parser);
        assert(pr_1.is_valid());
        assert(gl_iden_parser.m_expression.insert({"f1",&gl_exp_1}).second);

        std::string body_2="cos(y)/piph(x,y)";
        auto pr_2=gl_exp_2.parse({"x","y"},body_2.begin(),body_2.end(),
                                 gl_iden_parser,num_parser);
        assert(pr_2.is_valid());
        assert(gl_iden_parser.m_expression.insert({"f2",&gl_exp_2}).second);

        std::string body_res="f1(x,y)+f2(x,y)";
        auto pr_res=gl_exp_res.parse({"x","y"},body_res.begin(),body_res.end(),
                                     gl_iden_parser,num_parser);
        assert(pr_res.is_valid());

        auto check=[](real_t x,real_t y)
        {
            return (sin(x)+cos(y))/sqrt(x*x+y*y+0.1);
        };
        real_t error=accumulate_error(check,gl_exp_res,{0,1,0.2},{0,1,0.2});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    std::cout<<"test parsing\n";
}

















