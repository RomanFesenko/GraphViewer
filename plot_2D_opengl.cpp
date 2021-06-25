#include "plot_2D_opengl.h"
#include "transformation.h"

#include <GL/gl.h>

#include <iostream>

// Raw specialization for abstact CDrawer2D
// problems with digits rendering

CDrawer2D::viewport_t
COpenGLDrawer2D::AvailableViewport()const
{
  assert(fn_sizes);
  viewport_t vp;
  vp.m_bottom_left=point_t(-1.f,-1.f);
  vp.m_top_right=point_t(1.f,1.f);
  auto sizes=fn_sizes();
  vp.m_width_in_pixels=sizes.first;
  vp.m_height_in_pixels=sizes.second;
  return vp;
}

void COpenGLDrawer2D::SetMatrix(const Eigen::Matrix3f&_mx)
{
  assert(!m_draw);
  glMatrixMode(GL_MODELVIEW);glLoadIdentity();
  Eigen::Matrix4f mx;
  mx.setIdentity();
  mx.block<2,2>(0,0)=_mx.block<2,2>(0,0);
  mx.block<2,1>(0,3)=_mx.block<2,1>(0,2);
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(mx.data());
  m_domain_init=true;
}

void COpenGLDrawer2D::DrawLineBegin()
{
  assert(!m_draw);
  assert(m_domain_init);
  glBegin(GL_LINES);
  m_draw=true;
}

void COpenGLDrawer2D::DrawEnd()
{
  assert(m_draw);
  glEnd();
  m_draw=false;
}

void COpenGLDrawer2D::DrawLine(const point_t&p1,const point_t&p2)
{
  assert(m_draw);
  glVertex2fv(p1.data());
  glVertex2fv(p2.data());
}

void COpenGLDrawer2D::SetColor(const Eigen::Vector3f&c)
{
  glColor3fv(c.data());
}

void COpenGLDrawer2D::SetWidth(int i)
{
  assert(!m_draw);
  glLineWidth(i);
}

void COpenGLDrawer2D::FillBackground(const box_t&dn)
{
  assert(!m_draw);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glBegin(GL_QUADS);
  point_t p=dn.corner(box_t::BottomLeft);
  glVertex3f(p[0],p[1],m_back_disp);
  p=dn.corner(box_t::TopLeft);
  glVertex3f(p[0],p[1],m_back_disp);
  p=dn.corner(box_t::TopRight);
  glVertex3f(p[0],p[1],m_back_disp);
  p=dn.corner(box_t::BottomRight);
  glVertex3f(p[0],p[1],m_back_disp);
  glEnd();
}
