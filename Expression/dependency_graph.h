
#ifndef  _dependency_graph_
#define  _dependency_graph_

#include <list>
#include <vector>
#include <memory>

class CNode;
class CDependencyGraph;

class CBaseNodeData
{
    protected:
    virtual CNode* m_Node()const=0;
    virtual void m_SetNode(CNode*)const=0;
    public:
    const int m_type;
    explicit CBaseNodeData(int type):m_type(type){}
    bool IsLinked()const{return m_Node()!=nullptr;}
    void AddParent(const CBaseNodeData*other)const;
    void AddChild(const CBaseNodeData*other)const;
    void DetachParents()const;
    virtual bool IsCopyable()const{return false;}
    virtual ~CBaseNodeData(){}
    friend class CDependencyGraph;
};

class CNodeData:public CBaseNodeData
{
    mutable std::unique_ptr<CNode> m_node;
    protected:
    virtual CNode* m_Node()const override{return m_node.get();}
    virtual void m_SetNode(CNode*node)const override{m_node.reset(node);}
    void m_DetachFromGraph(){m_node=nullptr;}
    public:
    explicit CNodeData(int type):CBaseNodeData(type){}
    CNodeData(const CNodeData&)=delete;
    CNodeData(CNodeData&&)=delete;
    CNodeData&operator=(const CNodeData&)=delete;
};

class CCopyableNodeData:public CBaseNodeData
{
    mutable std::shared_ptr<CNode> m_node;
    protected:
    virtual CNode* m_Node()const override{return m_node.get();}
    virtual void m_SetNode(CNode*node)const override{m_node.reset(node);}
    public:
    explicit CCopyableNodeData(int type):CBaseNodeData(type){}
    virtual bool IsCopyable()const override{return true;}
};


class CNode
{
    std::list<CNode*>& m_nodes;
    typename std::list<CNode*>::iterator m_this;
    mutable char m_state;
    const CBaseNodeData* m_data=nullptr;
    // on which depends
    std::vector<CNode*> m_parents;
    // which depends
    std::vector<CNode*> m_childs;

    friend class CDependencyGraph;
    public:
    CNode(const CNode&)=delete;
    CNode&operator=(const CNode&)=delete;
    CNode(CNode&&)=delete;
    explicit CNode(std::list<CNode*>&nodes):
    m_nodes(nodes){}
    private:
    void m_AddParent( CNode*called);
    void m_AddChild( CNode*called);
    void m_DetachParents();
    public:
    ~CNode();
    friend class CBaseNodeData;
    friend class CDependencyGraph;
};

class CDependencyGraph
{
    public:
    enum target_t
    {
        parents_id,
        childs_id,
        sort_id,
        void_id
    };
    private:
    std::list<CNode*> m_nodes;
    mutable std::vector<const CBaseNodeData*> m_targets;
    std::vector<char> m_marked;
    mutable target_t m_last_target=void_id;

    public:
    CDependencyGraph(){}
    void AddNode(const CBaseNodeData*);
    bool Empty()const;
    std::size_t Size()const;
    void AllChilds(const CBaseNodeData*start)const;
    void AllParents(const CBaseNodeData*start)const;
    bool IsInTargets(const CBaseNodeData*val)const;
    void TopologicalSort()const;
    const std::vector<const CBaseNodeData*>& Targets()const{return m_targets;}
    target_t TargetType()const{return m_last_target;}
    ~CDependencyGraph()
    {
        assert(m_nodes.empty());
    }
};


#endif

