
#ifndef  _plot_2D_opengl_
#define  _plot_2D_opengl_

#include <utility>
#include <functional>
#include "plot_2D_base.h"


class COpenGLDrawer2D:public CDrawer2D
{
  using point_t=Eigen::Vector2f;
  using box_t=Eigen::AlignedBox<float,2>;
  bool m_domain_init=false;
  bool m_draw=false;
  const float m_back_disp=+0.5;
public:
  std::function<std::pair<int,int>()> fn_sizes;
  COpenGLDrawer2D() {}
  virtual viewport_t AvailableViewport()const;
  virtual void SetMatrix(const Eigen::Matrix3f&)override;
  virtual void DrawLineBegin()override;
  virtual void DrawEnd()override;
  virtual void DrawLine(const point_t&,const point_t&)override;
  virtual void SetColor(const Eigen::Vector3f&)override;
  virtual void SetWidth(int)override;
  virtual void FillBackground(const box_t&)override;
  //dummy
  virtual int StringHeight(const std::string&)const{return 1;}
  virtual int StringWidth(const std::string&)const{return 1;}
  virtual void StringOut(const std::string&,int x,int y){}
};

#endif

