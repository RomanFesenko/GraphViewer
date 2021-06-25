
#ifndef  _data_mesh_
#define  _data_mesh_

#include <utility>
#include <vector>
#include <array>
#include <algorithm>
#include <assert.h>


#include <Eigen/Core>

inline std::array<std::pair<std::size_t,std::size_t>,4>
quad_matrix(std::size_t i,std::size_t j)
{
    using ipair_t=std::pair<std::size_t,std::size_t>;

    return std::array<ipair_t,4>
    {ipair_t{i,j},ipair_t{i+1,j},ipair_t{i+1,j+1},ipair_t{i,j+1}};
    //{ipair_t{i,j+1},ipair_t{i+1,j+1},ipair_t{i+1,j},ipair_t{i,j}};
}

///data_mesh
// contains and updates the coordinates of points, normals, and level lines
template<class point_t,class value_t>
class data_mesh
{
    using size_t=std::size_t;
    using matrix_t=Eigen::Matrix<point_t,Eigen::Dynamic,Eigen::Dynamic>;
    matrix_t m_data;            // точки
    value_t m_s_delta;                      // шаг по параметру s
    value_t m_t_delta;                      // шаг по параметру t
    std::pair<value_t,value_t> m_s_range;   // диапазон измения s
    std::pair<value_t,value_t> m_t_range;   // диапазон измения t
    mutable matrix_t m_normals; // нормали
    std::pair<point_t,point_t> m_close_box; // замыкающий пара-д
    struct level_line_t
    {
        value_t m_constant;
        std::vector<std::pair<point_t,point_t>> m_lines;
    };
    std::array<std::vector<level_line_t>,3> m_levels; // линии уровня

    // define the closing box of the point cloud
    void m_set_close_box()
    {
        m_close_box.first=m_close_box.second=m_data(0,0);
        for(size_t i_s=0;i_s<m_data.rows();++i_s)
        {
            for(size_t i_t=0;i_t<m_data.cols();++i_t)
            {
                for(size_t dim=0;dim<3;dim++)
                {
                    const point_t& current=m_data(i_s,i_t);
                    m_close_box.first[dim]=std::min(current[dim],m_close_box.first[dim]);
                    m_close_box.second[dim]=std::max(current[dim],m_close_box.second[dim]);
                }
            }
        }
    }
    public:
    data_mesh(){}
    size_t rows()const{return m_data.rows();}
    size_t cols()const{return m_data.cols();}
    void resize(std::pair<value_t,value_t> s_range,
                std::pair<value_t,value_t> t_range,
                std::size_t num_s_range,std::size_t num_t_range)
    {
        m_data.resize(num_s_range+1,num_t_range+1);
        m_s_range=s_range;
        m_t_range=t_range;
        m_s_delta=(s_range.second-s_range.first)/num_s_range;
        m_t_delta=(t_range.second-t_range.first)/num_t_range;
    }

    template<class func_t>
    void fill(const func_t& functor)
    {
        for(size_t i_s=0;i_s<m_data.rows();++i_s)
        {
            for(size_t i_t=0;i_t<m_data.cols();++i_t)
            {
                m_data(i_s,i_t)=functor(m_s_range.first+m_s_delta*i_s,
                                         m_t_range.first+m_t_delta*i_t);
            }
        }
        m_set_close_box();
    }

    template<class func_t>
    void fill(const func_t& functor,
         std::pair<value_t,value_t> s_range,
         std::pair<value_t,value_t> t_range,
         std::size_t num_s_range,std::size_t num_t_range)
    {
        resize(s_range,t_range,num_s_range,num_t_range);
        fill(functor);
    }
    std::pair<point_t,point_t> close_box()const{return m_close_box;}
    const matrix_t& get_points()const{return m_data;}
    void set_normals();

    const matrix_t& get_normals()const{return m_normals;}

    template<class func_t>
    void set_level_line(const func_t&f,value_t constant,int );
    void clear_levels()
    {
        for(auto& lev:m_levels) lev.clear();
    }
    const std::array<std::vector<level_line_t>,3>& levels()const{return m_levels;}
    friend class CPlot3D;
};

// normal to parametrically defined surface
// defined as || (dr / ds) x (dr / dt) ||
// derivatives are approximated by finite differences

template<class point_t,class value_t>
void data_mesh<point_t,value_t>::set_normals()
{
    const value_t min_dist=0.001;
    auto check_and_set=[min_dist,this](const point_t&for_set,size_t i,size_t j)
    {
        value_t len=for_set.norm();
        return (len<min_dist)? point_t(0,0,1):for_set/len;
    };
    auto get_s_tangent=[this](size_t i,size_t j)
    {
        if(i==0)
        {
            return (m_data(1,j)-m_data(0,j))/(m_s_delta);
        }
        else if(i==m_data.rows()-1)
        {
            return (m_data(i,j)-m_data(i-1,j))/(m_s_delta);
        }
        else
        {
            return (m_data(i+1,j)-m_data(i-1,j))/(2*m_s_delta);
        }
    };
    auto get_t_tangent=[this](size_t i,size_t j)
    {
        if(j==0)
        {
            return (m_data(i,1)-m_data(i,0))/(m_t_delta);
        }
        else if(j==m_data.cols()-1)
        {
            return (m_data(i,j)-m_data(i,j-1))/(m_t_delta);
        }
        else
        {
            return (m_data(i,j+1)-m_data(i,j-1))/(2*m_t_delta);
        }
    };
    m_normals.resize(m_data.rows(),m_data.cols());
    for(int i=0;i<m_normals.rows();++i)
      for(int j=0;j<m_normals.cols();++j)
      {
          m_normals(i,j)=get_s_tangent(i,j).cross(get_t_tangent(i,j));
          m_normals(i,j).normalize();//fixed,ugly specular rendering without GL_NORMALIZE
      }

}

