
#ifndef  _test_common_
#define  _test_common_

#include <iostream>

inline void test_fail(const char *assertion, const char *file,
			   unsigned int line)
{
    std::cout<<"Test failed in:"
             <<line
             <<",file: "<<file<<std::endl;
}


#  define TEST(expr)\
(static_cast <bool> (expr)?	\
void (0): test_fail (#expr, __FILE__, __LINE__))

#endif

