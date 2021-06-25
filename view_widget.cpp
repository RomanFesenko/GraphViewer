#include <iostream>
#include <assert.h>
#include <math.h>
#include <numbers>

#include "opengl_iface.h"
#include "plot_2D_base.h"
#include "plot_2D_opengl.h"
#include "plot_2D_qt.h"
#include "view_widget.h"

#include <QtCore>
#include <QKeyEvent>

namespace num=std::numbers;

static QTime glTime;

static float Now()
{
  return glTime.elapsed()/1000.0f;
}

ogl_viewer_t::ogl_viewer_t(QWidget*p):
QGLWidget(p)
{}

void ogl_viewer_t::initializeGL()
{
  QWidget*p=static_cast<QWidget*>(parent());
  resize(p->width(), p->height());
  glEnable(GL_DEPTH_TEST);
  glClearColor( 0.0,  0.0, 0.0, 1.0);
}

void ogl_viewer_t::paintGL()
{
  CViewWidget*p=static_cast<CViewWidget*>(parent());
  if(p->m_view==CViewWidget::qt_view_id) return;
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  p->m_3d_renderer.Render(Now());
  swapBuffers();
  glCheckError();
}


void CViewWidget::m_slotForUpdate()
{
  QTimer::singleShot(30,this,SLOT(m_slotForUpdate()));
  m_UpdateAlways();
  //update();// QGLWidget hide CViewWidget and paintEvent not call
}

CViewWidget::CViewWidget(CPlot2D&p2, CRenderer&r3, QLabel *sb, QWidget*p):
QWidget(p),
m_view(qt_view_id),
m_plot(p2),
m_3d_renderer(r3),
m_statusbar(sb)
{
  setMouseTracking(m_view==qt_view_id);
  setFocusPolicy(Qt::ClickFocus);
  p2.SetScaling(CPlot2D::save_ratio_id);
  //p2.SetScaling(CPlot2D::accomodate_id);
  m_ogl_viewer=new ogl_viewer_t(this);
  if(m_view!=ogl_view_id) m_ogl_viewer->hide();
  m_slotForUpdate();
}


void CViewWidget::m_UpdateAlways()
{
  if(m_view==ogl_view_id)
  {
      m_ogl_viewer->paintGL();
  }
  else
  {
      update();
  }
}

void CViewWidget::paintEvent(QPaintEvent*)
{
  if(m_view==ogl_view_id) return;
  m_plot.UpdateForTime(Now());
  QPainter pr(this);
  pr.setFont(QFont("Times", 12, QFont::Normal));
  m_qt_drawer.SetPainter(pr);
  m_plot.Draw(m_qt_drawer);
  auto mouse=mapFromGlobal(QCursor::pos());
  auto coords=m_plot.WorldCoords(mouse.x(),mouse.y(),m_qt_drawer);
  if(coords)
  {
      QString str="X:"+QString::number((*coords)[0],'g',3)+
                  ",Y:"+QString::number((*coords)[1],'g',3);
      m_statusbar->setText(str);
  }
  else m_statusbar->setText("");
  pr.end();
}

void CViewWidget::resizeEvent(QResizeEvent*)
{
  if(m_view==ogl_view_id)
  {
      m_ogl_viewer->resize(width(),height());
      glViewport(0,0,width(),height());
  }
}

void CViewWidget::keyPressEvent(QKeyEvent*ke)
{
  if(m_view==qt_view_id) return;
  switch(ke->key())
   {
       case Qt::Key_W:
       case Qt::Key_Up:
       m_3d_renderer.IncMove(CRenderer::forward_id);
       break;
       case Qt::Key_S:
       case Qt::Key_Down:
       m_3d_renderer.IncMove(CRenderer::back_id);
       break;
       case Qt::Key_D:
       case Qt::Key_Right:
       m_3d_renderer.IncMove(CRenderer::right_id);
       break;
       case Qt::Key_A:
       case Qt::Key_Left:
       m_3d_renderer.IncMove(CRenderer::left_id);
       break;
       default:break;
    }
}


void CViewWidget::mouseMoveEvent(QMouseEvent*me)
{
  if(m_view==qt_view_id) return;
  static bool is_warp=false;
   if(is_warp)
   {
       is_warp=false;
       return;
   }
   QPoint delta;
   delta=me->pos()-rect().center();
   m_3d_renderer.IncView(delta.x(),delta.y());
   is_warp=true;
   QCursor::setPos(mapToGlobal(rect().center()));
}

void CViewWidget::SwitchTo3D()
{
  if(m_view==ogl_view_id) return;
  m_plot.Clear();
  m_ogl_viewer->show();
  m_ogl_viewer->resize(width(),height());
  glViewport(0,0,width(),height());
  m_view=ogl_view_id;
  m_statusbar->setText("");
  setMouseTracking(false);
}

void CViewWidget::SwitchTo2D()
{
  if(m_view==qt_view_id) return;
  m_3d_renderer.Clear();
  m_ogl_viewer->hide();
  m_view=qt_view_id;
  setMouseTracking(true);
}

void CViewWidget::Clear()
{
  m_plot.Clear();
  m_3d_renderer.Clear();
}
