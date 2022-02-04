#include <assert.h>
#include "test_dependency_graph.h"
#include "../dependency_graph.h"

#include  <iostream>
#include  <algorithm>
#include  <iterator>



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




void test_dependency_graph()
{
    using dag_t=dag<int>;
    using desc_t=dag_t::node_descriptor;
    auto  desc_comp=[](desc_t _1,desc_t _2){return _1.data()<_2.data();};
    auto  desc_equal=[](desc_t _1,desc_t _2){return _1.data()==_2.data();};
    auto equal_set=[desc_comp,desc_equal]( std::vector<desc_t> v_1, std::vector<desc_t> v_2)
    {
        if(v_1.size()!=v_2.size()) return false;
        std::sort(v_1.begin(),v_1.end(),desc_comp);
        std::sort(v_2.begin(),v_2.end(),desc_comp);
        return std::equal(v_1.begin(),v_1.end(),v_2.begin(),v_2.end(),desc_equal);
    };
    dag_t dg;
    auto test_parents=[&dg,equal_set](desc_t root,const std::vector<desc_t>&check)
    {
        std::vector<desc_t> pars;
        dg.traverse_parents(root,pars);
        return equal_set(check,pars);
    };
    auto test_childs=[&dg,equal_set](desc_t root,const std::vector<desc_t>&check)
    {
        std::vector<desc_t> pars;
        dg.traverse_childs(root,pars);
        return equal_set(check,pars);
    };

    {
        assert(dg.empty());
        desc_t descs[3];
        //0->1->2
        for(int i=0;i<3;++i)
        {
            descs[i]=dg.add_node(i);
            assert(descs[i]);
        }
        dg.set_link(descs[0],descs[1]);
        dg.set_link(descs[1],descs[2]);

        TEST(test_parents(descs[0],{}));
        TEST(test_parents(descs[1],{descs[0]}));
        TEST(test_parents(descs[2],{descs[0],descs[1]}));
        TEST(test_childs(descs[0],{descs[1],descs[2]}));
        TEST(test_childs(descs[1],{descs[2]}));
        TEST(test_childs(descs[2],{}));
        dg.clear();
    }
    {
        assert(dg.empty());
        bool is_link;
        desc_t descs[4];
        //0->1,0->2,1->3,2->3
        for(int i=0;i<4;++i)
        {
            descs[i]=dg.add_node(i);
            assert(descs[i]);
        }
        is_link=dg.set_link(descs[0],descs[1]);assert(is_link);
        is_link=dg.set_link(descs[0],descs[2]);assert(is_link);
        is_link=dg.set_link(descs[1],descs[3]);assert(is_link);
        is_link=dg.set_link(descs[2],descs[3]);assert(is_link);


        TEST(test_parents(descs[0],{}));
        TEST(test_parents(descs[1],{descs[0]}));
        TEST(test_parents(descs[2],{descs[0]}));
        TEST(test_parents(descs[3],{descs[0],descs[1],descs[2]}));
        TEST(test_childs(descs[0],{descs[1],descs[2],descs[3]}));

        TEST(test_childs(descs[1],{descs[3]}));
        TEST(test_childs(descs[2],{descs[3]}));
        TEST(test_childs(descs[3],{}));

        assert(dg.is_path(descs[0],descs[1]));
        assert(dg.is_path(descs[0],descs[2]));
        assert(dg.is_path(descs[0],descs[3]));
        assert(dg.is_path(descs[1],descs[3]));
        assert(!dg.is_path(descs[3],descs[0]));

        descs[2].data()=1;
        std::vector<desc_t> sorted;
        dg.topological_sort(std::back_inserter(sorted));
        assert(sorted.size()==4);
        std::reverse(sorted.begin(),sorted.end());
        assert(std::is_sorted(sorted.begin(),sorted.end(),desc_comp));
        dg.clear();
    }
    {
        bool is_link;
        desc_t descs[3];
        //0->1,1->2,2->0
        for(int i=0;i<3;++i)
        {
            descs[i]=dg.add_node(i);
            assert(descs[i]);
        }
        is_link=dg.set_link(descs[0],descs[1]);assert(is_link);
        is_link=dg.set_link(descs[1],descs[2]);assert(is_link);
        assert(!dg.set_link(descs[2],descs[0]));
    }
    std::cout<<"test dependency graph complete\n";
}

















