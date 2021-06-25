#include <iostream>
#include "renderer.h"

////////////////////////////////////////////////
/// Camera control and mode change commands
/// graph rendering
//////////////////////////////////////////////

CRenderer::CRenderer()
{
    SetPerspective(to_radian(80.0f),1.0f,0.1f,40.0f);
}

void CRenderer::SetMoveVelocity(float vel)
{
  m_move_velocity=vel;
}

// in degree
void CRenderer::SetViewVelocity(float vel)
{
  m_view_velocity=vel*Degree<float>;
}

void CRenderer::SetAnimationFunctor(std::function<float(float)> functor,float now)
{
  m_time_begin=now;
  if(functor) {m_animation_functor=functor;}
  else m_animation_functor=[](float t){return t;};
}

void CRenderer::SetFiniteAnimation(float max_anim_time,float now)
{
  m_time_begin=now;
  m_animation_functor=[max_anim_time](float time)
  {
      return std::min(time,max_anim_time);
  };
}

void CRenderer::SetPeriodicalAnimation(float period,float now)
{
  m_time_begin=now;
  m_animation_functor=[period](float time)
  {
      return time-floorf(time/period)*period;
  };
}


// At the beginning of the viewing, the camera is installed so that
// all points fell into the viewing angle, while holding
// minimum possible distance from the center of the graph

void CRenderer::m_SetCameraDefault()
{
    if(m_current.Empty()) return;
    auto close_rect=m_current.m_mesh.close_box();
    point3D_t center=(close_rect.first+close_rect.second)/2.0f;
    point3D_t delta=close_rect.second-close_rect.first;
    float _l=0.5*(delta[0]+std::max(delta[1],delta[2])/tan(m_aspect/2.0));
    m_camera.SetPosition(center+point3D_t::UnitX()*_l);
    m_camera.SetView(-point3D_t::UnitX());
}

void CRenderer::SetPerspective(float aspect,float ratio,float near,float far)
{
    m_aspect=aspect;
    m_camera.SetPerspective(aspect,ratio,near,far);
    SetMatrix(m_camera.PerspectiveMatrix(),GL_PROJECTION);
}

void CRenderer::SetRenderingTraits(const rendering_traits_t&traits,float now)
{
  m_current.SetTraits(traits,m_animation_functor((now-m_time_begin)*m_time_scaling));
}

bool CRenderer::SetTimeScaling(float sg)
{
  if(sg<0.1f||sg>10.0f) return false;
  m_time_scaling=sg;
  return true;
}

void CRenderer::SetFlag(int flag,float now)
{
  assert(now>=m_time_begin);
  m_current.SetFlag(flag,m_animation_functor((now-m_time_begin)*m_time_scaling));
}

void CRenderer::DropFlag(int flag,float now)
{
  assert(now>=m_time_begin);
  m_current.DropFlag(flag,m_animation_functor((now-m_time_begin)*m_time_scaling));
}

void CRenderer::Render(float current)
{
    if(m_current.Empty()) return;
    SetMatrix(m_camera.ViewMatrix(),GL_MODELVIEW);
    SetMatrix(m_camera.PerspectiveMatrix(),GL_PROJECTION);
    m_current.UpdateForTime(m_animation_functor((current-m_time_begin)*m_time_scaling));
    m_current.Draw();
}

void CRenderer::IncMove(move_t move_)
{
    switch(move_)
    {
        case forward_id:
        m_camera.IncreasePosition(m_camera.GetViewOrt()*m_move_velocity);
        break;

        case back_id:
        m_camera.IncreasePosition(-m_camera.GetViewOrt()*m_move_velocity);
        break;

        case right_id:
        m_camera.IncreasePosition(m_camera.GetRightOrt()*m_move_velocity);
        break;

        default:
        m_camera.IncreasePosition(-m_camera.GetRightOrt()*m_move_velocity);
    }

}

void CRenderer::IncView(float x,float y)
{
    float mod=sqrt(x*x+y*y);
    m_camera.IncreaseView(y*m_view_velocity/mod,-x*m_view_velocity/mod);
}
