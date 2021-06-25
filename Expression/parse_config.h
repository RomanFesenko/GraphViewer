
#ifndef  _parse_config_
#define  _parse_config_


#include "expression_parser.h"


#include <string>
#include "../GEOMETRY/primitives.h"

using str_citerator=std::string::const_iterator;
using point3D_t=Point3D<float>;

struct animate_chars_t
{
    enum animate_type_t
    {
        no_animate_id,
        finite_id,
        infinite_id,
        periodical_id
    };
    animate_type_t m_type;
    float m_time;
    std::function<point3D_t(float,float,float)> m_coords_functor;
};


struct plot3D_chars_t
{
    enum plot_type
    {
        colored_surfase_id= 1<<0,
        specular_surface_id=1<<1,
        mesh_id=            1<<2,
        close_box_id=       1<<3,
        x_levels_id=        1<<4,
        y_levels_id=        1<<5,
        z_levels_id=        1<<6,
        animated_id=        1<<7

    };
    const static int surface_id=colored_surfase_id|specular_surface_id;
    const static int colored_id=colored_surfase_id|mesh_id;
    const static int levels_id=x_levels_id|y_levels_id|z_levels_id;
    int m_type;
    int m_level_lines;
    std::function<point3D_t(float,float)> m_coords_functor;
    std::pair<float,float> m_s_range;
    std::pair<float,float> m_t_range;
    std::size_t m_num_s_range;
    std::size_t m_num_t_range;
    std::function<point3D_t(const point3D_t&)> m_color_functor;
};

struct parse_config_result_t
{
    parse_error_t m_parse_result;
    int m_index_error_string;

    parse_config_result_t():
    m_parse_result(parse_error_t::unknown_error_id,"")
    {}

    parse_config_result_t(parse_error_t pet,int _int):
    m_parse_result(pet),m_index_error_string(_int)
    {}

    std::vector<std::pair<animate_chars_t,plot3D_chars_t>> m_plots;
    bool is_valid()const{return m_parse_result.is_valid();}
};


parse_config_result_t ParseConfig(std::string&file);
std::string ErrorReport(const parse_config_result_t& pcr);

#endif

