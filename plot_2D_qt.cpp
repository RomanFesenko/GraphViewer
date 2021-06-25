#include <iostream>

#include "plot_2D_qt.h"
#include "transformation.h"

#include <QPaintDevice>

#pragma GCC diagnostic warning "-Wsign-conversion"
#pragma GCC diagnostic warning "-Wconversion"

CQtDrawer2D::viewport_t
CQtDrawer2D::AvailableViewport()const
{
  assert(m_painter);
  QPaintDevice*dev=m_painter->device();
  assert(dev);
  viewport_t vp;
  vp.m_width_in_pixels=dev->width();
  vp.m_height_in_pixels=dev->height();
  QRect rect=m_painter->viewport();
  vp.m_bottom_left=eigen_point(rect.bottomLeft());
  vp.m_top_right=eigen_point(rect.topRight());
  return vp;
}

void CQtDrawer2D::SetMatrix(const Eigen::Matrix3f&mx)
{
  QMatrix mat(mx(0,0),mx(1,0),
              mx(0,1),mx(1,1),
              mx(0,2),mx(1,2));
  m_painter->setMatrix(mat);
  m_domain_init=true;
}

void CQtDrawer2D::DrawLineBegin()
{
  //std::cout<<"draw...\n";
  assert(m_domain_init);
  assert(m_painter->isActive());
  m_painter->setPen(QPen(m_color,2,Qt::SolidLine));
}

void CQtDrawer2D::DrawEnd()
{
}

void CQtDrawer2D::DrawLine(const point_t&p1,const point_t&p2)
{
  m_painter->drawLine(qpoint(p1),qpoint(p2));
}

void CQtDrawer2D::SetColor(const Eigen::Vector3f&c)
{
  m_color.setRgb(c[0]*254.f,c[1]*254.f,c[2]*254.f);
  m_painter->setPen(m_color);
  //m_painter->setPen(QPen(m_color,1,Qt::SolidLine));
}

void CQtDrawer2D::SetWidth(int i)
{
  m_painter->setPen(QPen(m_color,i,Qt::SolidLine));
}

void CQtDrawer2D::FillBackground(const box_t&dn)
{
  QBrush brush(m_color,Qt::SolidPattern);
  QRectF rect(qpoint(dn.corner(box_t::TopLeft)),
              qpoint(dn.corner(box_t::BottomRight)));
  m_painter->fillRect(rect,brush);
}

int CQtDrawer2D::StringHeight(const std::string&str)const
{
  QFontMetrics fm(m_painter->font());
  return fm.height();
}

int CQtDrawer2D::StringWidth(const std::string&str)const
{
  QFontMetrics fm(m_painter->font());
  return fm.width(QString::fromStdString(str));
}

void CQtDrawer2D::StringOut(const std::string&str,int x,int y)
{
  m_painter->setTransform({});
  m_painter->drawText(x,y,QString::fromStdString(str));
}

