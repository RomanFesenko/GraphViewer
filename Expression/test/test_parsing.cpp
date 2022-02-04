#include <assert.h>
#include  <algorithm>
#include  <limits>
#include  <cmath>

#include "test_parsing.h"
#include "../expression_parser.h"


#include  "test_common.h"



using real_t=double;
static bool real_eq(real_t a,real_t b)
{
    static const real_t eps_10=std::numeric_limits<real_t>::epsilon()*10;
    return std::abs(a-b)<eps_10;
}

// floating point functions

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
        res+=std::abs(f1(i)-f2(i));
    }
    return res;
}

template<class F1,class F2>
real_t accumulate_error(const F1&f1,const F2&f2,range_t rg1,range_t rg2)// for any more ranges?..
{
    real_t res=0;
    for(real_t i=rg1.min;i<rg1.max;i+=rg1.inc)
    {
        for(real_t j=rg2.min;j<rg2.max;j+=rg2.inc)
        {
            res+=std::abs(f1(i,j)-f2(i,j));
        }
    }
    return res;
}

template<class F1,class F2>
real_t accumulate_error(const F1&f1,const F2&f2,range_t rg1,range_t rg2,range_t rg3)// for any more ranges?..
{
    real_t res=0;
    for(real_t i=rg1.min;i<rg1.max;i+=rg1.inc)
    {
        for(real_t j=rg2.min;j<rg2.max;j+=rg2.inc)
        {
            for(real_t k=rg3.min;k<rg3.max;k+=rg3.inc)
            {
                res=std::max(res,std::abs(f1(i,j,k)-f2(i,j,k)));
            }
        }
    }
    return res;
}

static void test_string_util()
{
    using namespace sutil;
    const char* test_string[]={"_test","tested","test","some test","some test string"};
    auto iden="test";
    assert(find_identifier(test_string[0],iden)==sutil::str_end(test_string[0]));
    assert(find_identifier(test_string[1],iden)==sutil::str_end(test_string[1]));
    assert(find_identifier(test_string[2],iden)-sutil::str_begin(test_string[2])==0);
    assert(find_identifier(test_string[3],iden)-sutil::str_begin(test_string[3])==5);
    assert(find_identifier(test_string[4],iden)-sutil::str_begin(test_string[4])==5);
    std::cout<<"test string util\n";
}

void test_parsing()
{
    using namespace expr;
    test_string_util();

    using function=expr::function<real_t>;
    using fptr_t=real_t(*)(real_t);
    const unsigned op_flag=expr::float_arithmetics_fl;
    const real_t const_a=1.4;
    const real_t const_b=2.1;
    const real_t const_c=6.9;
    auto const_parser=expr::make_constants_parser<real_t>({"const_a","const_b","const_c"},{const_a,const_b,const_c});
    auto funct_parser=expr::make_functions_parser<fptr_t,real_t>({"sin","cos","exp"},{ sin,cos,exp});
    auto iden_parser=expr::concat_parsers(std::ref(const_parser),std::ref(funct_parser));
    function func;
    {
        func=[](real_t r){return sin(r);};
        real_t error=accumulate_error(sin,func,{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        auto check=[](real_t x){return 1+2*x;};
        auto err=func.parse({"x"},"1+2*x",op_flag);
        assert(!err);
        real_t error=accumulate_error(check,func,{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        auto check=[](real_t x,real_t y){return (y+2)*x;};
        auto err=func.parse({"x","y"},"(y+2)*x",op_flag);
        assert(!err);
        real_t error=accumulate_error(check,func,{0,1,0.1},{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        auto check=[](real_t s,real_t t,real_t time)
        {
            return  1/ (0.3 + s) * cos (4.00 * time-4.0 * s);
        };
        auto err=func.parse({"s","t","time"},"1/(0.3+s)*cos(4.0*time-4.0*s)",op_flag,funct_parser);
        assert(!err);
        real_t error=accumulate_error(check,func,{0,5,0.5},{0,3,0.1},{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        auto check=[](real_t x,real_t y)
        {
            return (exp(1.567*y)+2*y)*x+cos(x-y*sin(x))/(1.8+x*x);
        };
        auto err=func.parse({"x","y"},"(exp(1.567*y)+2*y)*x+cos(x-y*sin(x))/(1.8+x*x)",op_flag,funct_parser);
        assert(!err);
        real_t error=accumulate_error(check,func,{0,1,0.1},{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        auto check=[=](real_t x,real_t y)
        {
            return (exp(const_b*y)+const_a*y)*x+cos(x-y*sin(x))/(const_c+x*x);
        };
        auto perr=func.parse({"x","y"},"(exp(const_b*y)+const_a*y)*x+cos(x-y*sin(x))/(const_c+x*x)",
                             op_flag,
                             iden_parser);
        assert(!perr);
        real_t error=accumulate_error(check,func,{0,1,0.1},{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        auto perr=func.parse({"x"},"xx",op_flag,iden_parser);
        assert(perr.type()==parse_error_t::unknown_identifier_id);
        perr=func.parse({"x"},"x+",op_flag,iden_parser);
        assert(perr.type()==parse_error_t::syntax_error_id);
        perr=func.parse({"x"},"(x+1",op_flag,iden_parser);
        assert(perr.type()==parse_error_t::parenthesis_error_id);
    }
    std::cout<<"test parsing\n";
}

















