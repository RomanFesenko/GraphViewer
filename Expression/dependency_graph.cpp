#include <assert.h>
#include "dependency_graph.h"

#include  <list>
#include  <iostream>
#include  <utility>
#include  <algorithm>
namespace rg=std::ranges;

//CBaseNodeData

void CBaseNodeData::AddParent(const CBaseNodeData*parent)const
{
    assert(IsLinked());
    assert(parent->IsLinked());
    m_Node()->m_AddParent(parent->m_Node());
}
void CBaseNodeData::AddChild(const CBaseNodeData*child)const
{
    assert(IsLinked());
    assert(child->IsLinked());
    m_Node()->m_AddChild(child->m_Node());
}

void CBaseNodeData::DetachParents()const
{
    assert(IsLinked());
    m_Node()->m_DetachParents();
}

//CNode

void CNode::m_AddParent(CNode*parent)
{
    m_parents.push_back(parent);
    parent->m_childs.push_back(this);
}

void CNode::m_AddChild(CNode*child)
{
    child->m_AddParent(this);
}

void CNode::m_DetachParents()
{
    for(auto par:m_parents)
    {
        auto iter=rg::find(par->m_childs,this);
        assert(iter!=par->m_childs.end());
        par->m_childs.erase(iter);
    }
    m_parents.clear();
}

CNode::~CNode()
{
    assert(m_childs.empty());
    assert(*m_this==this);
    m_DetachParents();
    m_nodes.erase(m_this);
}

//CDependencyGraph


bool CDependencyGraph::Empty()const
{
    return m_nodes.empty();
}

std::size_t CDependencyGraph::Size()const
{
    return m_nodes.size();
}

void CDependencyGraph::AddNode(const CBaseNodeData*data)
{
    assert(!data->IsLinked());
    CNode* new_node=new CNode(m_nodes);
    data->m_SetNode(new_node);
    m_nodes.push_back(new_node);
    new_node->m_this=--m_nodes.end();
    new_node->m_data=data->IsCopyable()? nullptr:data;
}

void CDependencyGraph::AllChilds(const CBaseNodeData*start)const
{
    assert(start&&start->IsLinked());
    for(const CNode*node:m_nodes)
    {
        node->m_state=0;
    }
    start->m_Node()->m_state=1;
    m_targets={start};
    m_targets.reserve(Size());//!
    for(int i=0;i<m_targets.size();++i)
    {
        if(!m_targets[i])continue;
        for(CNode*child:m_targets[i]->m_Node()->m_childs)
        {
            if(child->m_state==0)
            {
                child->m_state=1;
                m_targets.push_back(child->m_data);
            }
        }
    }
    m_last_target=childs_id;
    //first elements in m_nodes - more dependent
    rg::reverse(m_targets);
}

void CDependencyGraph::AllParents(const CBaseNodeData*start)const
{
    assert(start->IsLinked());
    for(const CNode*node:m_nodes)
    {
        node->m_state=0;
    }
    start->m_Node()->m_state=1;
    m_targets={start};
    m_targets.reserve(Size());//!
    for(int i=0;i<m_targets.size();++i)
    {
        assert(m_targets[i]);
        for(CNode*parent:m_targets[i]->m_Node()->m_parents)
        {
            if(parent->m_state==0)
            {
                parent->m_state=1;
                m_targets.push_back(parent->m_data);
            }
        }
    }
    m_last_target=parents_id;
}


void CDependencyGraph::TopologicalSort()const
{
    for(const CNode*node:m_nodes)
    {
        node->m_state=0;
    }
    m_targets.clear();
    auto depth_search=[this](const CNode*from)
    {
        using iterator_t=std::vector<CNode*>::const_iterator;
        using pair_t=std::pair<const CNode*,iterator_t>;
        std::vector<pair_t> stack;
        stack.push_back({from,from->m_childs.begin()});
        while(!stack.empty())
        {
            auto iter=stack.back().second;
            const CNode* node=stack.back().first;
            if(iter==node->m_childs.end())
            {
                m_targets.push_back(node->m_data);
                node->m_state=1;
                stack.pop_back();
            }
            else if((*iter)->m_state!=0)
            {
                stack.back().second++;
            }
            else
            {
                stack.push_back({*iter,(*iter)->m_childs.begin()});
            }
        }
    };
    for(const CNode*node:m_nodes)
    {
        if(node->m_state!=0) continue;
        depth_search(node);
    }
    m_last_target=sort_id;
}

bool CDependencyGraph::IsInTargets(const CBaseNodeData*data)const
{
    for(const CNode*node:m_nodes)
    {
        if(node->m_data==data)
        {
            return node->m_state!=0;
        }
    }
    return false;
}












