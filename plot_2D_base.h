
#ifndef  _plot_2D_base_
#define  _plot_2D_base_

#include <assert.h>
#include <memory>
#include <functional>
#include <vector>
#include <numbers>
#include <optional>

#include <Eigen/Core>
#include <Eigen/Geometry>


class CDrawer2D
{
  using point_t=Eigen::Vector2f;
  using box_t=Eigen::AlignedBox<float,2>;
public:
  struct viewport_t
  {
    point_t m_bottom_left,m_top_right;
    int m_width_in_pixels,m_height_in_pixels;
    float width()const; //>0
    float height()const; //>0
    point_t top_left()const;
    point_t bottom_right()const;
    float sign_x()const;
    float sign_y()const;
  };
  CDrawer2D() {}
  virtual viewport_t AvailableViewport()const=0;
  virtual void SetMatrix(const Eigen::Matrix3f&)=0;
  virtual void DrawLineBegin()=0;
  virtual void DrawEnd()=0;
  virtual void DrawLine(const point_t&,const point_t&)=0;
  virtual void SetColor(const Eigen::Vector3f&)=0;
  virtual void SetWidth(int)=0;
  virtual void FillBackground(const box_t&)=0;
  virtual int StringHeight(const std::string&)const=0;
  virtual int StringWidth(const std::string&)const=0;
  virtual void StringOut(const std::string&,int x,int y)=0;
  virtual ~CDrawer2D(){}
};

class CPlot2D
{
    using point_t=Eigen::Vector2f;
    using box_t=Eigen::AlignedBox<float,2>;
    using static_fn_t=std::function<point_t(float)>;
    using anime_fn_t=std::function<point_t(float,float)>;
    public:
    enum scaling_t
    {
      accomodate_id,save_ratio_id
    };
    enum coord_t
    {
      pixel_id,drawer_id,world_id
    };
    enum side_t
    {
      left_id,right_id,bottom_id,top_id
    };
    scaling_t m_scaling=accomodate_id;
    template<class F_t>
    struct graph_t
    {
      F_t m_functor;
      std::pair<float,float> m_param_range;
      bool m_is_dynamic;
      bool m_is_cartesian;
    };
    template<class F_t>
    static auto make_cartesian(F_t f,std::pair<float,float> r={-1,1})
    {
      auto f_=[f](float par,float t)
      {
          return point_t(par,f(par));
      };
      return graph_t<decltype(f_)>(f_,r,false,true);
    }
    template<class F_t>
    static auto make_cartesian_dyn(F_t f,std::pair<float,float> r={-1,1})
    {
      auto f_=[f](float par,float t)
      {
          return point_t(par,f(par,t));
      };
      return graph_t<decltype(f_)>(f_,r,true,true);
    }
    template<class F_t>
    static auto make_polar(F_t r)//r(@)
    {
      using namespace std::numbers;
      auto f_=[r](float fi,float t)
      {
          float r_fi=r(fi);
          return point_t(r_fi*cosf(fi),r_fi*sinf(fi));
      };
      return graph_t<decltype(f_)>(f_,{-pi,pi},false,false);
    }
    template<class F_t>
    static auto make_polar_dyn(F_t r)
    {
      using namespace std::numbers;
      auto f_=[r](float fi,float t)
      {
          float r_fi=r(fi,t);
          return point_t(r_fi*cosf(fi),r_fi*sinf(fi));
      };
      return graph_t<decltype(f_)>(f_,{-pi,pi},true,false);
    }
    template<class F_x_t,class F_y_t>
    static auto make_parametric(F_x_t _x,F_y_t _y,
                                std::pair<float,float> r)//x(t),y(t
    {
      auto f_=[_x,_y](float par,float t)
      {
          return point_t(_x(par),_y(par));
      };
      return graph_t<decltype(f_)>(f_,r,false,false);
    }
    template<class F_x_t,class F_y_t>
    static auto make_parametric_dyn(F_x_t _x,F_y_t _y,
                                    std::pair<float,float> r)
    {
      auto f_=[_x,_y](float par,float t)
      {
          return point_t(_x(par,t),_y(par,t));
      };
      return graph_t<decltype(f_)>(f_,r,true,false);
    }
    struct traits_t
    {
      inline static int m_num_points=200;
      //Eigen::Vector3f m_color={1,0,0};
      int m_width=1;
    };
    class plot_t:protected traits_t
    {
        std::function<point_t(float,float)> m_plot_functor;
        std::pair<float,float> m_param_range;
        bool m_is_dynamic;
        bool m_is_cartesian;
        box_t m_box;
        std::vector<point_t> m_points;
        float m_last_update;
        void m_Fill(float t);
        public:
        template<class F_t>
        explicit plot_t(const graph_t<F_t>&graph,const traits_t&pt={}):
        traits_t(pt)
        {
          m_plot_functor=graph.m_functor;
          m_param_range=graph.m_param_range;
          m_is_dynamic=graph.m_is_dynamic;
          m_is_cartesian=graph.m_is_cartesian;
          m_Fill(0.0f);
        }
        //void SetColor(Eigen::Vector3f&v){m_color=v;}
        void SetWidth(int i){m_width=i;}
        void SetNumPoints(int i){m_num_points=i;m_Fill(0);}
        friend class CPlot2D;
    };
    int m_num_hor_grid;
    int m_num_vert_grid;
    private:
    std::vector<std::string> m_x_values;
    std::vector<std::string> m_y_values;
    Eigen::Vector3f m_grid_color;
    Eigen::Vector3f m_back_color;
    Eigen::Vector3f m_scale_color;
    std::vector<Eigen::Vector3f> m_graphs_color;
    Eigen::AlignedBox<float,2> m_box;
    std::vector<plot_t> m_plots;
    void m_UpdateStrings();
    void m_UpdateBoundary();
    //void m_set_matrix(CDrawer2D&)const;
    //std::pair<point_t,point_t> m_GetUsedRect(CDrawer2D&,coord_t)const;
    std::pair<point_t,point_t> m_GetGraphRect(CDrawer2D&,coord_t)const;
    int IndentionWidth(const CDrawer2D&,side_t)const;
    void m_PrintValues(std::pair<point_t,point_t>,//pixels
                       CDrawer2D&)const;
    public:
    CPlot2D();
    template<class F_t>
    int AddPlot(const graph_t<F_t>&graph,const traits_t&pt={})
    {
      m_plots.push_back(plot_t(graph,pt));
      m_UpdateBoundary();
      return m_plots.size()-1;
    }
    void ErasePlot(int i);
    void ResetBox(const box_t&);
    plot_t&Plot(int i);
    void SetScaling(scaling_t s){m_scaling=s;}
    void UpdateForTime(float);
    void Draw(CDrawer2D &)const;
    std::optional<point_t> WorldCoords(int x,int y,CDrawer2D &)const;
    bool Empty()const{return m_plots.empty();}
    int Size()const{return m_plots.size();}
    void SetGridLines(int vert,int hor);
    void Clear();
};
#endif

