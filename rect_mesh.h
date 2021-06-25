
#ifndef  _rect_mesh_
#define  _rect_mesh_

#include <assert.h>

#include <utility>
#include <Eigen/Geometry>

class rect_mesh_2d_t
{
  using box_t=Eigen::AlignedBox<float,2>;
  using point_t=Eigen::Vector2f;
  using size_t= std::size_t;
  box_t m_box;
  size_t m_rows;
  size_t m_cols;
  float m_x_delta,m_y_delta;
  public:
  explicit rect_mesh_2d_t(const box_t&b,int r,
                          int c):
  m_box(b),m_rows(r),m_cols(c),
  m_x_delta(m_box.sizes()[0]/(m_cols)),
  m_y_delta(m_box.sizes()[1]/(m_rows))
  {
    assert(!m_box.isEmpty());
    assert(m_cols>0&&m_rows>0);
    assert(m_x_delta>0);
    assert(m_y_delta>0);
  }
  size_t rows()const{return m_rows;}
  size_t cols()const{return m_cols;}
  point_t point(size_t i,size_t j)const
  // fixed - (-i*...) - error convertion uint to int
  {
    assert(i<=m_rows&&j<=m_cols);
    return m_box.corner(box_t::TopLeft)+
           point_t(j*m_x_delta,-(i*m_y_delta));
  }
  box_t subbox(size_t i,size_t j)const
  {
    assert(i<m_rows&&j<m_cols);
    return box_t(point(i+1,j),point(i,j+1));
  }
};


template<class point_t>
class rect_mesh_3d_t
{
    using size_t= std::size_t;
    size_t m_rows;
    size_t m_columns;
    point_t m_begin;
    point_t m_s_ort;
    point_t m_t_ort;
    using value_t=std::decay<decltype(m_begin[0])>;
    public:
    rect_mesh_3d_t(const point_t&begin,
                   const point_t&snd,
                   const point_t&thrd,std::size_t num_s,std::size_t num_t):
    m_rows(num_s),
    m_columns(num_t),
    m_begin(begin),
    m_s_ort((snd-begin)/num_s),
    m_t_ort((thrd-begin)/num_t){}

    point_t point(size_t i,size_t j)const
    {
        return m_begin+i*m_s_ort+
                       j*m_t_ort;
    }
    size_t rows()const{return m_rows;}
    size_t columns()const{return m_columns;}
};

#endif

