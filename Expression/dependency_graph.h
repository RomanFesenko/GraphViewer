
#ifndef  _dependency_graph_
#define  _dependency_graph_

#include <list>
#include <vector>
#include <utility>
#include <memory>
#include <algorithm>


template<class functor_t>
class fn_output_iterator_t
{
    functor_t m_functor;
    struct proxy_t
    {
        functor_t* m_f_ptr;
        template<class rvalue_t>
        void operator=(rvalue_t&&rval){(*m_f_ptr)(std::forward<rvalue_t>(rval));}
    };
    public:
    fn_output_iterator_t(functor_t f):m_functor(f){}
    proxy_t operator*(){return proxy_t(&m_functor);}
    fn_output_iterator_t& operator++(int)const{return *this;}
    fn_output_iterator_t& operator++()const{return *this;}
};

struct empty_data_t{};

template<class node_data_t=empty_data_t>
class dag
{
    struct node_t
    {
        using index_t=typename std::vector<node_t*>::size_type;
        index_t             m_index=-1;
        std::vector<node_t*>m_childs;
        std::vector<node_t*>m_parents;
        node_data_t         m_data;
        node_t(const node_data_t&data):m_data(data){}
        node_t(node_data_t&&data):m_data(std::move(data)){}
    };
    struct adj_iterator_t
    {
        using edge_t=typename std::vector<node_t*>::iterator;
        node_t* source;
        edge_t  edge;
        edge_t  end;
        public:
        adj_iterator_t(node_t* s,std::vector<node_t*>&n):
        source(s),edge(n.begin()),end(n.end()){}
        bool at_end()const{return edge==end;}
        void inc(){++edge;}
    };
    inline static const char sleep=0,alive=1;

    std::vector<node_t*>        m_nodes;
    mutable std::vector<char>   m_colors;
    mutable std::vector<adj_iterator_t> m_stack;
    mutable std::vector<node_t*>m_cache;

    void m_fill_colors(char c)const
    {
        m_colors.resize(size());
        std::fill(m_colors.begin(),m_colors.end(),c);
    }

    template<class out_iterator_t>
    void m_topological_sort(node_t* from,out_iterator_t iter)const
    {
        m_colors[from->m_index]=alive;
        m_stack.push_back({from,from->m_childs});
        while(!m_stack.empty())
        {
            if(m_stack.back().at_end())
            {
                *iter=node_descriptor(m_stack.back().source);
                m_stack.pop_back();
            }
            else
            {
                node_t*target=*m_stack.back().edge;
                if(m_colors[target->m_index]==sleep)
                {
                    m_colors[target->m_index]=alive;
                    m_stack.back().inc();
                    m_stack.push_back({target,target->m_childs});
                }
                else
                {
                    m_stack.back().inc();
                }
            }
        }
    }

    public:
    class node_descriptor
    {
        node_t* m_node=nullptr;
        node_descriptor(node_t* node):m_node(node){}
        public:
        node_descriptor(){}
        node_data_t& data()requires (!std::is_same_v<node_data_t,empty_data_t>)
        {
            return m_node->m_data;
        }
        explicit operator bool()const{return m_node!=nullptr;}
        bool operator==(node_descriptor other)const{return m_node==other.m_node;}
        bool operator!=(node_descriptor other)const{return m_node!=other.m_node;}

        friend class dag;
    };
    node_descriptor add_node(const node_data_t&data={})
    {
        node_t* new_=new  node_t(data);
        new_->m_index=m_nodes.size();
        m_nodes.push_back(new_);
        return node_descriptor(new_);
    }
    node_descriptor add_node(node_data_t&&data={})
    {
        node_t* new_=new  node_t(std::move(data));
        new_->m_index=m_nodes.size();
        m_nodes.push_back(new_);
        return node_descriptor(new_);
    }
    void remove_node(node_descriptor desc)
    {
        detach_parents(desc);
        detach_childs(desc);
        auto index=desc.m_node->m_index;
        m_nodes.back()->m_index=index;
        std::swap(m_nodes[index],m_nodes.back());
        delete m_nodes.back();
        m_nodes.pop_back();
    }
    bool is_link(node_descriptor from,node_descriptor to)const
    {
        return std::find(from.m_node->m_childs.begin(),
                         from.m_node->m_childs.end(),
                         to.m_node)!=from.m_node->m_childs.end();
    }
    bool is_path(node_descriptor from,node_descriptor to)const
    {
        m_fill_colors(sleep);
        m_colors[from.m_node->m_index]=alive;
        m_cache.reserve(size());
        m_cache.clear();
        for(auto child:from.m_node->m_childs)
        {
            m_cache.push_back(child);
            m_colors[child->m_index]=alive;
        }
        for(decltype(m_cache.size()) i=0;i<m_cache.size();++i)
        {
            if(m_cache[i]==to.m_node) return true;
            for(auto child:m_cache[i]->m_childs)
            {
                if(m_colors[child->m_index]==sleep)
                {
                    m_colors[child->m_index]=alive;
                    m_cache.push_back(child);
                }
            }
        }
        return false;
    }

