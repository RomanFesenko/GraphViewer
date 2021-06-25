#include <iostream>
#include <sstream>
#include <iomanip>
#include <ranges>
#include <algorithm>

#include "plot_2D_base.h"
#include "rect_mesh.h"
#include "transformation.h"

#pragma GCC diagnostic warning "-Wsign-conversion"
#pragma GCC diagnostic warning "-Wconversion"

namespace rgs=std::ranges;

float CDrawer2D::viewport_t::width()const
{
  return m_top_right[0]-m_bottom_left[0];
}

// axis Y may be direct to down
float CDrawer2D::viewport_t::height()const
{
  return m_top_right[1]-m_bottom_left[1];
}

CDrawer2D::point_t
CDrawer2D::viewport_t::top_left()const
{
  return point_t(m_bottom_left[0],m_top_right[1]);
}

CDrawer2D::point_t
CDrawer2D::viewport_t::bottom_right()const
{
  return point_t(m_top_right[0],m_bottom_left[1]);
}

float CDrawer2D::viewport_t::sign_x()const
{
  return (width()>=0)? 1.f:-1.f;
}

float CDrawer2D::viewport_t::sign_y()const
{
  return (height()>=0)? 1.f:-1.f;
}

void CPlot2D::plot_t::m_Fill(float t)
{
  assert(m_num_points>=2);
  m_points.clear();
  float delta=(m_param_range.second-m_param_range.first)/
              (m_num_points-1);
  m_box.setEmpty();
  for(int i=0;i<m_num_points;++i)
  {
      float param=m_param_range.first+delta*i;
      m_points.push_back(m_plot_functor(param,t));
      m_box.extend(m_points.back());
  }
  m_last_update=t;
}

//Set default params
CPlot2D::CPlot2D():
m_num_hor_grid(4),m_num_vert_grid(4),
m_grid_color(0.25f,0.25f,0.f),
m_back_color(0.1f,0.1f,0.1f),
m_scale_color(1.f,1.f,1.f),
m_graphs_color({{1,0,0},{0,1,0},{0,0,1},{1,1,1}})
{
}

//Update view zone after added plot

void CPlot2D::m_UpdateBoundary()
{
  if(Empty()) return;
  m_box=m_plots[0].m_box;
  m_box.extend(m_plots.back().m_box);
  for(auto&plot:m_plots)
  {
      if(plot.m_is_cartesian)
      {
          plot.m_param_range={m_box.min()[0],m_box.max()[0]};
          plot.m_Fill(0);
          m_box.extend(plot.m_box);
      }
  }
  m_UpdateStrings();
  //std::cout<<m_box.min()<<","<<m_box.max()<<'\n';
}

//Forced change view zone

void CPlot2D::ResetBox(const box_t&b)
{
  m_box=b;
  for(auto&plot:m_plots)
  {
      if(plot.m_is_cartesian)
      {
          plot.m_param_range={m_box.min()[0],m_box.max()[0]};
          plot.m_Fill(0);
      }
  }
  m_UpdateStrings();
}

void CPlot2D::ErasePlot(int i)
{
  assert(i<m_plots.size());
  m_plots.erase(m_plots.begin()+i);
}


CPlot2D::plot_t&CPlot2D::Plot(int i)
{
  assert(i<m_plots.size());
  return m_plots[i];
}

void CPlot2D::UpdateForTime(float time)
{
  for(auto&plot:m_plots)
  {
      if(plot.m_last_update==time) continue;
      plot.m_Fill(time);
  }
}

//Convert widgets coords to users coords

std::optional<CPlot2D::point_t>
CPlot2D::WorldCoords(int x,int y,CDrawer2D&drawer)const
{
  if(Empty()) return {};
  auto box=m_GetGraphRect(drawer,drawer_id);
  Eigen::Matrix3f mx;
  ScaleMx2D(mx,box.first,box.second,
               m_box.corner(box_t::BottomLeft),
               m_box.corner(box_t::TopRight));
  point_t res;
  transform_2D(res,mx,point_t(x,y));
  if(!m_box.contains(res)) return {};
  return res;
}

