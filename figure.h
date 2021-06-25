
#ifndef  _figure_
#define  _figure_

#include <utility>
#include <vector>
#include <iterator>
#include <algorithm>


#include "transformation.h"

///Выпуклый многогранник

template<class point_t,class side_data_t>
class polyhedron_t
{
    std::vector<point_t> m_vertexes; //вершины
    mutable std::vector<std::pair<point_t,point_t>> m_edges;// заполняется по требованию
    using value_t=decltype(m_vertexes[0][0]+0);
    using size_t=std::size_t;
    public:
    struct side_t
    {
        side_data_t m_data;
        std::vector<size_t> m_vertexes;// индексы вершин образующих грань

    };
    private:
    std::vector<side_t> m_sides; // грани

    //определение всех пар вершин образующих ребра
    void m_set_edges()const
    {
        if(!m_edges.empty()) return;
        using ipair=std::pair<size_t,size_t>;
        std::vector<ipair> temp;
        for(auto&side:m_sides)
        {
            const auto& vertexes=side.m_vertexes;
            for(auto vert=vertexes.begin();vert<vertexes.end()-1;++vert)
            {
                temp.push_back(std::minmax(*vert,*(vert+1)));
            }
            temp.push_back(std::minmax(vertexes.front(),vertexes.back()));
        }
        std::sort(temp.begin(),temp.end());
        temp.erase(std::unique(temp.begin(),temp.end()),temp.end());
        m_edges.resize(temp.size());
        std::transform(temp.begin(),temp.end(),m_edges.begin(),
        [this](const ipair&pr){return std::pair(m_vertexes[pr.first],m_vertexes[pr.second]);});
    }
    public:
    polyhedron_t(const std::vector<point_t>&vertex,
                 const std::vector<side_t>& sides):
    m_vertexes(vertex),m_sides(sides){}
    template<class matrix_t>
    void transform(const matrix_t& tm)
    {
        auto transform_op=[&tm](const point_t& point)
        {
            return transform_3D(tm,point);
        };
        std::transform(m_vertexes.begin(),
                       m_vertexes.end(),
                       m_vertexes.begin(),transform_op);
    }
    const std::vector<point_t>& vertexes()const{return m_vertexes;}
    const std::vector<side_t>& sides()const{return m_sides;}
    const std::vector<std::pair<point_t,point_t>>& edges()const
    {
        m_set_edges();
        return m_edges;
    }
};

struct place_holder_t{};

template<class value_t,class point_t,class side_t=place_holder_t>
polyhedron_t<point_t,side_t>parallelepiped(const point_t&center,
                            value_t x_length,
                            value_t y_length,
                            value_t z_length,
                            std::pair<side_t,side_t>  x_sides={place_holder_t(),place_holder_t()},
                            std::pair<side_t,side_t>  y_sides={place_holder_t(),place_holder_t()},
                            std::pair<side_t,side_t>  z_sides={place_holder_t(),place_holder_t()})
{
    point_t min_corner=center-point_t(x_length/2,
                                      y_length/2,
                                      z_length/2);

    return polyhedron_t<point_t,side_t>(
           {
                min_corner,
                min_corner+point_t(x_length,0,0),
                min_corner+point_t(x_length,y_length,0),
                min_corner+point_t(0,y_length,0),

                min_corner+point_t(0,0,z_length),
                min_corner+point_t(x_length,0,z_length),
                min_corner+point_t(x_length,y_length,z_length),
                min_corner+point_t(0,y_length,z_length)
           },
           {
                {x_sides.second,{1,2,6,5}},
                {x_sides.first, {0,4,7,3}},
                {y_sides.second,{2,3,7,6}},
                {y_sides.first, {0,1,5,4}},
                {z_sides.second,{4,5,6,7}},
                {z_sides.first, {0,3,2,1}}

           }
    );
}

// апроксимация нормали к поверхности в точке center,
// по методу Гуро
template<class array_t,class iterator_t,class sentinel_t>
array_t gouraud_normal(const array_t&center,
                       iterator_t begin, sentinel_t end)
{
    using value_t=decltype(center[0]+0);
    const value_t min_square(0.001);

    array_t result(0,0,0);
    iterator_t next;
    iterator_t prev;
    for(iterator_t prev=begin,next=prev+1;next!=end;++prev,++next)
    {
        array_t temp=cross_product_3D(*prev-center,*next-center);
        value_t dist=distance_3D(temp);
        if(dist<min_square) continue;
        result+=temp/dist;
    }

    array_t temp=cross_product_3D(*prev-center,*begin-center);
    value_t dist=distance_3D(temp);
    if(dist>min_square)
    {
        result+=temp/dist;
    }
    return normalize_3D(result);
}

#endif

