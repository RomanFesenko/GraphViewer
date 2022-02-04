#include <assert.h>
#include  <algorithm>
#include  <limits>

#include  "../../Timing/timing.h"
#include "../expression_parser.h"
#include  "test_common.h"
#include "benchmarks.h"

using real_t=double;

struct range_t
{
    real_t min;
    real_t max;
    real_t inc;
};

template<class f_t>
double max(const f_t&f,range_t rg1,range_t rg2,range_t rg3)
{
    double res=f(rg1.min,rg2.min,rg3.min);
    for(double i=rg1.min;i<rg1.max;i+=rg1.inc)
    {
        for(double j=rg2.min;j<rg2.max;j+=rg2.inc)
        {
            for(double k=rg3.min;k<rg3.max;k+=rg3.inc)
            {
                double temp=f(i,j,k);
                if(temp>res) res=temp;
            }
        }
    }
    return res;
}


void test_benchmarks()
{
    expr::function<real_t> func;
    CTimer timer;
    auto native=[](double x,double y,double z)
    {
        return (x*y+x*z+y*z)/(x*x+y*y+z*z)+(x+y+z)/(x*y*z+1)-(x+y+3)*(z*x+5);
    };
    range_t r1={0.1,10,0.01};range_t r2={0.1,10,0.1};range_t r3={0.1,10,0.1};;

    timer.Restart();
    double v1=max(native,r1,r2,r3);
    timer.Stop();
    std::cout<<"Native:"<<timer.Pass<>()<<'\n';

    auto pr_error=func.parse({"x","y","z"},"(x*y+x*z+y*z)/(x*x+y*y+z*z)+(x+y+z)/(x*y*z+1)-(x+y+3)*(z*x+5)",
                             expr::float_arithmetics_fl);
    assert(!pr_error);

    timer.Restart();
    double v2=max(func,r1,r2,r3);
    timer.Stop();
    std::cout<<"Interpreter:"<<timer.Pass<>()<<'\n';

    std::cout<<"Delta:"<<v2-v1<<'\n';
}

















