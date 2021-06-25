#include <iostream>
#include <assert.h>
#include "parse_config.h"
#include "test/test_dependency_graph.h"
#include "test/test_parsing.h"
#include "test/test_data_pool.h"


#include <ctime>

using namespace std;

int main()
{
    test_dependency_graph();
    test_parsing();
    test_data_pool();
    return 0;
}
