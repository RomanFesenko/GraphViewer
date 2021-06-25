
#ifndef  _plot_3D_
#define  _plot_3D_

#include <functional>
#include <numbers>

#include "data_mesh.h"
#include "rect_mesh.h"

#include "opengl_iface.h"

struct rendering_traits_t
{
    enum plot_type:int
    {
        colored_surfase_id= 1<<0,
        specular_surface_id=1<<1,
        mesh_id=            1<<2,
        close_box_id=       1<<3,
        x_levels_id=        1<<4,
        y_levels_id=        1<<5,
        z_levels_id=        1<<6,
    };
    const static int surface_id=colored_surfase_id|specular_surface_id;
    const static int colored_id=colored_surfase_id|mesh_id;
    const static int levels_id=x_levels_id|y_levels_id|z_levels_id;
    std::function<point3D_t(const point3D_t&)> m_color_functor;
    int m_type;
    int m_level_lines;
    std::size_t m_num_s_range;
    std::size_t m_num_t_range;
    rendering_traits_t();
};



class CPlot3D
{
    using point_t=Eigen::Vector3f;
    using matrix_t=Eigen::MatrixX<point3D_t>;
    using size_t=std::size_t;
    // displacement of the level lines relative to the graph
    const float Levels_displace=0.3f;
    rendering_traits_t m_traits;
    std::function<point3D_t(float,float,float)> m_plot_functor=nullptr;
    bool m_is_dynamic;
    mutable CLightSource m_light_source;
    data_mesh<point3D_t,float> m_mesh;
    matrix_t m_colors;
    point3D_t m_scale_coeffs;// for bring closed box к [1-,1]^3
    void m_SetPointsColor();
    void m_FillMeshData(float time);
    void m_LightingSwitch(bool) const;
    template<class F_t>
    auto bind_time(F_t f,float t)
    {
      return [f,t](float _1,float _2){return f(_1,_2,t);};
    }
    public:
    template<class F_t>
    struct graph_t
    {
      F_t m_functor;
      std::pair<float,float> m_s_range;
      std::pair<float,float> m_t_range;
      bool m_is_dynamic=false;
    };
    template<class F_t>
    static auto
    make_cartesian(F_t f,std::pair<float,float> x_r,
                         std::pair<float,float> y_r)
    {
      auto functor=[f](float par_1,float par_2,float t)
      {
          return point_t(par_1,par_2,f(par_1,par_2));
      };
      return graph_t<decltype(functor)>(functor,x_r,y_r,false);
    }
    template<class F_t>
    static auto
    make_cartesian_dyn(F_t f,std::pair<float,float> x_r,
                                   std::pair<float,float> y_r)
    {
      auto functor=[f](float par_1,float par_2,float t)
      {
          return point_t(par_1,par_2,f(par_1,par_2,t));
      };
      return graph_t<decltype(functor)>(functor,x_r,y_r,true);
    }
    template<class F_t>
    static auto
    make_spherical(F_t r)//r(@,phi)
    {
      using namespace std::numbers;
      auto functor=[r](float par_1,float par_2,float t)
      {
          float r_t_fi=r(par_1,par_2);
          return point_t(r_t_fi*sinf(par_1)*cosf(par_2),
                         r_t_fi*sinf(par_1)*sinf(par_2),
                         r_t_fi*cosf(par_1));
      };
      return graph_t<decltype(functor)>(functor,{0,pi},{-pi,pi},false);
    }
    template<class F_t>
    static auto
    make_spherical_dyn(F_t r)
    {
      using namespace std::numbers;
      auto functor=[r](float par_1,float par_2,float t)
      {
          float r_t_fi=r(par_1,par_2,t);
          return point_t(r_t_fi*sinf(par_1)*cosf(par_2),
                         r_t_fi*sinf(par_1)*sinf(par_2),
                         r_t_fi*cosf(par_1));
      };
      return graph_t<decltype(functor)>(functor,{0,pi},{-pi,pi},true);
    }
    template<class F_t>
    static auto make_polar(F_t z,float r_max)//z(r,phi)
    {
      using namespace std::numbers;
      auto functor=[z](float ro,float phi,float t)
      {
          float z_t_fi=z(ro,phi);
          return point_t(ro*cosf(phi),
                         ro*sinf(phi),
                         z_t_fi);
      };
      return graph_t<decltype(functor)>(functor,{0,r_max},{-pi,pi},false);
    }
    template<class F_t>
    static auto make_polar_dyn(F_t z,float r_max)//z(r,phi,time)
    {
      using namespace std::numbers;
      auto functor=[z](float ro,float phi,float t)
      {
          float z_t_fi=z(ro,phi,t);
          return point_t(ro*cosf(phi),
                         ro*sinf(phi),
                         z_t_fi);
      };
      return graph_t<decltype(functor)>(functor,{0,r_max},{-pi,pi},true);
    }
    template<class F_x_t,class F_y_t,class F_z_t>
    //x(t),y(t),z(t)
    static auto make_parametric(F_x_t _x,F_y_t _y,F_z_t _z,
                                std::pair<float,float> rp1,
                                std::pair<float,float> rp2)
    {
      auto f_=[_x,_y,_z](float p1,float p2,float t)
      {
          return point_t(_x(p1,p2),_y(p1,p2),_z(p1,p2));
      };
      return graph_t<decltype(f_)>(f_,rp1,rp2,false);
    }
    template<class F_x_t,class F_y_t,class F_z_t>
    //x(t),y(t),z(t)
    static auto make_parametric_dyn(F_x_t _x,F_y_t _y,F_z_t _z,
                                std::pair<float,float> rp1,
                                std::pair<float,float> rp2)
    {
      auto f_=[_x,_y,_z](float p1,float p2,float t)
      {
          return point_t(_x(p1,p2,t),_y(p1,p2,t),_z(p1,p2,t));
      };
      return graph_t<decltype(f_)>(f_,rp1,rp2,true);
    }
    CPlot3D();
    void SetTraits(const rendering_traits_t&,float);
    template<class F_t>
    void SetGraph(const graph_t<F_t>&graph,const rendering_traits_t&rt)
    {
      m_plot_functor=graph.m_functor;
      m_is_dynamic=graph.m_is_dynamic;
      m_mesh.fill(bind_time(graph.m_functor,0.0f),
                  graph.m_s_range,graph.m_t_range,
                  rt.m_num_s_range,rt.m_num_t_range);
      SetTraits(rt,0.0f);
    }
    template<class F_t>
    void SetGraph(const graph_t<F_t>&graph)
    {
      SetGraph(graph,m_traits);
    }
    void SetFlag(int flag,float time);
    void DropFlag(int flags, float time);
    bool IsFlag(int flags)const;
    void UpdateForTime(float);
    void Draw()const;
    const rendering_traits_t& RenderingTraits()const{return m_traits;}
    bool Empty()const{return m_plot_functor==nullptr;}
    bool IsDynamic()const{return m_is_dynamic;}
    void Clear(){m_plot_functor=nullptr;}
    friend class CRenderer;
};

point3D_t DefaultColoredFunc(float param);
void DrawRectangleMesh(const rect_mesh_3d_t<point3D_t>&);

#endif

