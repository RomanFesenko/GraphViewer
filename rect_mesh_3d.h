
#ifndef  _rect_mesh_3d_
#define  _rect_mesh_3d_

#include <utility>

//прямоугольная сетка
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

