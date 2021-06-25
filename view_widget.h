#ifndef _view_widget_
#define _view_widget_

#include <memory>

#include "plot_2D_qt.h"
#include "renderer.h"

#include <QWidget>
#include <QLabel>
#include <QGLWidget>
#include <QTimer>


class ogl_viewer_t:public QGLWidget
{
  Q_OBJECT
  public:
  ogl_viewer_t(QWidget*p);
  void initializeGL()override;
  void paintGL()override;
  //void keyPressEvent(QKeyEvent*ke)override;
};

class CMainWindow;
class CViewWidget:public QWidget
{
  Q_OBJECT
  public:
  enum view_t{ qt_view_id,ogl_view_id};
  private:
  view_t m_view/*=qt_view_id*/;
  //2d plots
  CQtDrawer2D  m_qt_drawer;
  CPlot2D& m_plot;
  //3d plots
  ogl_viewer_t* m_ogl_viewer;
  CRenderer&m_3d_renderer;
  QLabel*m_statusbar;
  void m_UpdateAlways();
  private slots:
  void m_slotForUpdate();
  public:
  CViewWidget(CPlot2D&,CRenderer&,QLabel*,QWidget*p=nullptr);
  void resizeEvent(QResizeEvent*)override;
  void paintEvent(QPaintEvent*)override;
  void keyPressEvent(QKeyEvent*)override;
  void mouseMoveEvent(QMouseEvent*)override;
  void SwitchTo2D();
  void SwitchTo3D();
  void Clear();
  view_t ViewType()const{return m_view;}
  friend class ogl_viewer_t;
  friend class CMainWindow;
};

#endif // _view_widget_
