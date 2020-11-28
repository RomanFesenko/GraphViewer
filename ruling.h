
#ifndef  _ruling_
#define  _ruling_

#include "plot_builder.h"
#include "view_ruling.h"

struct render_content_t
{
    std::vector<std::pair<animate_chars_t,plot3D_chars_t>> m_plots;
    float m_move_velocity;
    float m_view_velocity;
};

class CRenderer
{
    data_mesh<point3D_t,float> m_mesh;
    CPlot3D m_current;
    CMoveableCamera m_camera;
    float m_aspect;
    render_content_t m_content;
    unsigned int m_plot_iterator;
    float m_time_begin;
    void m_SetCameraDefault();
    bool m_IsActive()const{return !m_content.m_plots.empty();}
    void m_LightingSwitch(bool);

    public:
    enum move_t
    {
        forward_id,
        back_id,
        right_id,
        left_id
    };
    CRenderer();
    void ClearContent(){m_content.m_plots.clear();}
    void SetPerspective(float aspect,float ratio,float ,float);
    void SetContent(const render_content_t&content,float current);
    void Render(float current);
    void NextPlot(float current);
    void IncMove(move_t move_);
    void IncView(float x,float y);
};

#endif

