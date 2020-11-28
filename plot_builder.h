
#ifndef  _plot_builder_
#define  _plot_builder_

#include <functional>

#include "data_mesh.h"
#include "rect_mesh_3d.h"

#include "opengl_iface.h"

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
    int m_level_lines=15;
    std::function<point3D_t(float,float)> m_coords_functor;
    std::pair<float,float> m_s_range;
    std::pair<float,float> m_t_range;
    std::size_t m_num_s_range;
    std::size_t m_num_t_range;
    std::function<point3D_t(const point3D_t&)> m_color_functor;
};



class CPlot3D
{
    using size_t=std::size_t;
    //смещение линий уровня относительно графика
    const float Levels_displace=0.3f;

    plot3D_chars_t m_chars;
    animate_chars_t m_animate_chars;
    CLightSource& m_light_source;
    data_mesh<point3D_t,float>& m_mesh;
    std::function<float(float)> m_advanser;
    ext::matrix<point3D_t> m_colors;
    point3D_t m_scale_coeffs;// для приведения замыкающего парал к [1-,1]^3
    void m_SetPointsColor();
    void m_FillMeshData();
    public:
    CPlot3D(CLightSource& light,data_mesh<point3D_t,float>& mesh):
    m_light_source(light),m_mesh(mesh){}
    void Set(const plot3D_chars_t&chars,animate_chars_t anim=
    animate_chars_t{animate_chars_t::no_animate_id,0.0f,nullptr});

    void UpdateForTime(float);
    void Draw()const;
    const plot3D_chars_t& Chars()const{return m_chars;}
};

point3D_t DefaultColoredFunc(float param);
void DrawRectangleMesh(const rect_mesh_3d_t<point3D_t>&);

#endif