/* define level lines by looking for level plane intersections
  with the sides of the grid cells
  The point of intersection of the segment P-Q with the plane A * r = k,
  defined as ri = (P (q-k) -Q * (p-k)) / (q-p), p = P * A, q = Q * A
  func must be equal for "func" from fill (func) !!!
*/
template<class point_t,class value_t>
template<class functor_t>
void data_mesh<point_t,value_t>::set_level_line(const functor_t&func,value_t constant,int coord)
{
    //using ipair=std::pair<std::size_t,std::size_t>;
    assert(coord==0||coord==1||coord==2);
    m_levels[coord].push_back(level_line_t());
    level_line_t& _level=m_levels[coord].back();
    _level.m_constant=constant;
    std::vector<std::pair<size_t,size_t>> intersections;
    auto cell_process=[this,&func,constant,&intersections,&_level,coord](size_t _s,size_t _t)
    {
        intersections.clear();
        auto is_same_sign=[](value_t val_1,value_t val_2)
        {
            return (val_1>=0) == (val_2>=0);
        };

        auto intersect_point=[](const point_t&fst,const point_t&snd,
                                value_t delta_fst,value_t delta_snd)
        {
            return (fst*delta_snd-snd*delta_fst)/(delta_snd-delta_fst);
        };

        std::array<value_t,4> difference;
        std::array<point_t,4> cell;

        {
            int _ind=0;
            for(auto& [i,j]:quad_matrix(_s,_t))
            {
                cell[_ind]=m_data(i,j);
                difference[_ind++]=m_data(i,j)[coord]-constant;
            }
        }

        for(size_t i=0;i<4;i++)
        {
            size_t next=(i+1)%4;
            if(!is_same_sign(difference[i],difference[next]))
            {
                intersections.push_back(std::pair{i,next});
            }
        }
        // define intersection points
        if(intersections.size()==0)
        {
            return;
        }
        else if(intersections.size()==2)
        {
            auto [_1,_2]=intersections[0];
            auto [_3,_4]=intersections[1];
            _level.m_lines.push_back(
            {
                intersect_point(cell[_1],cell[_2],difference[_1],difference[_2]),
                intersect_point(cell[_3],cell[_4],difference[_3],difference[_4])
            });
        }
        else if(intersections.size()==4)
        {
            point_t quad_center=func(m_s_range.first+_s*m_s_delta+m_s_delta/2,
                                     m_t_range.first+_t*m_t_delta+m_t_delta/2);
            if(is_same_sign(difference[0],quad_center[coord]-constant))
            {
                _level.m_lines.push_back(
                {
                    intersect_point(cell[0],cell[1],difference[0],difference[1]),
                    intersect_point(cell[1],cell[2],difference[1],difference[2])
                });
                _level.m_lines.push_back(
                {
                    intersect_point(cell[2],cell[3],difference[2],difference[3]),
                    intersect_point(cell[3],cell[0],difference[3],difference[0])
                });
            }
            else
            {
                _level.m_lines.push_back(
                {
                    intersect_point(cell[0],cell[3],difference[0],difference[3]),
                    intersect_point(cell[0],cell[1],difference[0],difference[1])
                });
                _level.m_lines.push_back(
                {
                    intersect_point(cell[1],cell[2],difference[1],difference[2]),
                    intersect_point(cell[2],cell[3],difference[2],difference[3])
                });
            }
        }
        else{assert(false);}
    };
    for(size_t _s=0;_s<rows()-1;_s++)
    {
        for(size_t _t=0;_t<cols()-1;_t++)
        {
            cell_process(_s,_t);
        }
    }
}

// the graph in the form of a grid is built by connecting adjacent
// points by line segments

template<class out_iterator_t>
void linear_plot(out_iterator_t out_iterator,std::size_t mx_rows,std::size_t mx_cols)
{
    using size_t=std::size_t;
    using ipair_t=std::pair<size_t,size_t>;
    using segment_t=std::pair<ipair_t,ipair_t>;
    for(size_t row=0;row<mx_rows-1;++row)
    {
        for(size_t col=0;col<mx_cols-1;++col)
        {
            *out_iterator=segment_t{{row,col},{row,col+1}};
            *out_iterator=segment_t{{row,col},{row+1,col}};
        }
        *out_iterator=segment_t{{row,mx_cols-1},{row+1,mx_cols-1}};
    }
    // bordering curve
    for(size_t col=0;col<mx_cols-1;++col)
    {
        *out_iterator=segment_t{{mx_rows-1,col},{mx_rows-1,col+1}};
    }
}

#endif

