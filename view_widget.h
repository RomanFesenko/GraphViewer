#ifndef _view_widget_
#define _view_widget_

#include <memory>

#include "plot_2D_qt.h"
#include "scene.h"

#include <QWidget>
#include <QLabel>
#include <QGLWidget>
#include <QTimer>


class CSceneViewer:public QGLWidget
{
  Q_OBJECT
  using point_t=Eigen::Vector3f;

  std::unique_ptr<CScene> m_scene;
  float m_move_velocity=0.2f;
  float m_turn_velocity=0.03f;
  static QGLFormat GetFormat()
  {
    QGLFormat format;
    format.setDepthBufferSize(24);
    format.setDoubleBuffer(true);
    format.setStencilBufferSize(8);
    format.setVersion(3,3);
    format.setProfile(QGLFormat::CoreProfile);
    return format;
  }

  public:
  CSceneViewer(QWidget*);
  void initializeGL()override;
  void paintGL()override;

  CScene& Scene(){return *m_scene;}
  float   MoveVelocity()const;
  float   TurnVelocity()const;
  CSceneViewer& SetMoveVelocity(float);
  CSceneViewer& SetTurnVelocity(float);
};

class CMainWindow;
class CViewWidget:public QWidget
{
  Q_OBJECT
  public:
  enum view_t{ qt_view_id,ogl_view_id};
  private:
  view_t m_view;
  //2d plots
  CQtDrawer2D  m_qt_drawer;
  CPlot2D& m_plot;
  //3d plots
  CSceneViewer* m_scene_viewer;
  QLabel*m_statusbar;
  void m_UpdateAlways();
  private slots:
  void m_slotForUpdate();
  public:
  CViewWidget(CPlot2D&,QLabel*,QWidget*p=nullptr);
  void resizeEvent(QResizeEvent*)override;
  void paintEvent(QPaintEvent*)override;
  void keyPressEvent(QKeyEvent*)override;
  void mouseMoveEvent(QMouseEvent*)override;
  void SwitchTo2D();
  void SwitchTo3D();
  CScene& Scene(){return m_scene_viewer->Scene();}
  CSceneViewer&Viewer(){return *m_scene_viewer;}
  void Clear();
  view_t ViewType()const{return m_view;}
  friend class CSceneViewer;
  friend class CMainWindow;
};

#endif // _view_widget_
