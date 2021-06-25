
#ifndef  _plot_2D_qt_
#define  _plot_2D_qt_

#include "plot_2D_base.h"
#include <QPainter>


class CQtDrawer2D:public CDrawer2D
{
  using point_t=Eigen::Vector2f;
  using box_t=Eigen::AlignedBox<float,2>;
  bool m_domain_init=false;
  box_t m_domain;
  QColor m_color={0,0,0};
  QPainter*m_painter{nullptr};
  static QPointF qpoint(const point_t&p)
  {return QPointF(p[0],p[1]);}
  static point_t eigen_point(const QPointF&p)
  {return point_t(p.x(),p.y());}
public:
  CQtDrawer2D() {}
  void SetPainter(QPainter&p){m_painter=&p;}
  virtual viewport_t AvailableViewport()const;
  virtual void SetMatrix(const Eigen::Matrix3f&)override;
  virtual void DrawLineBegin()override;
  virtual void DrawEnd()override;
  virtual void DrawLine(const point_t&,const point_t&)override;
  virtual void SetColor(const Eigen::Vector3f&)override;
  virtual void SetWidth(int)override;
  virtual void FillBackground(const box_t&)override;
  virtual int StringHeight(const std::string&)const override;
  virtual int StringWidth(const std::string&)const override;
  virtual void StringOut(const std::string&,int x,int y) override;
};

#endif