void CPlot2D::Draw(CDrawer2D&drawer)const
{
  //fill all used domain
  auto  vp=drawer.AvailableViewport();
  drawer.SetMatrix(Eigen::Matrix3f::Identity());
  drawer.SetColor(m_back_color);
  drawer.FillBackground(box_t(vp.m_bottom_left,vp.m_top_right));
  if(Empty()) return;
  // set transform matrix:
  auto rect=m_GetGraphRect(drawer,drawer_id);
  Eigen::Matrix3f mx;
  ScaleMx2D(mx,m_box.corner(box_t::BottomLeft),
               m_box.corner(box_t::TopRight),
               rect.first,rect.second);
  drawer.SetMatrix(mx);
  drawer.DrawLineBegin();
  if(m_num_hor_grid||m_num_vert_grid)
  {
      drawer.SetColor(m_grid_color);
      rect_mesh_2d_t rm(m_box,m_num_hor_grid+1,
                              m_num_vert_grid+1);
      for(int i=0;i<=rm.rows();++i)
      {
          //hor lines
          drawer.DrawLine(rm.point(i,0),rm.point(i,rm.cols()));
      }
      for(int i=0;i<=rm.cols();++i)
      {
          // vert lines
          drawer.DrawLine(rm.point(0,i),rm.point(rm.rows(),i));
      }
  }
  for(int j;const auto&plot:m_plots)
  {
      drawer.SetColor(m_graphs_color[j%m_graphs_color.size()]);
      j++;
      for(int i=0;i<plot.m_points.size()-1;++i)
          drawer.DrawLine(plot.m_points[i],plot.m_points[i+1]);
  }
  drawer.DrawEnd();
  //for Accomodate mode
  drawer.SetColor(m_scale_color);
  m_PrintValues(m_GetGraphRect(drawer,pixel_id),drawer);
}

void CPlot2D::Clear()
{
  m_plots.clear();
}

void CPlot2D::m_UpdateStrings()
{
  m_x_values.clear();
  m_y_values.clear();
  auto to_str=[](float f)
  {
      std::ostringstream ss;
      ss<<std::setprecision(3)<<f;
      std::string res=ss.str();
      ss<<std::scientific<<f;
      std::string res2=ss.str();
      return (res.size()<res2.size())? res:res2;
  };
  float temp=m_box.corner(box_t::BottomLeft)[0];
  float delta=m_box.sizes()[0]/(m_num_vert_grid+1);
  for(int i=0;i<=m_num_vert_grid+1;++i)
  {
      m_x_values.push_back(to_str(temp));
      temp+=delta;
  }
  temp=m_box.corner(box_t::BottomLeft)[1];
  delta=m_box.sizes()[1]/(m_num_hor_grid+1);
  for(int i=0;i<=m_num_hor_grid+1;++i)
  {
      m_y_values.push_back(to_str(temp));
      temp+=delta;
  }
}

// Plot rectangle in different coordinates
// and scaling

