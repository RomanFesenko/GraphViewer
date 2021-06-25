#include <assert.h>
#include "test_dependency_graph.h"
#include "../dependency_graph.h"

#include  <iostream>
#include  <memory>
#include  <algorithm>
#include  "../defer.h"

namespace rgs=std::ranges;

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


class CData:public CNodeData
{
    public:
    CData():CNodeData(0){}
};

void test_dependency_graph()
{

    auto equal_set=[](const std::vector<const CData*>&v_1,
                      const std::vector<const CBaseNodeData*>&v_2)
    {
        if(v_1.size()!=v_2.size()) return false;
        auto cv_1=v_1;
        auto cv_2=v_2;
        rgs::sort(cv_1);
        rgs::sort(cv_2);
        return rgs::equal(cv_1,cv_2);
    };
    CDependencyGraph dg;
    auto test_parents=[&dg,equal_set](const CData* root,const std::vector<const CData*>&v)
    {
        dg.AllParents(root);
        return equal_set(v,dg.Targets());
    };
    auto test_childs=[&dg,equal_set](const CData* root,const std::vector<const CData*>&v)
    {
        dg.AllChilds(root);
        return equal_set(v,dg.Targets());
    };

    {
        assert(dg.Empty());
        //0->1->2
        CData* datas[3];
        for(int i=0;i<3;++i)
        {
            datas[i]=new CData;
        }
        auto deleter=[&datas,&dg]()
        {
            dg.TopologicalSort();
            for(auto ptr:dg.Targets())
            {
                delete ptr;
            }
        };
        scope_guard_t sg(deleter);
        for(int i=0;i<3;++i)
        {
            dg.AddNode(datas[i]);
        }
        datas[0]->AddChild(datas[1]);
        datas[1]->AddChild(datas[2]);

        TEST(test_parents(datas[0],{datas[0]}));
        TEST(test_parents(datas[1],{datas[0],datas[1]}));
        TEST(test_parents(datas[2],{datas[0],datas[1],datas[2]}));
        TEST(test_childs(datas[0],{datas[0],datas[1],datas[2]}));
        TEST(test_childs(datas[1],{datas[2],datas[1]}));
        TEST(test_childs(datas[2],{datas[2]}));
    }
    {
        assert(dg.Empty());
        //0->1,0->2,1->3,2->3
        CData* datas[4];
        for(int i=0;i<4;++i)
        {
            datas[i]=new CData;
        }
        auto deleter=[&datas,&dg]()
        {
            dg.TopologicalSort();
            for(auto ptr:dg.Targets())
            {
                delete ptr;
            }
        };
        scope_guard_t sg(deleter);
        for(int i=0;i<4;++i)
        {
            dg.AddNode(datas[i]);
        }
        datas[0]->AddChild(datas[1]);
        datas[0]->AddChild(datas[2]);
        datas[1]->AddChild(datas[3]);
        datas[2]->AddChild(datas[3]);

        TEST(test_parents(datas[0],{datas[0]}));
        TEST(test_parents(datas[1],{datas[0],datas[1]}));
        TEST(test_parents(datas[2],{datas[0],datas[2]}));
        TEST(test_parents(datas[3],{datas[0],
                                    datas[1],
                                    datas[2],
                                    datas[3]}));

        TEST(test_childs(datas[0],{datas[0],
                                   datas[1],
                                   datas[2],
                                   datas[3]}));

        TEST(test_childs(datas[1],{datas[3],datas[1]}));
        TEST(test_childs(datas[2],{datas[3],datas[2]}));
        TEST(test_childs(datas[3],{datas[3]}));
    }
    assert(dg.Empty());
    std::cout<<"test dependency graph complete\n";
}

















