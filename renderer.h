
#ifndef  _renderer_
#define  _renderer_

#include "plot_3D.h"
#include "view_ruling.h"

class CRenderer
{
    CPlot3D m_current;
    CMoveableCamera m_camera;
    float m_aspect;
    float m_time_begin;
    float m_time_scaling=1.0f;//0.1...10.0
    std::function<float(float)> m_animation_functor=[](float t){return t;};
    void m_SetCameraDefault();
    float m_view_velocity=2.0f*Degree<float>;
    public:
    float m_move_velocity=0.05f;
    enum move_t
    {
        forward_id,
        back_id,
        right_id,
        left_id
    };
    CRenderer();
    void SetMoveVelocity(float);
    void SetViewVelocity(float);
    float ViewVelocity()const{return m_view_velocity/Degree<float>;}
    void SetAnimationFunctor(std::function<float(float)>,float now);
    template<class F_t>
    void SetGraph(const CPlot3D::graph_t<F_t>&graph,float now,
                  const rendering_traits_t&rt)
    {
      m_time_begin=now;
      m_time_scaling=1.0f;
      m_current.SetGraph(graph,rt);
      m_SetCameraDefault();
    }
    template<class F_t>
    void SetGraph(const CPlot3D::graph_t<F_t>&graph,float now)
    {
      SetGraph(graph,now,m_current.RenderingTraits());
    }
    void SetFiniteAnimation(float max_anim_time,float now);
    void SetPeriodicalAnimation(float period,float now);
    float TimeScaling()const{return m_time_scaling;}
    bool SetTimeScaling(float);

    void SetPerspective(float aspect,float ratio,float ,float);
    void SetRenderingTraits(const rendering_traits_t&, float now);
    void SetFlag(int flag,float now);
    void DropFlag(int flag,float now);
    bool IsFlag(int flag)const{return m_current.IsFlag(flag);}
    const rendering_traits_t&RenderingTraits()const
    {return m_current.RenderingTraits();}

    bool Empty()const{return m_current.Empty();}
    void Clear(){m_current.Clear();}

    void Render(float current);
    void IncMove(move_t move_);
    void IncView(float x,float y);
};
#endif

