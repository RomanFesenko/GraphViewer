#include <iostream>
#include "ruling.h"

///********************************************************
/// Управление камерой и обработка команд смены графика - *
///  контроллер в схеме MCV                               *
///********************************************************

CLightSource glLightSource;

light_params_t gl_lights_params=
light_params_t(
    point3D_t(0,0,5),// положение
    std::array<point3D_t,3>{
            point3D_t{0.1,0.1,0.1},// фон
            point3D_t{0.5,0.5,0.5}, // диффузное
            point3D_t{1.0,1.0,1.0}  //отраженное
    },
    point3D_t(1.1,1.0,1.0) // расстояние
);

CRenderer::CRenderer():
m_current(glLightSource,m_mesh)
{
    SetPerspective(to_radian(80.0f),1.0f,0.1f,40.0f);
}

// В начале обзора камера устанавливается так чтобы
// в угол обзора попадали все точки, удерживая при этом
// минимально возможную дистанцию от центра графика

void CRenderer::m_SetCameraDefault()
{
    if(!m_IsActive()) return;
    auto close_rect=m_mesh.close_box();
    point3D_t center=(close_rect.first+close_rect.second)/2.0f;
    point3D_t delta=close_rect.second-close_rect.first;
    float _l=0.5*(delta[0]+std::max(delta[1],delta[2])/tan(m_aspect/2.0));
    m_camera.SetPosition(center+point3D_t::ort_t<0>(_l));
    m_camera.SetView(-glXOrt);
}

void CRenderer::SetPerspective(float aspect,float ratio,float near,float far)
{
    m_aspect=aspect;
    m_camera.SetPerspective(aspect,ratio,near,far);
    SetMatrix(m_camera.PerspectiveMatrix(),GL_PROJECTION);
}

void CRenderer::SetContent(const render_content_t&content,float current)
{
    glLightSource.SetParams(gl_lights_params);
    m_time_begin=current;
    m_content=content;
    m_plot_iterator=0;
    m_current.Set(m_content.m_plots[m_plot_iterator].second,
                  m_content.m_plots[m_plot_iterator].first);
    m_SetCameraDefault();

    m_LightingSwitch(m_content.m_plots[m_plot_iterator].second.m_type&
                     plot3D_chars_t::specular_surface_id);
}

void CRenderer::Render(float current)
{
    if(!m_IsActive()) return;
    SetMatrix(m_camera.ViewMatrix(),GL_MODELVIEW);
    SetMatrix(m_camera.PerspectiveMatrix(),GL_PROJECTION);
    m_current.UpdateForTime(current-m_time_begin);
    m_current.Draw();
}

void CRenderer::NextPlot(float current)
{
    if(!m_IsActive()) return;
    m_time_begin=current;
    if(++m_plot_iterator==m_content.m_plots.size()) m_plot_iterator=0;
    m_current.Set(m_content.m_plots[m_plot_iterator].second,
                  m_content.m_plots[m_plot_iterator].first);
    m_SetCameraDefault();

    m_LightingSwitch(m_content.m_plots[m_plot_iterator].second.m_type&
                     plot3D_chars_t::specular_surface_id);
}

void CRenderer::IncMove(move_t move_)
{
    switch(move_)
    {
        case forward_id:
        m_camera.IncreasePosition(m_camera.GetViewOrt()*m_content.m_move_velocity);
        break;

        case back_id:
        m_camera.IncreasePosition(-m_camera.GetViewOrt()*m_content.m_move_velocity);
        break;

        case right_id:
        m_camera.IncreasePosition(m_camera.GetRightOrt()*m_content.m_move_velocity);
        break;

        default:
        m_camera.IncreasePosition(-m_camera.GetRightOrt()*m_content.m_move_velocity);
    }

}

void CRenderer::IncView(float x,float y)
{
    float mod=sqrt(x*x+y*y);
    m_camera.IncreaseView(y*m_content.m_view_velocity/mod,
                          -x*m_content.m_view_velocity/mod);
}

void CRenderer::m_LightingSwitch(bool _on)
{
    if(_on)
    {
        glShadeModel(GL_SMOOTH);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    }
    else
    {
        glDisable(GL_LIGHT0);
        glDisable(GL_LIGHTING);
    }
}

