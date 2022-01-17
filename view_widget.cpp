#include <iostream>
#include <assert.h>
#include <math.h>
#include <numbers>

#include <GL/glew.h>

#include "Shaders/vao_managment.h"
#include "Shaders/shader_programm.h"


#include "opengl_iface.h"
#include "plot_2D_base.h"
#include "plot_2D_opengl.h"
#include "plot_2D_qt.h"
#include "view_widget.h"

#include <QtCore>
#include <QKeyEvent>

CFunctionalMesh glMesh;
static QTime glTime;

static float Now()
{
  return glTime.elapsed()/1000.0f;
}

/////////////////////////////////////////////////
//            CSceneViewer
/////////////////////////////////////////////////

CSceneViewer::CSceneViewer(QWidget*parent):QGLWidget(GetFormat(),parent)
{
   m_move_velocity=0.2f;
   m_turn_velocity=0.03f;
}



void CSceneViewer::initializeGL()
{
  if(m_scene) return;// initialized
  makeCurrent();
  show();
  resize(800,800);
  glViewport(0,0,800,800);

  glClearColor(0,  0.0, 0.0, 1.0);
  glewExperimental=GL_TRUE;
  if(glewInit() != GLEW_OK)
  {
      assert(false);
  }

  m_scene=std::make_unique<CScene>();
  m_scene->AddMesh(glMesh);
}

void CSceneViewer::paintGL()
{
  assert(m_scene);
  glCheckError();
  m_scene->Render(Now());

  swapBuffers();
}

float   CSceneViewer::MoveVelocity()const
{
  return m_move_velocity;
}

float   CSceneViewer::TurnVelocity()const
{
  return m_turn_velocity;
}

CSceneViewer& CSceneViewer::SetMoveVelocity(float vel)
{
   m_move_velocity=vel;
   return *this;
}

CSceneViewer& CSceneViewer::SetTurnVelocity(float vel)
{
  m_turn_velocity=vel;
  return *this;
}

/////////////////////////////////////////////////
//            CViewWidget
/////////////////////////////////////////////////

void CViewWidget::m_slotForUpdate()
{
  QTimer::singleShot(30,this,SLOT(m_slotForUpdate()));
  if(m_view==ogl_view_id)
  {
      m_scene_viewer->paintGL();
  }
  else
  {
      update();
  }
}

CViewWidget::CViewWidget(CPlot2D&p2, QLabel *sb, QWidget*p):
QWidget(p),
//m_view(qt_view_id),
m_view(ogl_view_id),
m_plot(p2),
m_statusbar(sb)
{
  setMouseTracking(m_view==qt_view_id);
  setFocusPolicy(Qt::ClickFocus);
  p2.SetScaling(CPlot2D::save_ratio_id);//p2.SetScaling(CPlot2D::accomodate_id);
  m_scene_viewer=new CSceneViewer(this);
  m_scene_viewer->initializeGL();
  if(m_view!=ogl_view_id)
  {
      m_scene_viewer->hide();
  }
  else
  {
      m_scene_viewer->show();
      m_scene_viewer->resize(width(),height());
      glViewport(0,0,width(),height());
  }
  m_slotForUpdate();
}


void CViewWidget::m_UpdateAlways()
{

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
      m_scene_viewer->resize(width(),height());
      glViewport(0,0,width(),height());
  }
}

void CViewWidget::keyPressEvent(QKeyEvent*ke)
{
  if(m_view==qt_view_id) return;
  auto&camera=Scene().Camera();
  float velocity=m_scene_viewer->MoveVelocity();
  switch(ke->key())
   {
       case Qt::Key_W:
       case Qt::Key_Up:
       camera.IncreasePosition(camera.GetViewOrt()*velocity);
       break;
       case Qt::Key_S:
       case Qt::Key_Down:
       camera.IncreasePosition(-camera.GetViewOrt()*velocity);
       break;
       case Qt::Key_D:
       case Qt::Key_Right:
       camera.IncreasePosition(camera.GetRightOrt()*velocity);
       break;
       case Qt::Key_A:
       case Qt::Key_Left:
       camera.IncreasePosition(-camera.GetRightOrt()*velocity);
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
   QPoint delta=(me->pos()-rect().center());
   float x=delta.x();
   float y=delta.y();
   float mod=sqrt(x*x+y*y);
   if(mod==0.0f) return;
   x*=m_scene_viewer->TurnVelocity()/mod;
   y*=m_scene_viewer->TurnVelocity()/mod;
   Scene().Camera().IncreaseView(y,-x);
   is_warp=true;
   QCursor::setPos(mapToGlobal(rect().center()));
}

void CViewWidget::SwitchTo3D()
{
  if(m_view==ogl_view_id) return;
  m_plot.Clear();
  m_scene_viewer->show();
  m_scene_viewer->resize(width(),height());
  glViewport(0,0,width(),height());
  m_view=ogl_view_id;
  m_statusbar->setText("");
  setMouseTracking(false);
}

void CViewWidget::SwitchTo2D()
{
  if(m_view==qt_view_id) return;
  glMesh.Clear();
  m_scene_viewer->hide();
  m_view=qt_view_id;
  setMouseTracking(true);
}

void CViewWidget::Clear()
{
  m_plot.Clear();
  glMesh.Clear();
}
