
#ifndef  _tree_builder_
#define  _tree_builder_

#include <utility>
#include <vector>

#include "transformation.h"



template<class value_t,class point_t>
struct tree_param_t
{
    std::pair<point_t,point_t> m_root;
    value_t m_scale;
    value_t m_angle;
    value_t m_min_length;
};


template<class value_t,class point_t,class out_iterator_t>
void draw_tree(const tree_param_t<value_t,point_t>& tree_param, out_iterator_t iterator)
{
    using line_t=std::pair<point_t,point_t>;
    std::vector<line_t> stack;
    stack.push_back(tree_param.m_root);
    auto turn_left=TurnMx2D(point_t{value_t(0),value_t(0)},tree_param.m_angle/2);
    auto turn_right=TurnMx2D(point_t{value_t(0),value_t(0)},-tree_param.m_angle/2);
    while(!stack.empty())
    {
        line_t current=stack.back(); stack.pop_back();
        *iterator=current;
        point_t delta=(current.second-current.first)*tree_param.m_scale;
        if(distance_2D(delta)<tree_param.m_min_length) continue;
        stack.push_back({current.second,current.second+Transform(turn_left,delta)});
        stack.push_back({current.second,current.second+Transform(turn_right,delta)});
    }
}

 /*tree_param_t<float,point_t> tree_param
  {
    {point_t(0.0,0.9),point_t(0.0,0.4)},//
     0.8f,
     Mpi/4,
     0.03
  };*/

#endif

