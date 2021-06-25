#include <assert.h>
#include "test_data_pool.h"
#include "../data_pool.h"


#include  "test_common.h"
#include  <algorithm>
#include  <limits>


bool real_eq(real_t a,real_t b)
{
    static const real_t eps_10=std::numeric_limits<real_t>::epsilon()*10;
    return abs(a-b)<eps_10;
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
        res=std::max(abs(f1(i)-f2(i)),res);
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
            res=std::max(abs(f1(i,j)-f2(i,j)),res);
        }
    }
    return res;
}

static CDataPool gl_pool;
const real_t const_a=1.4;
const real_t const_b=2.1;
const real_t const_c=6.9;


void test_data_pool()
{
    using function_t=CFunctionPool::function_t;
    assert(gl_pool.RegisterConstant("const_a",const_a));
    assert(gl_pool.RegisterConstant("const_b",const_b));
    assert(gl_pool.RegisterConstant("const_c",const_c));
    {
        auto check=[](real_t x){return 1+2*x;};
        auto we=gl_pool.CreateFunction({"x"},"1+2*x");
        assert(we);
        real_t error=accumulate_error(check,we,{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        auto check=[](real_t x){return (1+2)*x;};
        auto we=gl_pool.CreateFunction({"x"},"(1+2)*x");
        assert(we);
        real_t error=accumulate_error(check,we,{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        auto check=[](real_t x,real_t y){return (y+2)*x;};
        auto we=gl_pool.CreateFunction({"x","y"},"(y+2)*x");
        assert(we);
        real_t error=accumulate_error(check,we,{0,1,0.1},{0,1,0.1});
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
            return (exp(1.567*y)+2*y)*x+cos(x-y*sin(x))/(1.8+x*x);
        };
        auto we=gl_pool.CreateFunction({"x","y"},
        "(exp(1.567*y)+2*y)*x+cos(x-y*sin(x))/(1.8+x*x)");
        assert(we);
        real_t error=accumulate_error(check,we,{0,1,0.1},{0,1,0.1});
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
            return (exp(const_b*y)+const_a*y)*x+
                    cos(x-y*sin(x))/(const_c+x*x);
        };
        auto we=gl_pool.CreateFunction({"x","y"},
                "(exp(const_b*y)+const_a*y)*x+ \
                cos(x-y*sin(x))/(const_c+x*x)");
        assert(we);
        real_t error=accumulate_error(check,we,{0,1,0.1},{0,1,0.1});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        assert(gl_pool.RegisterFunction
                        ("piph",{"x","y"},"sqrt(x*x+y*y+0.1)"));

        assert(gl_pool.RegisterFunction
                        ("f1",{"x","y"},"sin(x)/piph(x,y)"));

        assert(gl_pool.RegisterFunction
                        ("f2",{"x","y"},"cos(y)/piph(x,y)"));

        auto we=gl_pool.RegisterFunction("main",{"x","y"},"f1(x,y)+f2(x,y)");
        assert(we);

        auto check=[](real_t x,real_t y)->real_t
        {
            return (sin(x)+cos(y))/sqrt(x*x+y*y+0.1);
        };
        real_t error=accumulate_error(check,we,{0,1,0.2},{0,1,0.2});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
        gl_pool.SetDependencyContext(gl_pool.FindFunction("piph"));
        assert(gl_pool.GetContext().size()==3);
        auto f=const_cast<function_t*>(gl_pool.FindFunction("main"));
        assert(f);
        auto pr=gl_pool.ReparseFunction(*f,{"x","y"},"f1(x,y)+f2(x,y)+const_a+const_b");
        assert(pr.is_valid());
        auto check2=[check](real_t x,real_t y)->real_t
        {
            return check(x,y)+const_a+const_b;
        };
        error=accumulate_error(check2,f->expression(),{0,1,0.2},{0,1,0.2});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    {
        gl_pool.Clear();
        assert(gl_pool.Constants().Empty());
        assert(gl_pool.Functions().Empty());
        assert(gl_pool.RegisterFunction("f1",{"x"},"x"));
        assert(gl_pool.RegisterFunction("f2",{"x"},"f1(x)"));
        auto ev=gl_pool.RegisterFunction("f3",{"x"},"f2(x)");
        assert(ev);
        auto error=accumulate_error([](auto x){return x;},ev,{0,1,0.2});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
        auto f1=gl_pool.Functions().Find(std::string("f1"));
        assert(f1);
        gl_pool.SetDependencyContext(f1);
        assert(gl_pool.GetContext().size()==2);
        gl_pool.DeleteDependencyContext();
        assert(gl_pool.Functions().Empty());
    }
    {
        gl_pool.Clear();
        auto _x=gl_pool.CreateFunction({"s","t"},"s");
        assert(_x);
        auto _y=gl_pool.CreateFunction({"s","t"},"t");
        assert(_y);
        auto __z=gl_pool.CreateFunction({"s","t"},"s+t");
        auto _z=__z;
        assert(_z);
        auto check=[](real_t a,real_t b){return a+b;};
        auto error=accumulate_error(check,_z,{0,1,0.2},{0,1,0.2});
        TEST(real_eq(error,0));
        if(!real_eq(error,0))
        {
            std::cout<<"error:"<<error<<'\n';
            return;
        }
    }
    assert(gl_pool.Constants().Empty());
    assert(gl_pool.Functions().Empty());
    std::cout<<"test data pool\n";
}

