std::pair<CPlot2D::point_t,CPlot2D::point_t>
CPlot2D::m_GetGraphRect(CDrawer2D&dr,coord_t type)const
{
  auto vp=dr.AvailableViewport();
  if(m_scaling==accomodate_id)
  {
      switch(type)
      {
          case pixel_id:
          return {{IndentionWidth(dr,left_id),
                   vp.m_height_in_pixels-IndentionWidth(dr,bottom_id)},
                  {vp.m_width_in_pixels-IndentionWidth(dr,right_id),
                   IndentionWidth(dr,top_id)
                  }};

          case drawer_id:{
          float k_w=vp.width()/vp.m_width_in_pixels;
          float k_h=vp.height()/vp.m_height_in_pixels;
          point_t  bl=vp.m_bottom_left+point_t(IndentionWidth(dr,left_id)*k_w,
                                               IndentionWidth(dr,bottom_id)*k_h);
          point_t  tr=vp.m_top_right-point_t(IndentionWidth(dr,right_id)*k_w,
                                             IndentionWidth(dr,top_id)*k_h);
          return {bl,tr};
          }

          case world_id:
          return {m_box.corner(box_t::BottomLeft),
                  m_box.corner(box_t::TopRight)};

          default:assert(false);
      }
  }
  else if(m_scaling==save_ratio_id)
  {
    if(type==world_id) return {m_box.corner(box_t::BottomLeft),
                               m_box.corner(box_t::TopRight)};

    int h1=vp.m_height_in_pixels-IndentionWidth(dr,top_id)-
           IndentionWidth(dr,bottom_id);
    int w1=vp.m_width_in_pixels-IndentionWidth(dr,left_id)-
           IndentionWidth(dr,right_id);
    assert(h1>0);
    assert(w1>0);
    float k=std::min(h1/m_box.sizes()[1],w1/m_box.sizes()[0]);
    float h2=k*m_box.sizes()[1];
    float w2=k*m_box.sizes()[0];
    float ind_left=IndentionWidth(dr,left_id);
    float ind_top=IndentionWidth(dr,top_id);
    point_t pix_bl={ind_left,ind_top+h2};
    point_t pix_tr={ind_left+w2,ind_top};
    if(type==pixel_id) return {pix_bl,pix_tr};

    assert(type==drawer_id);
    Eigen::Matrix3f mx;
    ScaleMx2D(mx,point_t(0,0),point_t(vp.m_width_in_pixels,vp.m_height_in_pixels),
                 vp.top_left(),vp.bottom_right());
    point_t drawer_bl,drawer_tr;
    transform_2D(drawer_bl,mx,pix_bl);
    transform_2D(drawer_tr,mx,pix_tr);
    return {drawer_bl,drawer_tr};
  }
  else {assert(false);}
}

//The amount of indention from the edges of the widget in pixels
// for digits

int CPlot2D::IndentionWidth(const CDrawer2D&dr,side_t s)const
{
  auto proj_h=[&](const auto&s){return dr.StringHeight(s);};
  auto proj_w=[&](const auto&s){return dr.StringWidth(s);};
  int res;
  switch(s)
  {
    case left_id:
    res= proj_w(*rgs::max_element(m_y_values,{},proj_w))+5;break;
    case right_id:
    res= 8;break;
    case bottom_id:
    res= proj_h(*rgs::max_element(m_x_values,{},proj_h))+5;break;
    case top_id:
    res= dr.StringHeight("0E")/2;break;
    default:assert(false);
  }
  assert(res>0);
  return res;
}

//Digits

void CPlot2D::m_PrintValues(std::pair<point_t,point_t> bx,//pixels
                            CDrawer2D&dr)const
{
  assert(m_x_values.size()==m_num_vert_grid+2);
  assert(m_y_values.size()==m_num_hor_grid+2);
  //abscissa axis
  const int y_pos_abcs=bx.first[1]+IndentionWidth(dr,bottom_id)-3;
  int x_pos=bx.first[0];
  int x_delta=(bx.second[0]-bx.first[0])/(m_num_vert_grid+1);
  assert(x_delta>0);
  for(const auto&val:m_x_values)
  {
      int str_width=dr.StringWidth(val);
      dr.StringOut(val,x_pos-str_width/2,y_pos_abcs);
      x_pos+=x_delta;
  }
  //ordinate axis
  int y_pos_ord=bx.first[1]+dr.StringWidth("0")/2;
  int y_delta=(bx.second[1]-bx.first[1])/(m_num_hor_grid+1);
  assert(y_delta<0);
  for(const auto&val:m_y_values)
  {
      dr.StringOut(val,2,y_pos_ord);
      y_pos_ord+=y_delta;
  }
}

void CPlot2D::SetGridLines(int vert,int hor)
{
  m_num_vert_grid=vert;
  m_num_hor_grid=hor;
  m_UpdateStrings();
}