    bool set_link(node_descriptor from,node_descriptor to)
    {
        // preserve double-linking or cyclic path
        if(is_link(from,to)||is_path(to,from)) return false;

        from.m_node->m_childs.push_back(to.m_node);
        to.m_node->m_parents.push_back(from.m_node);
        return true;
    }
    int  detach_parents(node_descriptor ndesc)
    {
        for(node_t* parent:ndesc.m_node->m_parents)
        {
            auto iter=std::find(parent->m_childs.begin(),parent->m_childs.end(),ndesc.m_node);
            assert(iter!=parent->m_childs.end());
            std::iter_swap(iter,parent->m_childs.end()-1);
            parent->m_childs.pop_back();
        }
        auto ret=ndesc.m_node->m_parents.size();
        ndesc.m_node->m_parents.clear();
        return ret;
    }
    int  detach_childs(node_descriptor ndesc)
    {
        for(node_t* child:ndesc.m_node->m_childs)
        {
            auto iter=std::find(child->m_parents.begin(),child->m_parents.end(),ndesc.m_node);
            assert(iter!=child->m_parents.end());
            std::iter_swap(iter,child->m_parents.end()-1);
            child->m_parents.pop_back();
        }
        auto ret=ndesc.m_node->m_childs.size();
        ndesc.m_node->m_childs.clear();
        return ret;
    }
    void traverse_childs(node_descriptor from,std::vector<node_descriptor>&childs)const
    {
        m_fill_colors(sleep);
        m_colors[from.m_node->m_index]=alive;
        childs.clear();
        for(auto child:from.m_node->m_childs)
        {
            childs.push_back(node_descriptor(child));
            m_colors[child->m_index]=alive;
        }
        for(decltype(childs.size()) i=0;i<childs.size();++i)
        {
            for(auto child:childs[i].m_node->m_childs)
            {
                if(m_colors[child->m_index]==sleep)
                {
                    m_colors[child->m_index]=alive;
                    childs.push_back(node_descriptor(child));
                }
            }
        }
    }
    void traverse_parents(node_descriptor from,std::vector<node_descriptor>&parents)const
    {
        m_fill_colors(sleep);
        m_colors[from.m_node->m_index]=alive;
        parents.clear();
        for(auto parent:from.m_node->m_parents)
        {
            parents.push_back(node_descriptor(parent));
            m_colors[parent->m_index]=alive;
        }
        for(decltype(parents.size()) i=0;i<parents.size();++i)
        {
            for(auto parent:parents[i].m_node->m_parents)
            {
                if(m_colors[parent->m_index]==sleep)
                {
                    m_colors[parent->m_index]=alive;
                    parents.push_back(node_descriptor(parent));
                }
            }
        }
    }

    template<class out_iterator_t>
    void topological_sort(node_descriptor from,out_iterator_t iter)const
    {
        m_fill_colors(sleep);
        m_topological_sort(from.m_node,iter);
    }
    template<class out_iterator_t>
    void topological_sort_except_root(node_descriptor from,out_iterator_t iter)const
    {
        m_fill_colors(sleep);
        for(auto child:from.m_node->m_childs)
        {
            m_topological_sort(child,iter);
        }
    }

    template<class out_iterator_t>
    void topological_sort(out_iterator_t iter)const
    {
        m_fill_colors(sleep);
        for(decltype(size()) i=0;i<size();++i)
        {
            if(m_colors[i]==sleep)
            {
                m_topological_sort(m_nodes[i],iter);
            }
        }
    }
    void clear()
    {
        for(auto node:m_nodes) delete node;
        m_nodes.clear();
    }
    bool empty()const{return m_nodes.empty();}
    auto size()const{return m_nodes.size();}
    ~dag(){clear();}
};

#endif

