#include <iostream>
#include <assert.h>
#include "test/test_dependency_graph.h"
#include "test/test_parsing.h"
#include "test/test_function_pool.h"
#include "test/benchmarks.h"


#include <ctime>

using namespace std;

int main()
{
    test_dependency_graph();
    test_parsing();
    test_function_pool();
    test_benchmarks();
    return 0;
}
