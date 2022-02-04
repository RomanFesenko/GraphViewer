#include <assert.h>
#include  <algorithm>
#include  <limits>
#include  <cmath>

#include "../function_pool.h"
#include  "test_common.h"
#include "test_function_pool.h"

using real_t=float;
static bool real_eq(real_t a,real_t b)
{
    static const real_t eps_10=std::numeric_limits<real_t>::epsilon()*10;
    return std::abs(a-b)<eps_10;
}

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
        res=std::max(std::abs(f1(i)-f2(i)),res);
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
            res=std::max(std::abs(f1(i,j)-f2(i,j)),res);
        }
    }
    return res;
}


void test_function_pool()
{
    using CFunction=CFunctionPool::CFunction;
    const real_t const_a=1.4;
    const real_t const_b=2.1;
    const real_t const_c=6.9;
    CFunctionPool fpool;
    CFunctionPool::parse_error_t pe;
    assert(fpool.CreateConstant("const_a",const_a));
    assert(fpool.CreateConstant("const_b",const_b));
    assert(fpool.CreateConstant("const_c",const_c));

    {
        auto check=[](real_t x){return 1+2*x;};
        auto fn=fpool.CreateFunction({"x"},"1+2*x",pe);
        if(!fn)
        {
            std::cout<<pe.what()<<","<<pe.detail()<<'\n';
            assert(false);
        }
        real_t error=accumulate_error(check,fn,{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        auto check=[](real_t x){return (1+2)*x;};
        auto fn=fpool.CreateFunction({"x"},"(1+2)*x");
        assert(fn);
        real_t error=accumulate_error(check,fn,{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        auto check=[](real_t x,real_t y){return (y+2)*x;};
        auto fn=fpool.CreateFunction({"x","y"},"(y+2)*x");
        assert(fn);
        real_t error=accumulate_error(check,fn,{0,1,0.1},{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        auto check=[](real_t x,real_t y)->real_t
        {
            return (std::exp(1.567*y)+2*y)*x+std::cos(x-y*std::sin(x))/(1.8+x*x);
        };
        auto fn=fpool.CreateFunction({"x","y"},"(exp(1.567*y)+2*y)*x+cos(x-y*sin(x))/(1.8+x*x)");


        assert(fn);
        real_t error=accumulate_error(check,fn,{0,1,0.1},{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        auto check=[=](real_t x,real_t y)->real_t
        {
            return (exp(const_b*y)+const_a*y)*x+cos(x-y*sin(x))/(const_c+x*x);
        };
        auto fn=fpool.CreateFunction({"x","y"},"(exp(const_b*y)+const_a*y)*x+cos(x-y*sin(x))/(const_c+x*x)");
        assert(fn);
        real_t error=accumulate_error(check,fn,{0,1,0.1},{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }

    {
        assert(fpool.CreateAndRegisterFunction("piph",{"x","y"},"sqrt(x*x+y*y+0.1)"));
        assert(fpool.CreateAndRegisterFunction("f1",{"x","y"},"sin(x)/piph(x,y)"));
        assert(fpool.CreateAndRegisterFunction("f2",{"x","y"},"cos(y)/piph(x,y)"));
        auto main_=fpool.CreateAndRegisterFunction("main",{"x","y"},"f1(x,y)+f2(x,y)");

        assert(main_);
        auto check=[](real_t x,real_t y)->real_t
        {
            return (sin(x)+cos(y))/sqrt(x*x+y*y+0.1);
        };
        real_t error=accumulate_error(check,main_,{0,1,0.2},{0,1,0.2});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
        auto main_2=fpool.FindFunction("main");
        assert(main_2);
        auto pr=fpool.ReparseFunction(main_2,{"x","y"},"f1(x,y)+f2(x,y)+const_a+const_b");
        assert(!pr);
        auto check2=[=](real_t x,real_t y)->real_t
        {
            return check(x,y)+const_a+const_b;
        };
        error=accumulate_error(check2,main_2,{0,1,0.2},{0,1,0.2});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        fpool.Clear();
        assert(fpool.CreateAndRegisterFunction("f1",{"x"},"x"));
        assert(fpool.CreateAndRegisterFunction("f2",{"x"},"f1(x)"));
        auto f3=fpool.CreateAndRegisterFunction("f3",{"x"},"f2(x)");
        assert(f3);
        auto error=accumulate_error([](auto x){return x;},f3,{0,1,0.2});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
        assert(fpool.FindFunction(std::string("f2")));
        assert(fpool.FindFunction(std::string("f3")));
        auto f1=fpool.FindFunction(std::string("f1"));
        assert(f1);
        std::vector<CFunction> depf;
        fpool.DependentFunctions(f1,depf);
        assert(depf.size()==2);
    }
    {
        fpool.Clear();
        auto f1=fpool.CreateAndRegisterFunction("f1",{"s","t"},"s");
        assert(f1);
        auto f2=fpool.CreateAndRegisterFunction("f2",{"s","t"},"t");
        assert(f2);
        auto f3=fpool.CreateAndRegisterFunction("f3",{"s","t"},"f1(s,t)+f2(s,t)");
        assert(f3);
        auto check=[](real_t a,real_t b){return a+b;};
        auto error=accumulate_error(check,f3,{0,1,0.2},{0,1,0.2});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
        assert(fpool.Functions()==3);
        fpool.EraseFunction(f1);
        assert(fpool.Functions()==1);
        assert(!fpool.FindFunction("f1"));
        assert(!fpool.FindFunction("f3"));
        assert(fpool.FindFunction("f2"));
        assert(f2(0,1)==1&&f2(0,2)==2&&f2(0,3)==3);
    }
    std::cout<<"test data pool\n";
}

















